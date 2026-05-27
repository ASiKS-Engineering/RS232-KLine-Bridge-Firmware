/*************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
File:     $Id: uart.c,v 1.6.2.2 2009/11/29 08:56:12 Peter Exp $
Software: AVR-GCC 4.1, AVR Libc 1.4.6 or higher
Hardware: any AVR with built-in UART, 
License:  GNU General Public License 
          
DESCRIPTION:
    An interrupt is generated when the UART has finished transmitting or
    receiving a byte. The interrupt handling routines use circular buffers
    for buffering received and transmitted data.
    
    The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE variables define
    the buffer size in bytes. Note that these variables must be a 
    power of 2.
    
USAGE:
    Refere to the header file uart.h for a description of the routines. 
    See also example test_uart.c.

NOTES:
    Based on Atmel Application Note AVR306
                    
LICENSE:
    Copyright (C) 2006 Peter Fleury

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                        
*************************************************************************/
/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#define F_CPU 	14745600
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"


/*
 *  constants and macros
 */

/* size of RX/TX buffers */
#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1)

#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
#error RX buffer size is not a power of 2
#endif
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
#error TX buffer size is not a power of 2
#endif

#if defined(__AVR_AT90S2313__) \
 || defined(__AVR_AT90S4414__) || defined(__AVR_AT90S4434__) \
 || defined(__AVR_AT90S8515__) || defined(__AVR_AT90S8535__) \
 || defined(__AVR_ATmega103__)
 /* old AVR classic or ATmega103 with one UART */
 #define AT90_UART
 #define UART0_RECEIVE_INTERRUPT   SIG_UART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_UART_DATA
 #define UART0_STATUS   USR
 #define UART0_CONTROL  UCR
 #define UART0_DATA     UDR  
 #define UART0_UDRIE    UDRIE
#elif defined(__AVR_AT90S2333__) || defined(__AVR_AT90S4433__)
 /* old AVR classic with one UART */
 #define AT90_UART
 #define UART0_RECEIVE_INTERRUPT   SIG_UART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_UART_DATA
 #define UART0_STATUS   UCSRA
 #define UART0_CONTROL  UCSRB
 #define UART0_DATA     UDR 
 #define UART0_UDRIE    UDRIE
#elif  defined(__AVR_ATmega8__)  || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__) \
  || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega8535__) \
  || defined(__AVR_ATmega323__)
  /* ATmega with one USART */
 #define ATMEGA_USART
 #define UART0_RECEIVE_INTERRUPT   SIG_UART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_UART_DATA
 #define UART0_STATUS   UCSRA
 #define UART0_CONTROL  UCSRB
 #define UART0_DATA     UDR
 #define UART0_UDRIE    UDRIE
#elif defined(__AVR_ATmega163__) 
  /* ATmega163 with one UART */
 #define ATMEGA_UART
 #define UART0_RECEIVE_INTERRUPT   SIG_UART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_UART_DATA
 #define UART0_STATUS   UCSRA
 #define UART0_CONTROL  UCSRB
 #define UART0_DATA     UDR
 #define UART0_UDRIE    UDRIE
#elif defined(__AVR_ATmega162__) 
 /* ATmega with two USART */
 #define ATMEGA_USART0
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   SIG_USART0_RECV
 #define UART1_RECEIVE_INTERRUPT   SIG_USART1_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART0_DATA
 #define UART1_TRANSMIT_INTERRUPT  SIG_USART1_DATA
 #define UART0_STATUS   UCSR0A
 #define UART0_CONTROL  UCSR0B
 #define UART0_DATA     UDR0
 #define UART0_UDRIE    UDRIE0
 #define UART1_STATUS   UCSR1A
 #define UART1_CONTROL  UCSR1B
 #define UART1_DATA     UDR1
 #define UART1_UDRIE    UDRIE1
