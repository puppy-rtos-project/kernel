/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <puppy_core.h>
#include "drv_uart.h"

#include <stdio.h>

struct device_uart
{
    pup_ubase_t hw_base;
    uint32_t irqno;
};

void *uart0_base = (void*)0x10000000;
struct device_uart uart0;

void uart_init(void)
{
    uart0.hw_base = (pup_ubase_t)uart0_base;
    uart0.irqno = 0x0a;
    uint32_t div = UART_REFERENCE_CLOCK / (UART_DEFAULT_BAUDRATE * 16);

    write8_uart0(UART_IER, 0x00);
    write8_uart0(UART_LCR, UART_LCR_BAUD_LATCH);

    // LSB
    write8_uart0(0, div & 0xff);
    // MSB
    write8_uart0(1, (div >> 8) & 0xff);

    // set word length to 8 bits, no parity
    write8_uart0(UART_LCR, UART_LCR_EIGHT_BITS);

    write8_uart0(UART_FCR, UART_FCR_FIFO_ENABLE | UART_FCR_FIFO_CLEAR);
    uint8_t value = read8_uart0(UART_IER);
    write8_uart0(UART_IER, value | UART_IER_RX_ENABLE);
    return;
}

int uart_putc(char c)
{
    struct device_uart *uart;
    uart = &uart0;

    // wait for Transmit Holding Empty to be set in LSR.
    while((read8_uart0(UART_LSR) & UART_LSR_TX_IDLE) == 0)
        ;
    write8_uart0(UART_THR, c);

    return (1);
}

int uart_getc()
{
    struct device_uart *uart;
    volatile uint32_t lsr;
    int ch = -1;

    uart = &uart0;
    lsr = read8_uart0(UART_LSR);

    if (lsr & UART_LSR_RX_READY)
    {
        ch = read8_uart0(UART_RHR);
    }
    return ch;
}
