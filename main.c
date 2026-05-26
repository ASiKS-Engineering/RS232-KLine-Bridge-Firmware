/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#define F_CPU 	14745600
#endif

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include "uart.h"

#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <avr/pgmspace.h>

#include "helpers.h"


//-----------------------------------------------------------------------------------------
//		Global types and definitions
//-----------------------------------------------------------------------------------------

#define DEFAULT_COM1_BAUDRATE	10400
#define DEFAULT_COM2_BAUDRATE	19200


typedef union{
uint32_t ulBaudrate;
struct{
uint8_t ucBaudrateByte4;
uint8_t ucBaudrateByte3;
uint8_t ucBaudrateByte2;
uint8_t ucBaudrateByte1;
}byte;
} BAUDRATE_UNION;


//-----------------------------------------------------------------------------------------
//		Global parameters
//-----------------------------------------------------------------------------------------
BAUDRATE_UNION Baudrate_COM1;
BAUDRATE_UNION Baudrate_COM2;


volatile uint8_t led0_timer=0;
volatile uint8_t led1_timer=0;

char cCfgStringBuffer[100];

unsigned char cPortValue; 



void ClearRedLED(void)
{

		PORTA&= ~0x01;

}



void SetRedLED(void)
{

	PORTA |= 0x01;

}


void ClearGreenLED(void)
{

	PORTA &= ~0x02;

}



void SetGreenLED(void)
{

	PORTA |= 0x02;

}


uint8_t pa2_is_high(void)
{
    return (PINA & (1 << PA2));
}




//-----------------------------------------------------------------------------------------
//		ServiceBaudrateCfg
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine services the baud rate configuration change mechanism
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
//void ServiceBaudrateCfg(void)
ISR(PCINT0_vect)

{

	cPortValue = PINA & _BV(PA2);

	if(cPortValue == 0x04)
	{

		if(uiCfgFlagArmed == 1)
		{
			// Processe received BAUDRATE config string  
			//uart1_gets()
		}
		uiCfgFlagArmed = 0;

		ClearRedLED();
//		PORTA &= ~0x01;
	}
	else
	{
//		SetRedLED();
		PORTA |= 0x01;

		uiCfgFlagArmed = 1;


	}

}


//-----------------------------------------------------------------------------------------
//		EnableExternalInt
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine initialises external interrupr for PCINT2
//
//	Parameters:		none
//
//	Return:			none
//
// 	Author:			Dr. Andreas Schramm
//
//	Date;Changes:	2011-07-20; Initial   	
//
//-----------------------------------------------------------------------------------------
void EnableExternalInt(void)
{
	
	// Mask the interrupt pin
	PCMSK0 = (1<<PCINT2);

	// Enable interrupt
	GICR = (1<<PCIE0);

}

//-----------------------------------------------------------------------------------------
//		Init_IOs
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine initialises the respective IOs
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
void Init_IOs(void)
{

	// PA0 und PA1 als Ausgang
	DDRA |= (1<<PA1) | (1<<PA0);//0x03; 

    // PA2 als Eingang
    DDRA &= ~(1 << PA2);

	// Set internal Pullups
	//PORTA |= (1<<PA2) | (1<<PA1) | (1<<PA0);
	PORTA |= (1<<PA1) | (1<<PA0);

	// Set PortD Pin 0 as Input
	DDRD &= ~(1<<PD0);
	// Set internal Pullups
	PORTD |= (1<<PD0) | (1<<PD1);


    // PD5 zunächst als Eingang (Tristate)
    DDRD &= ~(1 << PD5);
    PORTD &= ~(1 << PD5);


}


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



//-----------------------------------------------------------------------------------------
//		InitBlinkingTimer
//-----------------------------------------------------------------------------------------
//
//	Description:	This routine initialises Timer0 that serves as blinking timer for the 
//					leds
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
void Timer0_Init(void)
{
    // 16MHz /64 = 250kHz 250 Takte =1ms OCR0=249

    TCCR0=
        (1<<WGM01)|   // CTC
        (1<<CS01) |
        (1<<CS00);    // /64

    OCR0=249;

    TIMSK |=
        (1<<OCIE0);
}

/****************************************
 Timer Interrupt
****************************************/
ISR(TIMER0_COMP_vect)
{
    if(led0_timer)
    {
        led0_timer--;

        if(!led0_timer)
        {
            PORTA &=
                ~(1<<PA0);
        }
    }

    if(led1_timer)
    {
        led1_timer--;

        if(!led1_timer)
        {
            PORTA &=
                ~(1<<PA1);
        }
    }
}

void UART0_Enable(void)
{
    UCSR0B=
        (1<<RXEN0)|
        (1<<TXEN0)|
        (1<<RXCIE0);
}

void UART0_Disable(void)
{
    UCSR0B=0;
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

	uint8_t lastState=0xFF;

	uint8_t bridgeEnabled=0;

	uint8_t pa2 = 0;

	// Disable all interrupts
	cli();

	// Initialize the respective IOs
	Init_IOs();

	ClearRedLED();
	ClearGreenLED();

	// Init timer 0 for led blinking support
	Timer0_Init();
	

  	/*
     *  Initialize UART library, pass baudrate and AVR cpu clock
     *  with the macro 
     *  UART_BAUD_SELECT() (normal speed mode )
     *  or 
     *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
     */
	Baudrate_COM1.ulBaudrate=DEFAULT_COM1_BAUDRATE;	
    uart_init( UART_BAUD_SELECT(Baudrate_COM1.ulBaudrate,F_CPU) ); 
    
	Baudrate_COM2.ulBaudrate=DEFAULT_COM2_BAUDRATE;	
	uart1_init( UART_BAUD_SELECT(Baudrate_COM2.ulBaudrate,F_CPU) ); 


	// Enable all interrupts 
    sei();
  

  	while(1)
  	{
 
		// Get current status of DTR line
	    pa2 = (PINA&(1<<PA2));

        // service PA2 status - DTR line
        if(pa2!=lastState)
        {
            lastState=pa2;

            if(pa2)
            {
                bridgeEnabled=1;
                
				buffers_clear();

                UART0_Enable();
				
                // PD5 Tristate
                DDRD &= ~(1<<PD5);
                PORTD &= ~(1<<PD5);

            }
            else
            {
                bridgeEnabled=0;

                UART0_Disable();

                // PD5 LOW
                PORTD &= ~(1<<PD5);
                DDRD |= (1<<PD5);
            }
        }
		
		// Service UARTs only if bridge is not armed
		if(bridgeEnabled == 1)
		{
			Service_UART0();
			Service_UART1();
		}
	
  }

  return 0;
}

 


