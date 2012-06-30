CC ?= arm-raspberrypi-linux-gnueabi-gcc

VC_LIB ?= /home/hauxwell/vc/firmware/hardfp/opt/vc

CFLAGS =	-fpic \
			-pipe \
			-mfloat-abi=hard \
			-mfpu=vfp \
			-mtune=arm1176jzf-s \
			-march=armv6zk \
			-Os \
			-c \
			-I$(VC_LIB)/include \
			-I$(VC_LIB)/include/interface/vcos/pthreads

LDFLAGS =	-shared

SOURCES =	piglut.c \
				esutil.c

OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = libpiglut.so

all: $(SOURCES) $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE) *.o

$(EXECUTABLE): $(OBJECTS)
	@echo "Linking ... " $@
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	@echo "Compiling ... " $<
	@$(CC) $(CFLAGS) $< -o $@
