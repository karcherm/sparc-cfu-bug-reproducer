#include <stdio.h>

void* U3memcpy(void* dest, const void* src, size_t len);
size_t U3copy_from_user(void* dest, const void* src, size_t len);

const char srcbuffer[] = "Hello world";
char dstbuffer[sizeof srcbuffer];

int main(void)
{
    U3copy_from_user(dstbuffer, srcbuffer, sizeof dstbuffer);
    puts(dstbuffer);
}
