CC = pdp11-aout-gcc
CFLAGS ?= -Os -ffreestanding
M11 ?= macro11
M11FLAGS ?= -d md -d me -e bex -yus
DUMPOBJ ?= dumpobj
SIMH ?= pdp11

all : console-echo.bin

console-echo.asm : console-echo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S -mdec-asm -o console-echo.asm console-echo.c

console-echo.obj : console-echo.asm
	$(M11) console-echo.asm $(M11FLAGS) -l console-echo.lst -o console-echo.obj

console-echo.bin : console-echo.obj
	$(DUMPOBJ) console-echo.obj console-echo.bin > console-echo.dmp

clean:
	rm -f console-echo.asm console-echo.bin console-echo.obj console-echo.lst console-echo.dmp 

test:
	$(SIMH) console-echo.ini
