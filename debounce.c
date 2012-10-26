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

#include "debounce.h"

//decides on button states while ensuring bounce has no effect
//pull up resistors used, low is a press
void debounce( uint8_t * button_states )
{
	int i;

	//number of consecutive ones
	static uint8_t button_high[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	
	//number of consecutive zeros
	static uint8_t button_low[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	//loop for each button
	for( i=0; i<8; i++ )
	{
		//if the pin is low (pressed down)
		if( ((DEBOUNCE_PIN)&(1<<i)) == (0x00) )
		{
			//increment consecutive lows
			if( button_low[i] < 255 ) button_low[i]++;
			
			//reset consecutive highs
			button_high[i] = 0;
		}
		else
		{ 
			//increment consecutive highs
			if( button_high[i] < 255) button_high[i]++;
			
			//reset consecutive lows
			button_low[i] = 0;
		}

		//if the number of consecutive lows has reached a threshold (see define)
		//then the button is down (not a press yet)
		if( button_low[i] == BTN_PRESS_THRESHOLD )
		{
			button_states[i] = DOWN;
		}

		//if the number of consecutive lows has reached a high threshold (see define)
		//then the button is being held down
		if( button_low[i] == BTN_HOLD_THRESHOLD )
		{
			button_states[i] = HOLD;
		}

		//if the number of consecutive highs has reached a threshold (see define)
		//then the button has been released
		if( button_high[i] == BTN_PRESS_THRESHOLD )
		{
			//if the button was previously down, then this is now a press
			//the state must be acknowledged and changed to UP to reset.
			if( button_states[i] == DOWN )
			{
				button_states[i] = PRESS;
			}

			//if the button was previously being held, then it is no longer
			//being held and is UP
			if( button_states[i] == HOLD )
			{
				button_states[i] = UP;
			}
		}
	}
}
