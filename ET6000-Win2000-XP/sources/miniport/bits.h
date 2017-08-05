/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000BITS_H_
#define _ET6000BITS_H_


/*****************************************************************************/
/*
 * Set bits in a byte pointed by addr; mask must contain 0s at the bits
 * positions to be set and must contain 1s at all other bits; val must
 * contain the values of bits to be set.
 */
__inline void set8(volatile char *addr, char mask, char val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void set16(volatile short *addr, short mask, short val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void set32(volatile int *addr, int mask, int val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline void ioSet8(short port, char mask, char val)
{
    char current;

    if (mask == 0) {
        __asm {
            mov al, val
            mov dx, port
            out dx, al
        }
    }
    else {
        __asm {
            mov dx, port
            in al, dx
            mov current, al
        }
        current = (current & mask) | (val & ~mask);
        __asm {
            mov al, current
            mov dx, port
            out dx, al
        }
    }
}
/*****************************************************************************/
__inline void ioSet16(short port, short mask, short val)
{
    short current;

    if (mask == 0) {
        __asm {
            mov ax, val
            mov dx, port
            out dx, ax
        }
    }
    else {    
        __asm {
            mov dx, port
            in ax, dx
            mov current, ax
        }
        current = (current & mask) | (val & ~mask);
        __asm {
            mov ax, current
            mov dx, port
            out dx, ax
        }
    }
}
/*****************************************************************************/
__inline void ioSet32(short port, int mask, int val)
{
    int current;

    if (mask == 0) {
        __asm {
            mov eax, val
            mov dx, port
            out dx, eax
        }
    }
    else {
        __asm {
            mov dx, port
            in eax, dx
            mov current, eax
        }
        current = (current & mask) | (val & ~mask);
        __asm {
            mov eax, current
            mov dx, port
            out dx, eax
        }
    }
}
/*****************************************************************************/
__inline char ioGet8(short port)
{
    char current;
    __asm {
        mov dx, port
        in al, dx
        mov current, al
    }
    return current;
}
/*****************************************************************************/
__inline short ioGet16(short port)
{
    short current;
    __asm {
        mov dx, port
        in ax, dx
        mov current, ax
    }
    return current;
}
/*****************************************************************************/
__inline int ioGet32(short port)
{
    int current;
    __asm {
        mov dx, port
        in eax, dx
        mov current, eax
    }
    return current;
}
/*****************************************************************************/


#endif /* _ET6000BITS_H_ */
