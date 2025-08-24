#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>

void* U1memcpy(void* dest, const void* src, size_t len);
void* U3memcpy(void* dest, const void* src, size_t len);
size_t U1copy_from_user(void* dest, const void* src, size_t len);
size_t U3copy_from_user(void* dest, const void* src, size_t len);

size_t (*copy_from_user)(void*, const void*, size_t) = U3copy_from_user;

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

int main(int argc, char** argv)
{
    int i;
    struct sigaction sa;
    if (argc > 1)
    {
        if (!strcmp(argv[1], "--u1"))
            copy_from_user = U1copy_from_user;
        else if(!strcmp(argv[1], "--u3"))
            copy_from_user = U3copy_from_user;
        else
        {
            fputs("Bad option, use --u1 or --u3 (default)\n", stderr);
            return 1;
        }
    }
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
        size_t indicated_remaining, actual_remaining;
        char* actual_end;
        const char* error_type;
        memset(dstbuffer, 0xFF, BUFFERSIZE);
        indicated_remaining = copy_from_user(dstbuffer, srcbuffer+BUFFERSIZE-i, STANDARDSIZE);
        actual_end = memchr(dstbuffer, 0xFF, BUFFERSIZE);
        if (actual_end == NULL)
            actual_end = dstbuffer + STANDARDSIZE;
        actual_remaining = STANDARDSIZE - (actual_end-dstbuffer);

        error_type = NULL;
        if (indicated_remaining > STANDARDSIZE)
            error_type = "UNREASONABLY HIGH";
        else if (indicated_remaining != STANDARDSIZE && dstbuffer[STANDARDSIZE - indicated_remaining - 1] != (char)0xAA)
            error_type = "TOO LOW";
        else if (dstbuffer[STANDARDSIZE - indicated_remaining] != (char)0xFF)
            error_type = "TOO_HIGH";

        if (error_type)
        {
            printf("%s: %d bytes till fault returned %zd bytes left, but should have returned %zd, fault at %llx\n", error_type, i, indicated_remaining, actual_remaining, last_sigsegv_pc);
        }
    }
    return 0;
}
