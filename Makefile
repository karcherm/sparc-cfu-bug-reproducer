KERNELSRC=.

ASFLAGS=-g -mcpu=ultrasparc -I$(KERNELSRC)
CFLAGS=-g -O3

all: cfutest

OBJECTS = cfutest.o copy_from_user.o memcpy.o
cfutest: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) cfutest
