/*
 * Copyright 2012 Craig Kewley
 *
 * This file is part of AVRDigitalClock.
 *
 * AVRDigitalClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AVRDigitalClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AVRDigitalClock.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "debounce.h"

//for accessing time array
#define SEC 0
#define MIN 1
#define HUR 2

//states of the clock
#define STATE_HUR_MIN 0
#define STATE_MIN_SEC 1
#define STATE_HUR_MIN_SET_HUR 2
#define STATE_HUR_MIN_SET_MIN 3
#define STATE_MIN_SEC_SET_MIN 4
#define STATE_MIN_SEC_SET_SEC 5

//buttons
#define BTN_MODE 0
#define BTN_UP 1
#define BTN_DOWN 2

//number of milliseconds before button hold can be acted upon
//this is not the number of milliseconds before a hold is
//registered, see debounce.c
#define BTN_UPDATE_INTERVAL 100

//flags updated by timer interrupts
volatile uint8_t sec_flag = 0; 
volatile uint8_t milli_flag = 0;

//function prototypes
void sec_timer(void);
void millisec_timer( void );
void display( uint8_t digits_one, uint8_t digits_two, uint8_t disp_mask );
void handle_input( uint8_t * time, uint8_t * state, uint8_t * button_state, uint8_t * btn_hold_update_flag );

int main( void )
{
	//stores button states, see debounce.c
	uint8_t button_state[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	
	//stores the state of the clock, see the state defines above.
	uint8_t state = 0;

	//This can be changed to only display some of the digits.
	//Used to flash digits when setting the clock.
	uint8_t disp_mask = 0xFF;

	//stores the current {seconds, minutes, hours}. Use defines above.
	uint8_t time[3] = { 0, 0, 0 };


	//the following flags are updates when a millisecond is acknowledged.
	//when they reach a certain value a corresponding function is carried out.
	uint8_t disp_flag = 0; //update the seven segment display
	uint8_t mask_flag = 0; //change the mask (used to flash digits)
	uint8_t debounce_flag = 0; //run the debounce code, update button states
	uint8_t btn_hold_update_flag = 0; //slows down the effect of holding a button
	
	//PORTB controls which digit is currently active
	DDRB = 0xFF;
	PORTB = 0xFF;

	//PORTD controls which segments are active
	DDRD = 0xFF;
	PORTD = 0x00;

	//PORTC receives input from three buttons
	DDRC = 0x00;
	PORTC = 0xFF;

	//setup timers
	sec_timer();
	millisec_timer();

	//enable interrupts
	sei();

	//main loop
	while( 1 )
	{
		//acknowledge second flag
		if( sec_flag )
		{
			sec_flag = 0;

			//don't update if we're setting the time
			if( state == STATE_HUR_MIN || state == STATE_MIN_SEC )
			{
				time[SEC]++;			
				if( time[SEC] == 60 )
				{
					time[SEC] = 0;
					time[MIN]++;
				}

				if( time[MIN] == 60 )
				{
					time[MIN] = 0;
					time[HUR]++;
				}
			
				if( time[HUR] == 24 )
				{
					time[HUR] = 0;
				}
			}
		}

		//acknowledge millisecond flag
		if( milli_flag )
		{
			milli_flag = 0;

			//update all other flags that rely on millisecond count
			mask_flag++;
			disp_flag++;
			debounce_flag++;
			btn_hold_update_flag++;
		}

		//flash digits while updating
		if( mask_flag == 250 )
		{
			mask_flag = 0;

			switch( state )
			{
				//flash the digits on the left
				case STATE_HUR_MIN_SET_HUR:
				case STATE_MIN_SEC_SET_MIN:
					disp_mask = ~(disp_mask&0x03);
					break;

				//flash the digits on the right
				case STATE_HUR_MIN_SET_MIN:
				case STATE_MIN_SEC_SET_SEC:
					disp_mask = ~(disp_mask&0x0C);
					break;

				//else turn all on
				default:
					disp_mask = 0xFF;
			}
		}

		//acknowledge display-update flag
		if( disp_flag == 1 )
		{
			disp_flag = 0;
			
			//clock can display either hours/minutes or minutes/second
			//decide which based on current state
			switch( state )
			{
				case STATE_HUR_MIN:
				case STATE_HUR_MIN_SET_HUR:
				case STATE_HUR_MIN_SET_MIN:
					display( time[HUR], time[MIN], disp_mask );
					break;
				
				case STATE_MIN_SEC:
				case STATE_MIN_SEC_SET_MIN:
				case STATE_MIN_SEC_SET_SEC:
					display( time[MIN], time[SEC], disp_mask );
					break;
			}
		}

		//acknowledge debounce flag
		//call debounce function
		if( debounce_flag == 10 )
		{
			debounce_flag = 0;

			//pass button states
			debounce( button_state );
		}

		//handle button events, change state		
		handle_input( time, &state, button_state, &btn_hold_update_flag );	
	}
}


//handles button events from the three buttons
void handle_input( uint8_t * time, uint8_t * state, uint8_t * button_state, uint8_t * btn_hold_update_flag )
{
		
		//handle mode button
		//if pressed then switch between hour-minutes minutes-seconds display
		//if held, switch to set time mode, pressing again moves to next digits
		//press again to go back to the previous time mode.
		if( button_state[BTN_MODE] == PRESS || button_state[BTN_MODE] == HOLD )
		{
			switch(*state)
			{
				case STATE_HUR_MIN:
					if( button_state[0] == HOLD )
					{
						(*state) = STATE_HUR_MIN_SET_HUR;
					}
					else if( button_state[0] == PRESS )
					{
						(*state) = STATE_MIN_SEC;
					}
					break;
				case STATE_MIN_SEC:
					if( button_state[0] == HOLD )
					{
						(*state) = STATE_MIN_SEC_SET_MIN;
					}
					else if( button_state[0] == PRESS )
					{
						(*state) = STATE_HUR_MIN;
					}
					break;
				case STATE_HUR_MIN_SET_HUR:
					(*state) = STATE_HUR_MIN_SET_MIN;
					break;
				case STATE_HUR_MIN_SET_MIN:
					(*state) = STATE_HUR_MIN;
					break;
				case STATE_MIN_SEC_SET_MIN:
					(*state) = STATE_MIN_SEC_SET_SEC;
					break;
				case STATE_MIN_SEC_SET_SEC:
					(*state) = STATE_MIN_SEC;
					break;
			}
			button_state[BTN_MODE] = UP;
		}

		//handle up button press
		//used to move time digits up
		if( button_state[BTN_UP] == PRESS )
		{
			switch( *state )
			{
				case STATE_HUR_MIN_SET_HUR:
					time[HUR]++;
					if( time[HUR] == 24) time[HUR] = 0;
					break;

				case STATE_HUR_MIN_SET_MIN:
				case STATE_MIN_SEC_SET_MIN:
					time[MIN]++;
					if( time[MIN] == 60 ) time[MIN] = 0;
					break;

				case STATE_MIN_SEC_SET_SEC:
					time[SEC]++;
					if( time[SEC] == 60 ) time[SEC] = 0;
					break;
	

			}
			button_state[BTN_UP] = UP;
		}

		//handle up button hold
		//same as press only keeps going until released
		//only updates once per BTN_UPDATE_INTERVAL otherwise it changes too fast
		if( button_state[BTN_UP] == HOLD )
		{
			switch( *state )
			{
				case STATE_HUR_MIN_SET_HUR:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						time[HUR]++;
						if( time[HUR] == 24) time[HUR] = 0;
					}
					break;

				case STATE_HUR_MIN_SET_MIN:
				case STATE_MIN_SEC_SET_MIN:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						time[MIN]++;
						if( time[MIN] == 60 ) time[MIN] = 0;
					}
					break;

				case STATE_MIN_SEC_SET_SEC:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						time[SEC]++;
						if( time[SEC] == 60 ) time[SEC] = 0;
					}
					break;
			}
		}
		
		//handle down button press
		//same as up...only this one decrements
		if( button_state[BTN_DOWN] == PRESS )
		{
			switch( *state )
			{
				case STATE_HUR_MIN_SET_HUR:
					if( time[HUR] == 0 ) time[HUR] = 24;
					time[HUR]--;
					break;

				case STATE_HUR_MIN_SET_MIN:
				case STATE_MIN_SEC_SET_MIN:
					if( time[MIN] == 0 ) time[MIN] = 60;
					time[MIN]--;
					break;
				case STATE_MIN_SEC_SET_SEC:
					if( time[SEC] == 0 ) time[SEC] = 60;
					time[SEC]--;
					break;
			}
			button_state[BTN_DOWN] = UP;
		}

		//handle down button hold
		//same as up button hold, only decrements
		if( button_state[BTN_DOWN] == HOLD )
		{
			switch( *state )
			{
				case STATE_HUR_MIN_SET_HUR:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						if( time[HUR] == 0 ) time[HUR] = 24;
						time[HUR]--;
					}
					break;

				case STATE_HUR_MIN_SET_MIN:
				case STATE_MIN_SEC_SET_MIN:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						if( time[MIN] == 0 ) time[MIN] = 60;
						time[MIN]--;
					}
					break;
				case STATE_MIN_SEC_SET_SEC:
					if( (*btn_hold_update_flag) == BTN_UPDATE_INTERVAL )
					{
						(*btn_hold_update_flag) = 0;
						if( time[SEC] == 0 ) time[SEC] = 60;
						time[SEC]--;
					}
					break;
			}
		}
}

//display the next digit on the seven segment display
//displays only one digit per call so no time is wasted with delays
void display( uint8_t digits_one, uint8_t digits_two, uint8_t disp_mask )
{
	//hex values for digits
	const uint8_t seg[] = { 0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18 };
	
	//current digit, incremented below
	static uint8_t cur_digit = 0;

	//turn all digits off
	PORTB = 0x00;

	//splits double digits into single digits
	switch( cur_digit )
	{
		case 0:
			PORTD = (digits_one>9) ? seg[(digits_one/10)] : seg[0];
			break;
		case 1:
			PORTD = (digits_one>9)?seg[ (digits_one%10) ]:seg[digits_one];
			break;
		case 2:		
			PORTD = (digits_two>9)?seg[ (digits_two/10) ]:seg[0];
			break;
		case 3:
			PORTD = (digits_two>9)?seg[ (digits_two%10) ]:seg[digits_two];
			break;
	}
	
	//turn digit one (as long as it's on in the display mask)
	PORTB = (1<<cur_digit) & disp_mask;

	//move to next digit
	cur_digit++;
	if( cur_digit == 4 ) cur_digit = 0;
}

//setup second interval timer
void sec_timer(void)
{
	//external oscillator from TOSC1
	ASSR  = (1<<AS2);

	//prescale by 128
	TCCR2B |= (1<<CS22)|(1<<CS00);
    
	//clear on overflow
    	TIFR2  = (1<<TOV2);

	//enable TOV2 interrupt
	TIMSK2  = (1<<TOIE2);
}

//setup millisecond interval timer
void millisec_timer()
{
        //interrupt when timer0 equals OCR0A
        TIMSK0 = (1<<OCIE0A);

        //clear on pos compare with OCRA0A (CTC mode)
        TCCR0A = (1<<WGM01);

        //use system clock, prescale by 8
        TCCR0B = (1<<CS01);

        //timer match value
        OCR0A = 125;
}

//second interrupt
ISR(TIMER2_OVF_vect)
{
	sec_flag = 1;
}

//millisecond interrupt
ISR( TIMER0_COMPA_vect )
{
	milli_flag = 1;
}




