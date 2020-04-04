# MACRO-11 Example

This example demonstrates how to use gcc to generate MACRO-11 compatible assembly output
from C code.

The example code is a simple console echo program, similar to those described in various
DEC documentation.  The code uses a C struct (defined in DL11.h) to provide symbolic access
to the console registers.

```
#include "DL11.h"

__asm__(".asect"); // Assemble code in absolute mode
__asm__(".=0200"); // Place start() at 0200

void start(void)
{
    struct DL11 * const console = (struct DL11 *)0177560;

    for (;;)
    {
        // Wait character received on console.
        while (console->RCSRLow == 0);

        // Read character from receiver buffer and write to transmitter buffer.
        console->XBUFLow = console->RBUFLow;
    }
}
```

The output of the compiler is fairly concise when compiled with size optimization (-Os),
although not quite as small as the hand coded variants:

```
 1                                .title  console-echo.c
 2                                .enabl  lsb,reg
 3                            
 4 000000                         .psect  code,i,ro,con
 5 000000                         .asect
 6        000200                  .=0200
 7                                .even
 8                                .globl  start
 9                            start:
10                            3$:
11 000200 113700  177560          movb    @#-220,r0
12 000204 105700                  tstb    r0
13 000206 001774                  beq 3$
14 000210 113700  177562          movb    @#-216,r0
15 000214 110037  177566          movb    r0,@#-212
16 000220 000767                  br  3$
17                                .end
```

## Compiling the code

To compile and assemble the code by hand, perform the following steps:

```
$ pdp11-aout-gcc -Os -ffreestanding -S -mdec-asm -o console-echo.asm console-echo.c
$ macro11 console-echo.asm -d md -d me -e bex -yus -l console-echo.lst -o console-echo.obj
```

To convert the resultant object file into absolute-loader format, do the following:

```
$ dumpobj console-echo.obj console-echo.bin
```

There is also a Makefile available that automates the above steps:

```
$ make
pdp11-aout-gcc  -Os -ffreestanding -S -mdec-asm -o console-echo.asm console-echo.c
macro11 console-echo.asm -d md -d me -e bex -yus -l console-echo.lst -o console-echo.obj
dumpobj console-echo.obj console-echo.bin > console-echo.dmp
```

## Testing the code with simh

To test the example, enter the following commands in the simh PDP-11 simulator:

```
$ pdp11
sim> set cpu 11/05
sim> set cpu 16K
sim> set cpu idle
sim> load console-echo.bin
sim> g 0200
```

Once the code is running, you should be able to type characters at the console and
see them echoed back.

The code can also be run using the `make test` target:

```
$ make test
PDP-11 simulator V4.0-0 Current        git commit id: c8f73155
Disabling XQ

Loading console-echo.bin
Starting program...
```


