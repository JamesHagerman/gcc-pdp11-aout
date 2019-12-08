# GNU GCC 9.2.0 for PDP-11

This repo contains the `Dockerfile` and other files useful for compiling simple C programs for the PDP-11 line of
mini computers. Becuase PDP-11's are ancient technology, the real purpose is to compile programs for the PDP
machines emulated by [`simh`](http://simh.trailing-edge.com/).

This includes the [PiDP-11](https://obsolescence.wixsite.com/obsolescence/pidp-11)!

## Using the prebuilt image

Direct usage of `gcc` and the other tools is not available yet. You'll have to mount your source tree as a docker
volume for now...

`docker run -it gcc-pdp11-aout:working-v2 bash`

Once you have a shell inside the container, you will be able to compile C programs!

*Note: Keep in mind, this is not your normal C world. Things are different in PDP land! If this is your first time
coding for the PDP-11, you've got some reading ahead of you...*

There is an extremely simple `foo.c` in the image that can be used to test the compiler as follows. 


```
root@14031c390c9f:/usr/local/lib# pdp11-aout-gcc -nostdlib foo.c
root@14031c390c9f:/usr/local/lib# pdp11-aout-objdump -D a.out

a.out:     file format a.out-pdp11


Disassembly of section .text:

00000000 <_start>:
   0:	1166           	mov	r5, -(sp)
   2:	1185           	mov	sp, r5
   4:	0a00           	clr	r0
   6:	1585           	mov	(sp)+, r5
   8:	0087           	rts	pc
root@14031c390c9f:/usr/local/lib#
```

While that may get you a correctly compiled binary, the tricky part is getting that binary running under
`simh`. Honestly, I'm still figuring that part out. :(

## Building the image yourself

This will take a while. You'll be compiling `binutil` and `gnu gcc` from source.

`docker build --tag gcc-pdp11-aout:local .`

