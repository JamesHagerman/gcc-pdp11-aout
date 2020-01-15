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
