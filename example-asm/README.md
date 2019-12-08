# ASM Example

This example was copied from https://ancientbits.blogspot.com/2012/07/programming-barebones-pdp11.html

It is intended to print a string to the system console. It has not been tested fully but I have put it here for
reference as it should be pretty close to working:

## Compiling the code

```
# Compile the object files
pdp11-aout-as putconch.s -o putconch.o
pdp11-aout-as hellopdp.s -o hellopdp.o

# Link the object files into a binary with reasonable addresses for the PDP-11/simh
pdp11-aout-ld -T ldaout.cmd hellopdp.o putconch.o -o hellopdp.out

# Use the authors `bin2load` file (found in ./tools/) to convert the output to a LDA format
bin2load -a -f hellopdp.out -o hellopdp.lda
```

## Running the example in `simh`

Again, this has not been tested, just taken wholesale from ancientbits' blog post...

```
sim> load hellopdp.lda
sim> e pc
PC:     001000
sim> e -m 001000:01100
1000:   MOV #10000,SP
1004:   MOV #1200,R1
1010:   MOV #14,R2
1014:   MOVB (R1),R0
1016:   JSR PC,1040
1022:   DEC R2
1024:   BEQ 1034
1026:   INC R1
1030:   JMP 1014
1034:   NOP
1036:   HALT
1040:   MOV R1,-(SP)
1042:   MOV R2,-(SP)
1044:   MOV #11610,R1
1050:   MOV 177564,R2
1054:   BIT R2,#200
1060:   BNE 1076
1062:   DEC R1
1064:   BNE 1050
1066:   MOV #2,R0
1072:   JMP 1140
1076:   MOVB R0,177566
sim> g
Hello world!
HALT instruction, PC: 001040 (MOV R1,-(SP))
sim>
```

