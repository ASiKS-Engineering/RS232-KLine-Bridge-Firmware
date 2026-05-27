/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#define F_CPU 	14745600
#endif

#include <avr/io.h>
#include "uart.h"
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>

//-----------------------------------------------------------------------------------------
//		Global types and definitions
//-----------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------
//		Global parameters
//-----------------------------------------------------------------------------------------
volatile uint8_t led0_timer = 0;
volatile uint8_t led1_timer = 0;


//-----------------------------------------------------------------------------------------
//		ISR(TIMER0_COMP_vect)
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine is intterupt entry point for timer 0 compare
//
//	Parameters:		none
//
//	Return:			none
//
// 	Author:			Dr. Andreas Schramm
//
//	Date;Changes:	2011-07-19; Initial   	
//
//-----------------------------------------------------------------------------------------
ISR(TIMER0_COMP_vect)
{
    if(led0_timer)
    {
        led0_timer--;

        if(!led0_timer)
        {
            PORTA &= ~(1<<PA0);
        }
    }

    if(led1_timer)
    {
        led1_timer--;

        if(!led1_timer)
        {
            PORTA &= ~(1<<PA1);
        }
    }
}



void startup_blink(void)
{
    // Beide LEDs nacheinander blinken
    for(uint8_t i = 0; i < 3; i++)
    {
        PORTA |= (1 << PA0);   // LED an PA0 EIN
        PORTA &= ~(1 << PA1);  // LED an PA1 AUS
        _delay_ms(200);

        PORTA &= ~(1 << PA0);  // LED an PA0 AUS
        PORTA |= (1 << PA1);   // LED an PA1 EIN
        _delay_ms(200);
    }

    // Beide kurz gleichzeitig einschalten
    PORTA |= (1 << PA0) | (1 << PA1);
    _delay_ms(500);

    // Beide aus
    PORTA &= ~((1 << PA0) | (1 << PA1));
}


//-----------------------------------------------------------------------------------------
//		main
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine is the main entry point after reset
//
//	Parameters:		none
//
//	Return:			none
//
// 	Author:			Dr. Andreas Schramm
//
//	Date;Changes:	2011-07-19; Initial   	
//
//-----------------------------------------------------------------------------------------
int main (void) 
{

	uint8_t u8_last_dtr_value=0xFF;
	uint8_t u8_bridgeEnabled=0;
	uint8_t u8_dtr_value = 0;

	//uint32_t u32_Baudrate;
	
	/* --- Disable interrupts --- */
	cli();

	/* --- Initialize the respective IOs --- */
	// PA0 und PA1 als Ausgang
	DDRA |= (1<<PA1) | (1<<PA0);

    // PA2 als Eingang
    DDRA &= ~(1 << PA2);

	// Set internal Pullups
	//PORTA |= (1<<PA2) | (1<<PA1) | (1<<PA0);
	PORTA |= (1<<PA1) | (1<<PA0);

	// Set PortD Pin 0 as Input
	DDRD &= ~(1<<PD0);
	// Set internal Pullups
	PORTD |= (1<<PD0) | (1<<PD1);


    // Set PD5 als input (Tristate)
    DDRD &= ~(1 << PD5);
    PORTD &= ~(1 << PD5);

	// Clear LEDs
	PORTA &= ~((1 << PA0) | (1 << PA1));

	startup_blink();


	/* --- Init timer 0 (~10ms) that services leds  --- */
	// 14745600 / 1024 = 14400 Hz
	// 14400 / 100 = 144 Takte
	// OCR0 = 143
    TCCR0 =
    (1<<WGM01) |   // CTC
    (1<<CS02) |
    (1<<CS00);     // /1024

	OCR0 = 143;

    TIMSK |= (1<<OCIE0);
	

  	/*
     *  Initialize UART library, pass baudrate and AVR cpu clock
     *  with the macro 
     *  UART_BAUD_SELECT() (normal speed mode )
     *  or 
     *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
     */
    uart_init(UART_BAUD_SELECT(10400UL,F_CPU)); 
    
	uart1_init(UART_BAUD_SELECT(19200UL,F_CPU)); 


	/* --- Enable interrupts --- */ 
    sei();
  

  	while(1)
  	{
 
		// Get current status of DTR line
	    u8_dtr_value = (PINA&(1<<PA2));

        // Service PA2 status - DTR line
        if(u8_dtr_value != u8_last_dtr_value)
        {
            u8_last_dtr_value = u8_dtr_value;

            if(u8_dtr_value)
            {
                u8_bridgeEnabled=1;
                
				Clear_UART_Buffers();

                //Enable UART 0
				UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
				
                // PD5 Tristate
                DDRD &= ~(1<<PD5);
                PORTD &= ~(1<<PD5);

            }
            else
            {
                u8_bridgeEnabled=0;

                // Disable UART 0
				UCSR0B=0;

                // PD5 LOW
                PORTD &= ~(1<<PD5);
                DDRD |= (1<<PD5);
            }
        }
		
		// Service UARTs only if bridge is not armed
		if(u8_bridgeEnabled == 1)
		{
			Service_UART0();
			Service_UART1();
		}
		else
		{
			Service_CMD();
		}
			
	
  }

  return 0;
}

 