#elif defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) 
 /* ATmega with two USART */
 #define ATMEGA_USART0
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   SIG_UART0_RECV
 #define UART1_RECEIVE_INTERRUPT   SIG_UART1_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_UART0_DATA
 #define UART1_TRANSMIT_INTERRUPT  SIG_UART1_DATA
 #define UART0_STATUS   UCSR0A
 #define UART0_CONTROL  UCSR0B
 #define UART0_DATA     UDR0
 #define UART0_UDRIE    UDRIE0
 #define UART1_STATUS   UCSR1A
 #define UART1_CONTROL  UCSR1B
 #define UART1_DATA     UDR1
 #define UART1_UDRIE    UDRIE1
#elif defined(__AVR_ATmega161__)
 /* ATmega with UART */
 #error "AVR ATmega161 currently not supported by this libaray !"
#elif defined(__AVR_ATmega169__) 
 /* ATmega with one USART */
 #define ATMEGA_USART
 #define UART0_RECEIVE_INTERRUPT   SIG_USART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART_DATA
 #define UART0_STATUS   UCSRA
 #define UART0_CONTROL  UCSRB
 #define UART0_DATA     UDR
 #define UART0_UDRIE    UDRIE
#elif defined(__AVR_ATmega48__) ||defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__)
 /* ATmega with one USART */
 #define ATMEGA_USART0
 #define UART0_RECEIVE_INTERRUPT   SIG_USART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART_DATA
 #define UART0_STATUS   UCSR0A
 #define UART0_CONTROL  UCSR0B
 #define UART0_DATA     UDR0
 #define UART0_UDRIE    UDRIE0
#elif defined(__AVR_ATtiny2313__)
 #define ATMEGA_USART
 #define UART0_RECEIVE_INTERRUPT   SIG_USART0_RX 
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART0_UDRE
 #define UART0_STATUS   UCSRA
 #define UART0_CONTROL  UCSRB
 #define UART0_DATA     UDR
 #define UART0_UDRIE    UDRIE
#elif defined(__AVR_ATmega329__) ||defined(__AVR_ATmega3290__) ||\
      defined(__AVR_ATmega649__) ||defined(__AVR_ATmega6490__) ||\
      defined(__AVR_ATmega325__) ||defined(__AVR_ATmega3250__) ||\
      defined(__AVR_ATmega645__) ||defined(__AVR_ATmega6450__)
  /* ATmega with one USART */
  #define ATMEGA_USART0
  #define UART0_RECEIVE_INTERRUPT   SIG_UART_RECV
  #define UART0_TRANSMIT_INTERRUPT  SIG_UART_DATA
  #define UART0_STATUS   UCSR0A
  #define UART0_CONTROL  UCSR0B
  #define UART0_DATA     UDR0
  #define UART0_UDRIE    UDRIE0
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(__AVR_ATmega1280__)  || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega640__)
/* ATmega with two USART */
  #define ATMEGA_USART0
  #define ATMEGA_USART1
  #define UART0_RECEIVE_INTERRUPT   SIG_USART0_RECV
  #define UART1_RECEIVE_INTERRUPT   SIG_USART1_RECV
  #define UART0_TRANSMIT_INTERRUPT  SIG_USART0_DATA
  #define UART1_TRANSMIT_INTERRUPT  SIG_USART1_DATA
  #define UART0_STATUS   UCSR0A
  #define UART0_CONTROL  UCSR0B
  #define UART0_DATA     UDR0
  #define UART0_UDRIE    UDRIE0
  #define UART1_STATUS   UCSR1A
  #define UART1_CONTROL  UCSR1B
  #define UART1_DATA     UDR1
  #define UART1_UDRIE    UDRIE1  
#elif defined(__AVR_ATmega644__)
 /* ATmega with one USART */
 #define ATMEGA_USART0
 #define UART0_RECEIVE_INTERRUPT   SIG_USART_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART_DATA
 #define UART0_STATUS   UCSR0A
 #define UART0_CONTROL  UCSR0B
 #define UART0_DATA     UDR0
 #define UART0_UDRIE    UDRIE0
