KERNELSRC=.

ASFLAGS=-g -mcpu=niagara4 -include userheader.h -include asm/asi.h -I $(KERNELSRC)
CFLAGS=-g -O3

all: cfutest

GENmemcpy.o: $(KERNELSRC)/arch/sparc/lib/GENmemcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

# The define here is a workaround due to a missing macro in that file.
# I don't want to patch that file to be able to use the kernel version
# directly
U1memcpy.o: $(KERNELSRC)/arch/sparc/lib/U1memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $< -DVISExitHalf=VISExit

U3memcpy.o: $(KERNELSRC)/arch/sparc/lib/U3memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

Memcpy_utils.o: $(KERNELSRC)/arch/sparc/lib/Memcpy_utils.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

NGmemcpy.o: $(KERNELSRC)/arch/sparc/lib/NGmemcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $< -D 'VISEntryHalfFast(x)=VISEntryHalf' -D VISExitHalfFast=VISExitHalf

NG2memcpy.o: $(KERNELSRC)/arch/sparc/lib/NG2memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

NG4memcpy.o: $(KERNELSRC)/arch/sparc/lib/NG4memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $< -D 'VISEntryHalfFast(x)=VISEntryHalf' -D VISExitHalfFast=VISExitHalf

OBJECTS = cfutest.o \
          GENcopy_from_user.o U1copy_from_user.o U3copy_from_user.o NGcopy_from_user.o NG2copy_from_user.o NG4copy_from_user.o \
          GENcopy_to_user.o U1copy_to_user.o U3copy_to_user.o \
          GENmemcpy.o U1memcpy.o U3memcpy.o Memcpy_utils.o NGmemcpy.o NG2memcpy.o NG4memcpy.o
cfutest: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) cfutest
