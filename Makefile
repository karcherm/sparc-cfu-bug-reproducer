KERNELSRC=.

ASFLAGS=-g -mcpu=ultrasparc -include userheader.h -I $(KERNELSRC)
CFLAGS=-g -O3

all: cfutest

# The define here is a workaround due to a missing macro in that file.
# I don't want to patch that file to be able to use the kernel version
# directly
U1memcpy.o: $(KERNELSRC)/arch/sparc/lib/U1memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $< -DVISExitHalf=VISExit

U3memcpy.o: $(KERNELSRC)/arch/sparc/lib/U3memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

OBJECTS = cfutest.o U1copy_from_user.o U3copy_from_user.o U1memcpy.o U3memcpy.o
cfutest: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) cfutest