#elif defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644P__)
 /* ATmega with two USART */
 #define ATMEGA_USART0
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   SIG_USART_RECV
 #define UART1_RECEIVE_INTERRUPT   SIG_USART1_RECV
 #define UART0_TRANSMIT_INTERRUPT  SIG_USART_DATA
 #define UART1_TRANSMIT_INTERRUPT  SIG_USART1_DATA
 #define UART0_STATUS   UCSR0A
 #define UART0_CONTROL  UCSR0B
 #define UART0_DATA     UDR0
 #define UART0_UDRIE    UDRIE0
 #define UART1_STATUS   UCSR1A
 #define UART1_CONTROL  UCSR1B
 #define UART1_DATA     UDR1
 #define UART1_UDRIE    UDRIE1
#else
 #error "no UART definition for MCU available"
#endif


/*
 *  module global variables
 */


static volatile uint8_t UART_TxBuf[UART_BUFFER_MAX];
static volatile uint8_t UART_RxBuf[UART_BUFFER_MAX];
static volatile uint16_t UART_TxHead;
static volatile uint16_t UART_TxTail;
static volatile uint16_t UART_RxHead;
static volatile uint16_t UART_RxTail;
static volatile uint8_t UART_LastRxError;
static uint16_t uart0_rx_size;
static uint16_t uart0_tx_size;
static uint16_t uart0_rx_mask;
static uint16_t uart0_tx_mask;




#if defined( ATMEGA_USART1 )
static volatile uint8_t UART1_TxBuf[UART_BUFFER_MAX];
static volatile uint8_t UART1_RxBuf[UART_BUFFER_MAX];
static volatile uint16_t UART1_TxHead;
static volatile uint16_t UART1_TxTail;
static volatile uint16_t UART1_RxHead;
static volatile uint16_t UART1_RxTail;
static volatile uint8_t UART1_LastRxError;
static uint16_t uart1_rx_size;
static uint16_t uart1_tx_size;
static uint16_t uart1_rx_mask;
static uint16_t uart1_tx_mask;

#endif

/*
 *  module extern variables
 */
extern volatile uint8_t led0_timer;
extern volatile uint8_t led1_timer;


uint16_t validate_buffer_size(uint16_t size)
{
    if( (size == 0) ||
        (size > UART_BUFFER_MAX) ||
        (size & (size - 1)) ) // zweierkomplement check
    {
        return UART_BUFFER_DEFAULT;
    }

    return size;
}

//void uart_buffers_init(uint16_t uart0_rx_buffer_size, uint16_t uart0_tx_buffer_size, uint16_t uart1_rx_buffer_size, uint16_t uart1_tx_buffer_size)
void uart_buffers_init(const settings_t *settings)
{
	
    uart0_rx_size =
        validate_buffer_size(settings->uart0_rx_buffer_size);

    uart0_tx_size =
        validate_buffer_size(settings->uart0_tx_buffer_size);

    uart1_rx_size =
        validate_buffer_size(settings->uart1_rx_buffer_size);

    uart1_tx_size =
        validate_buffer_size(settings->uart1_tx_buffer_size);
	
    uart0_rx_mask = uart0_rx_size - 1;
    uart0_tx_mask = uart0_tx_size - 1;

    uart1_rx_mask = uart1_rx_size - 1;
    uart1_tx_mask = uart1_tx_size - 1;
}


