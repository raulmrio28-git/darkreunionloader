#include "dcc/plat.h"
#include "io/io_type.h"

/*
The following is code for UART I/O, reverse engineered from a
QSC60x5-based device's ARMPRG.

No software interface stuffs are found, so we assume almost
everything here is the same as QSC60x0 one, except the register
locations.
*/

void qsc60x5_uart_drain();

/* Stars of the show */

bool qsc60x5_uart_init()
{
    uint8_t r;
    WRITE_U8(0x80018038, 0); //clear UART_IRDA reg
    qsc60x5_uart_drain();
    while ((READ_U8(0x80018008) & 1) == 0) //read and wdog while RXRDY bit of UART_SR is 0
    {
        r = READ_U8(0x8001800C); //read till death
        if (r) {}
        wdog_reset();
    }
    return true;
}

bool qsc60x5_uart_active()
{
    bool ret = false;
    uint8_t s = READ_U8(0x80018008);

    if (s & 1) //RXRDY bit of UART_SR enabled...
    {
        if (s>>4) //err bits found
        {
            WRITE_U8(0x80018010, (3<<4)); //reset errors
            WRITE_U8(0x80018010, (1<<4)); //reset RX
            WRITE_U8(0x80018010, (1<<4)); //reset RX again
            WRITE_U8(0x80018010, 1); //clear other UART_CR bits and make RX enabled
        }
        else
            ret = true;
    }
    return ret;
}

void qsc60x5_uart_drain()
{
    while ((READ_U8(0x80018008) & 8) == 0) //wdog while TXEMT bit of UART_SR is 0
        wdog_reset();
}


void qsc60x5_uart_read()
{
    uint8_t len = 0;
    uint8_t v = 0;
    wdog_reset();
    while(true)
    {
        if ((len>0x7f)|((READ_U8(0x80018008) & 1) == 0))
            break; //length over or RXRDY flag not set
        len++;
        if ((READ_U8(0x80018008) >> 3))
        {
            WRITE_U8(0x80018010, (3<<4)); //reset errors
            WRITE_U8(0x80018010, (1<<4)); //reset RX
            WRITE_U8(0x80018010, (1<<4)); //reset RX again
            WRITE_U8(0x80018010, 1); //clear other UART_CR bits and make RX enabled
        }
        else
        {
            v = READ_U8(0x8001800C);
            DRL_Packet_RX(v);
        }
        wdog_reset();
    }
}

void qsc60x5_uart_write(uint32_t b)
{
    while ((READ_U8(0x80018014) & 1) == 0) //read and wdog while TXLEV bit of UART_SR is 0
    {
        wdog_reset();
    }
    WRITE_U8(0x8001800C, b); //write our new value
}

drl_io_funcs_t io_uart = {
    .init = qsc60x5_uart_init,
    .active = qsc60x5_uart_active,
    .drain = qsc60x5_uart_drain,
    .read = qsc60x5_uart_read,
    .write = qsc60x5_uart_write,
    .bitsize = 8
};