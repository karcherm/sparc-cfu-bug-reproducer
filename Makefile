KERNELSRC=.

ASFLAGS=-g -mcpu=ultrasparc -include userheader.h -I $(KERNELSRC)
CFLAGS=-g -O3

all: cfutest

U3memcpy.o: $(KERNELSRC)/arch/sparc/lib/U3memcpy.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

OBJECTS = cfutest.o U3copy_from_user.o U3memcpy.o
cfutest: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) cfutest