//SIGNAL(UART0_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART Receive Complete interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
/*
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    // read UART status register and UART data register  
    usr  = UART0_STATUS;
    data = UART0_DATA;
    

#if defined( AT90_UART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)|_BV(UPE)) );
#elif defined( ATMEGA_USART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)|_BV(UPE)) );
#elif defined( ATMEGA_USART0 )
    lastRxError = (usr & (_BV(FE0)|_BV(DOR0)|_BV(UPE0)) );
#elif defined ( ATMEGA_UART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)|_BV(UPE)) );
#endif
        
    // calculate buffer index 
    tmphead = ( UART_RxHead + 1) & UART_RX_BUFFER_MASK;
    
    if ( tmphead == UART_RxTail ) {
        // error: receive buffer overflow
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        // store new index 
        UART_RxHead = tmphead;
        // store received data in buffer
        UART_RxBuf[tmphead] = data;
    }
    UART_LastRxError = lastRxError;   
}
*/

ISR(UART0_RECEIVE_INTERRUPT)
{
    uint8_t head;
    uint8_t status;
    uint8_t data;
    uint8_t error;

    status = UART0_STATUS;
    data   = UART0_DATA;

#if defined(ATMEGA_USART0)
    error = status & (_BV(FE0) | _BV(DOR0) | _BV(UPE0));
#else
    error = status & (_BV(FE) | _BV(DOR) | _BV(UPE));
#endif

    //head = (UART_RxHead + 1) & UART_RX_BUFFER_MASK;
    head = (UART_RxHead + 1) & uart0_rx_mask;

    if (head == UART_RxTail)
    {
        error = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        UART_RxHead = head;
        UART_RxBuf[head] = data;
    }

    UART_LastRxError = error;
}


//SIGNAL(UART0_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART Data Register Empty interrupt
Purpose:  called when the UART is ready to transmit the next byte
**************************************************************************/
/*
{
	
    unsigned char tmptail;

    
    if ( UART_TxHead != UART_TxTail) {
        // calculate and store new buffer index 
        tmptail = (UART_TxTail + 1) & UART_TX_BUFFER_MASK;
        UART_TxTail = tmptail;
        // get one byte from buffer and write it to UART 
        UART0_DATA = UART_TxBuf[tmptail];  // start transmission
    }else{
        // tx buffer empty, disable UDRE interrupt 
        UART0_CONTROL &= ~_BV(UART0_UDRIE);
    }
}
*/
ISR(UART0_TRANSMIT_INTERRUPT)
{
    uint8_t tail;

    if (UART_TxHead == UART_TxTail)
    {
        UART0_CONTROL &= ~_BV(UART0_UDRIE);
        return;
    }

    //tail = (UART_TxTail + 1) & UART_TX_BUFFER_MASK;
    tail = (UART_TxTail + 1) & uart0_tx_mask;

    UART_TxTail = tail;

    UART0_DATA = UART_TxBuf[tail];
}



/*************************************************************************
Function: uart_init()
Purpose:  initialize UART and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart_init(uint16_t baudrate)
{
    UART_TxHead = 0;
    UART_TxTail = 0;
    UART_RxHead = 0;
    UART_RxTail = 0;

#if defined(ATMEGA_USART) || defined(ATMEGA_USART0) || defined(ATMEGA_UART)

    /* Double speed mode */
    if (baudrate & 0x8000)
    {
        UART0_STATUS = _BV(U2X0);
        baudrate &= ~0x8000;
    }

    /* Baudrate setzen */
    UBRR0H = (uint8_t)(baudrate >> 8);
    UBRR0L = (uint8_t)(baudrate);

    /* RX, TX, RX Interrupt */
    UART0_CONTROL = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);

    /* Frame format: 8N1 oder 8E1 (je nach Variante) */
#if defined(URSEL0)
    UCSR0C = _BV(URSEL0) | _BV(UCSZ01) | _BV(UCSZ00);
#elif defined(UPM01)
    UCSR0C = _BV(UPM01) | _BV(UCSZ01) | _BV(UCSZ00);
#else
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
#endif

#elif defined(AT90_UART)

    UBRR = (uint8_t)baudrate;

    UART0_CONTROL = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);

#endif
}/* uart_init */

