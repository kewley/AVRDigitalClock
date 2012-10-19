#include <avr/io.h>

#include "debounce.h"

void debounce( uint8_t (*bs)[8] )
{
	int i;

	static uint8_t button_high[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static uint8_t button_low[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	for( i=0; i<8; i++ )
	{
		if( ((DEBOUNCE_PIN)&(1<<i)) == (0x00) )
		{
			if( button_high[i] < 255 ) button_high[i]++;
			button_low[i] = 0;
		}
		else
		{ 
			if( button_low[i] < 255) button_low[i]++;
			button_high[i] = 0;
		}

		if( button_high[i] == 3 )
		{
			(*bs)[i] = DOWN;
		}
		if( button_high[i] == 100 )
		{
			(*bs)[i] = HOLD;
		}
		if( button_low[i] == 3 )
		{
			if( (*bs)[i] == DOWN )
			{
				(*bs)[i] = PRESS;
			}
			if( (*bs)[i] == HOLD )
			{
				(*bs)[i] = UP;
			}
		}
	}
}


