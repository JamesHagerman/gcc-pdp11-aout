#ifndef DL11_H
#define DL11_H

#include <stdint.h>

struct DL11
{
    // Receiver Status Register
    union
    {
        volatile uint16_t RCSR;
        struct
        {
            volatile uint8_t RCSRLow;
            volatile uint8_t RCSRHigh;
        };
    };

    // Receiver Buffer Register
    union
    {
        volatile uint16_t RBUF;
        struct
        {
            volatile uint8_t RBUFLow;
            volatile uint8_t RBUFHigh;
        };
    };

    // Transmitter Status Register
    union
    {
        volatile uint16_t XCSR;
        struct
        {
            volatile uint8_t XCSRLow;
            volatile uint8_t XCSRHigh;
        };
    };

    // Transmitter Buffer Register
    union
    {
        volatile uint16_t XBUF;
        struct
        {
            volatile uint8_t XBUFLow;
            volatile uint8_t XBUFHigh;
        };
    };

};

#endif // DL11_H
