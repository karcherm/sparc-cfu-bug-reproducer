#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>

void* U1memcpy(void* dest, const void* src, size_t len);
void* U3memcpy(void* dest, const void* src, size_t len);
void* NGmemcpy(void* dest, const void* src, size_t len);
void* NG2memcpy(void* dest, const void* src, size_t len);
void* NG4memcpy(void* dest, const void* src, size_t len);
size_t U1copy_from_user(void* dest, const void* src, size_t len);
size_t U3copy_from_user(void* dest, const void* src, size_t len);
size_t NGcopy_from_user(void* dest, const void* src, size_t len);
size_t NG2copy_from_user(void* dest, const void* src, size_t len);
size_t NG4copy_from_user(void* dest, const void* src, size_t len);

typedef size_t copy_fn(void*, const void*, size_t);
copy_fn *copy_from_user = U3copy_from_user;

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

void run_test(copy_fn *fn, char *dest, const char *src, size_t destspace, size_t try_to_copy, size_t till_fault)
{
    size_t indicated_remaining, actual_remaining;
    char* actual_end;
    const char* error_type;

    last_sigsegv_pc = 0;
    memset(dest, 0xFF, destspace);
    indicated_remaining = fn(dest, src, try_to_copy);
    actual_end = memchr(dest, 0xFF, destspace);
    if (actual_end == NULL)
        actual_end = dest + destspace;
    actual_remaining = try_to_copy - (actual_end-dest);

    error_type = NULL;
    if (indicated_remaining > try_to_copy)
        error_type = "UNREASONABLY HIGH";
    else if (indicated_remaining != try_to_copy && dest[try_to_copy - indicated_remaining - 1] != (char)0xAA)
        error_type = "TOO LOW";
    else if (dest[try_to_copy - indicated_remaining] == (char)0xAA)
        error_type = "TOO_HIGH";

    if (error_type)
    {
        printf("%s: %d bytes till fault returned %zd of %zd bytes left, but should have returned %zd, fault at %llx\n", error_type, till_fault, indicated_remaining, try_to_copy, actual_remaining, last_sigsegv_pc);
    }
}

void run_from_user_test(size_t request_size, size_t bytes_till_fault, size_t dst_misalignment)
{
    run_test(copy_from_user, dstbuffer + dst_misalignment, srcbuffer + BUFFERSIZE - bytes_till_fault, request_size + 128, request_size, bytes_till_fault);
}

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
const size_t copy_sizes[] = {1, 7, 8, 15, 16, 17, 63, 64, 65, 191, 192, 193, 319, 320, 321, 511, 1023, 1024};
const size_t misalignments[] = {0, 1, 7, 8, 15, 16, 17, 33, 63};
int main(int argc, char** argv)
{
    int i, sizeidx, alignidx;
    struct sigaction sa;
    if (argc > 1)
    {
        if (!strcmp(argv[1], "--u1"))
            copy_from_user = U1copy_from_user;
        else if(!strcmp(argv[1], "--u3"))
            copy_from_user = U3copy_from_user;
        else if(!strcmp(argv[1], "--ng"))
            copy_from_user = NGcopy_from_user;
        else if(!strcmp(argv[1], "--ng2"))
            copy_from_user = NG2copy_from_user;
        else if(!strcmp(argv[1], "--ng4"))
            copy_from_user = NG4copy_from_user;
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

    for (sizeidx = 0; sizeidx < ARRAY_SIZE(copy_sizes); sizeidx++)
    {
        size_t copy_size = copy_sizes[sizeidx];
        for (alignidx = 0; alignidx < ARRAY_SIZE(misalignments); alignidx++)
        {
            for (i = 0; i <= copy_size; i++)
            {
                run_from_user_test(copy_size, i, misalignments[alignidx]);
            }
        }
    }
    return 0;
}
