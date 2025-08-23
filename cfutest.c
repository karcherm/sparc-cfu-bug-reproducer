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

void my_sigsegv(int sig, siginfo_t *info, void *ucontext)
{
    const extable_t *ptr;
    struct sigcontext *sc = (struct sigcontext*)ucontext;
    uint32_t low_pc = sc->sigc_regs.tpc & 0xffffffff;
    printf("SIGSEGV at %p accessing %p: code:%d\n", sc->sigc_regs.tpc, info->si_addr, info->si_code);
    for (ptr = extable_start; ptr < extable_end; ptr++)
    {
        if (ptr->code_addr == low_pc)
        {
            sc->sigc_regs.tnpc = sc->sigc_regs.tpc - ptr->code_addr + ptr->handler_addr;
            sc->sigc_regs.tpc = sc->sigc_regs.tnpc - 4;
            return;
        }
    }
    puts("No handler found");
    exit(2);
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
    sa.sa_sigaction = my_sigsegv;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    for (i = 0; i < 1024; i++)
    {
        size_t remaining = U3copy_from_user(dstbuffer, srcbuffer+BUFFERSIZE-i, 1024);
        if (remaining > 1024)
                printf("BAD: %d %zd\n", i, remaining);
    }
    return 0;
}