void Clear_UART_Buffers(void)
{
    UART_RxHead = UART_RxTail = 0;
    UART1_RxHead = UART1_RxTail = 0;
	UART_TxHead = UART_TxTail = 0;
    UART1_TxHead = UART1_TxTail = 0;
}


/*************************************************************************
Function: uart_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart_getc(void)
{    
    unsigned char tmptail;
    //unsigned char data;


    if ( UART_RxHead == UART_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate /store buffer index */
    //tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;
    tmptail = (UART_RxTail + 1) & uart0_rx_mask;

	UART_RxTail = tmptail; 
    
    /* get data from receive buffer */
    //data = UART_RxBuf[tmptail];
    
    return (UART_LastRxError << 8) + UART_RxBuf[tmptail];

}/* uart_getc */


/*************************************************************************
Function: uart_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart_putc(unsigned char data)
{
    unsigned char tmphead;

    
    //tmphead  = (UART_TxHead + 1) & UART_TX_BUFFER_MASK;
    tmphead = (UART_TxHead + 1) & uart0_tx_mask;

	
    while ( tmphead == UART_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART_TxBuf[tmphead] = data;
    UART_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART0_CONTROL    |= _BV(UART0_UDRIE);

}/* uart_putc */




/*
 * these functions are only for ATmegas with two USART
 */
#if defined( ATMEGA_USART1 )

//SIGNAL(UART1_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART1 Receive Complete interrupt
Purpose:  called when the UART1 has received a character
**************************************************************************/
/*
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    // read UART status register and UART data register 
    usr  = UART1_STATUS;
    data = UART1_DATA;
    
    lastRxError = (usr & (_BV(FE1)|_BV(DOR1)|_BV(UPE1)) );
        
    // calculate buffer index 
    tmphead = ( UART1_RxHead + 1) & UART_RX_BUFFER_MASK;
    
    if ( tmphead == UART1_RxTail ) {
        // error: receive buffer overflow 
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        // store new index
        UART1_RxHead = tmphead;
        // store received data in buffer
        UART1_RxBuf[tmphead] = data;
    }
    UART1_LastRxError = lastRxError;  
	 
}
*/
ISR(UART1_RECEIVE_INTERRUPT)
{
    uint8_t head;
    uint8_t status;
    uint8_t data;
    uint8_t error;

    status = UART1_STATUS;
    data   = UART1_DATA;

    error = status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1));

    //head = (UART1_RxHead + 1) & UART_RX_BUFFER_MASK;
    head = (UART1_RxHead + 1) & uart1_rx_mask;

    if (head == UART1_RxTail)
    {
        error = UART_BUFFER_OVERFLOW >> 8;
    }
    else
    {
        UART1_RxHead = head;
        UART1_RxBuf[head] = data;
    }

    UART1_LastRxError = error;
}



//SIGNAL(UART1_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART1 Data Register Empty interrupt
Purpose:  called when the UART1 is ready to transmit the next byte
**************************************************************************/
/*
{
    unsigned char tmptail;

    
    if ( UART1_TxHead != UART1_TxTail) {
        // calculate and store new buffer index 
        tmptail = (UART1_TxTail + 1) & UART_TX_BUFFER_MASK;
        UART1_TxTail = tmptail;
        // get one byte from buffer and write it to UART 
        UART1_DATA = UART1_TxBuf[tmptail];  // start transmission 
    }else{
        // tx buffer empty, disable UDRE interrupt
        UART1_CONTROL &= ~_BV(UART1_UDRIE);
    }
}
*/

ISR(UART1_TRANSMIT_INTERRUPT)
{
    uint8_t tail;

    if (UART1_TxHead == UART1_TxTail)
    {
        UART1_CONTROL &= ~_BV(UART1_UDRIE);
        return;
    }

    //tail = (UART1_TxTail + 1) & UART_TX_BUFFER_MASK;
	tail = (UART1_TxTail + 1) & uart1_tx_mask;
    UART1_TxTail = tail;

    UART1_DATA = UART1_TxBuf[tail];
}


