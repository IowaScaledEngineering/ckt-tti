/*************************************************************************
Title:    CKT-BD1 Single Channel Block Detector
Authors:  Michael Petersen <railfan@drgw.net>
          Nathan D. Holmes <maverick@drgw.net>
File:     $Id: $
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2016 Michael Petersen & Nathan Holmes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <stdlib.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

volatile uint8_t eventFlags = 0;
#define EVENT_INPUT_READ  0x01

void initialize50HzTimer(void)
{
	// Set up timer 0 for 100Hz interrupts
	TCNT0 = 0;
	OCR0A = 0x9B;  // 8MHz / 1024 / 0x9B = 50Hz
	TCCR0A = _BV(WGM01);
	TCCR0B = _BV(CS02) | _BV(CS00);  // 1024 prescaler
	TIMSK |= _BV(OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
	eventFlags |= EVENT_INPUT_READ;
}

void init(void)
{
	MCUSR = 0;
	wdt_reset();
	wdt_enable(WDTO_250MS);
	wdt_reset();

	// Pin Assignments for PORTB/DDRB
	//  PB0 - MOSI (unused, output)
	//  PB1 - Invert output (input, pullup)
	//  PB2 - SCK (unused, output)
	//  PB3 - Touch Trigger input (input)
	//  PB4 - Control line (output)
	//  PB5 - Not used
	//  PB6 - Not used
	//  PB7 - Not used
	DDRB  = 0b00010101;
	PORTB = 0b00000010;

	initialize50HzTimer();
}

inline void setOutputLow()
{
	PORTB &= ~_BV(PB4);
}

inline void setOutputHigh()
{
	PORTB |= _BV(PB4);
}

inline bool isInverted()
{
	return ( 0 == (PINB & _BV(PB1))?true:false );

}

int main(void)
{
	uint8_t lastStatus = _BV(PB3);
	uint8_t currentOutput = 1;
	// Deal with watchdog first thing
	MCUSR = 0;					// Clear reset status
	init();

	sei();

	while(1)
	{
		wdt_reset();
		
		if (eventFlags & EVENT_INPUT_READ)
		{
			eventFlags &= ~(EVENT_INPUT_READ);
			DDRB &= ~_BV(PB3); // Set the touch trigger line to an input
			_delay_ms(2);

			uint8_t newStatus = (PINB & _BV(PB3));
			if (newStatus != lastStatus)
			{
				if (0 == newStatus)
					currentOutput ^= 0x01;
				lastStatus = newStatus;
			}
			
			if (0 == currentOutput)
			{
				DDRB |= _BV(PB3); // Set the touch trigger line back to an output
				PORTB &= ~_BV(PB3);
			}

			if (isInverted())
			{
				if(currentOutput)
					setOutputHigh();
				else
					setOutputLow();
			
			} else {
				if(0 == currentOutput)
					setOutputHigh();
				else
					setOutputLow();
			}
			
		
		}

	}
}

