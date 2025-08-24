#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>

void* U3memcpy(void* dest, const void* src, size_t len);
size_t U3copy_from_user(void* dest, const void* src, size_t len);

#define BUFFERSIZE 8192
#define STANDARDSIZE 1024

char *srcbuffer;
char *dstbuffer;

extern char __start___ex_table;
extern char __stop___ex_table;

typedef struct {
    uint32_t code_addr;
    uint32_t handler_addr;
} extable_t;

const extable_t* const extable_start = (const extable_t*)&__start___ex_table;
const extable_t* const extable_end = (const extable_t*)&__stop___ex_table;

volatile uint64_t last_sigsegv_pc;

void my_sigsegv(int sig, siginfo_t *info, void *ucontext)
{
    const extable_t *ptr;
    struct sigcontext *sc = (struct sigcontext*)ucontext;
    last_sigsegv_pc = sc->sigc_regs.tpc;
    uint32_t low_pc = sc->sigc_regs.tpc & 0xffffffff;
    for (ptr = extable_start; ptr < extable_end; ptr++)
    {
        if (ptr->code_addr == low_pc)
        {
            sc->sigc_regs.tnpc = sc->sigc_regs.tpc - ptr->code_addr + ptr->handler_addr;
            sc->sigc_regs.tpc = sc->sigc_regs.tnpc - 4;
            return;
        }
    }
    signal(SIGSEGV, SIG_DFL);
}

int main(void)
{
    int i;
    struct sigaction sa;
    srcbuffer = mmap(NULL, 3*BUFFERSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (srcbuffer == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    dstbuffer = srcbuffer + 2*BUFFERSIZE;
    munmap(srcbuffer + BUFFERSIZE, BUFFERSIZE);
    memset(srcbuffer, 0xAA, BUFFERSIZE);

    sa.sa_sigaction = my_sigsegv;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    for (i = 0; i < STANDARDSIZE; i++)
    {
        size_t remaining;
        memset(dstbuffer, 0xFF, BUFFERSIZE);
        remaining = U3copy_from_user(dstbuffer, srcbuffer+BUFFERSIZE-i, STANDARDSIZE);
        if (remaining > STANDARDSIZE)
        {
            printf("WAY TOO HIGH: %d %zd, fault at %llx\n", i, remaining, last_sigsegv_pc);
        }
        else if (remaining != STANDARDSIZE && dstbuffer[STANDARDSIZE - remaining - 1] != (char)0xAA)
        {
            printf("TOO LOW: %d %zd, fault at %llx\n", i, remaining, last_sigsegv_pc);
        }
        else if (dstbuffer[STANDARDSIZE - remaining] != (char)0xFF)
        {
            char* actual_end = memchr(dstbuffer, 0xFF, BUFFERSIZE);
            printf("TOO HIGH: %d %zd, fault at %llx\n", i, remaining, last_sigsegv_pc);
            printf(" correct remainder: %zd\n", STANDARDSIZE - (actual_end-dstbuffer));
        }
    }
    return 0;
}