/*************************************************************************
Function: uart1_init()
Purpose:  initialize UART1 and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart1_init(uint16_t baudrate)
{
    UART1_TxHead = 0;
    UART1_TxTail = 0;
    UART1_RxHead = 0;
    UART1_RxTail = 0;

    /* Double speed mode */
    if (baudrate & 0x8000)
    {
        UART1_STATUS = _BV(U2X1);
        baudrate &= ~0x8000;
    }

    /* Baudrate setzen */
    UBRR1H = (uint8_t)(baudrate >> 8);
    UBRR1L = (uint8_t)(baudrate);

    /* RX + TX + RX Interrupt aktivieren */
    UART1_CONTROL = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);

    /* Frame: 8N1 */
#ifdef URSEL1
    UCSR1C = _BV(URSEL1) | _BV(UCSZ11) | _BV(UCSZ10);
#else
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
#endif
}/* uart_init */


/*************************************************************************
Function: uart1_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart1_getc(void)
{    
    unsigned char tmptail;
    //unsigned char data;


    if ( UART1_RxHead == UART1_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate /store buffer index */
    //tmptail = (UART1_RxTail + 1) & UART_RX_BUFFER_MASK;
    tmptail = (UART1_RxTail + 1) & uart1_rx_mask;
	UART1_RxTail = tmptail; 
    
    /* get data from receive buffer */
    //data = UART1_RxBuf[tmptail];
    
    return (UART1_LastRxError << 8) + UART1_RxBuf[tmptail];

}/* uart1_getc */


/*************************************************************************
Function: uart1_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart1_putc(unsigned char data)
{
    unsigned char tmphead;

    
    //tmphead = (UART1_TxHead + 1) & UART_TX_BUFFER_MASK;
    tmphead = (UART1_txHead + 1) & uart1_tx_mask;
	
    while ( tmphead == UART1_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART1_TxBuf[tmphead] = data;
    UART1_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART1_CONTROL    |= _BV(UART1_UDRIE);

}/* uart1_putc */






void Service_UART0(void)
{
    uint16_t c;

    c = uart_getc();

    if (c & UART_NO_DATA)
    {
        return;
    }

    /* Error handling */
    if (c & (UART_FRAME_ERROR | UART_OVERRUN_ERROR |
             UART_BUFFER_OVERFLOW | UART_PARITY_ERROR))
    {
        PORTA |= (1 << PA1);
        led1_timer = 5;
    }

    /* normal receive action */
    PORTA |= (1 << PA0);
    led0_timer = 2;

    uart1_putc((uint8_t)c);
}



void Service_UART1(void)
{
    uint16_t c;

    c = uart1_getc();

    if (c & UART_NO_DATA)
    {
        return;
    }

    /* Error handling (all errors treated the same) */
    if (c & (UART_FRAME_ERROR | UART_OVERRUN_ERROR |
             UART_BUFFER_OVERFLOW | UART_PARITY_ERROR))
    {
        PORTA |= (1 << PA1);
        led1_timer = 5;
    }

    /* Echo back received character */
    uart_putc((uint8_t)c);
}


uint8_t console_active = 0;

void Service_CMD(void){

    uint16_t c;

    c = uart1_getc();
	
    // Wait for console activation
    if (!console_active)
    {
        if (c == '-')
        {
            console_active = 1;
        }
        return;
    }

    // Console-commands
    switch (c)
    {
        case 'v':
                // Send 
			    uart1_putc("v");
			    uart1_putc(VERSION_MAJOR);
			    uart1_putc(".");
    			uart1_putc(VERSION_MINOR);
			    uart1_putc(".");
    			uart1_putc(VERSION_PATCH);
    			uart1_putc("/r");
    			uart1_putc("/n");
			    console_active = 0;
            break;

        default:
            // Send unknown cmd response
            uart1_putc('?');
		    console_active = 0;
            break;
    }

}



#endif
