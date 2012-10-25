#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000
#include <util/delay.h>

#include "debounce.h"

#define SEC 0
#define MIN 1
#define HUR 2

#define STATE_CLOCK 0
#define STATE_SET_CLOCK_MIN 1
#define STATE_SET_CLOCK_SEC 2

volatile uint8_t sec_flag = 0;
volatile uint8_t milli_flag = 0;

void sec_timer(void);
void millisec_timer( void );
void display( uint8_t * time, uint8_t disp_mask );

int main( void )
{
	uint8_t button_state[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t  disp_flag = 0, mask_flag = 0, debounce_flag = 0;
	uint8_t state = 0, disp_mask = 0xFF, btn_hold_update_flag = 0;
	uint8_t time[3] = { 0, 0, 0 };

        DDRB = 0xFF;
	PORTB = 0xFF;

	DDRD = 0xFF;
	PORTD = 0x00;

	DDRC = 0x00;
	PORTC = 0xFF;

	sec_timer();
	millisec_timer();
	sei();

	while( 1 )
	{
		if( milli_flag )
		{
			milli_flag = 0;

			mask_flag++;
			disp_flag++;
			debounce_flag++;
			btn_hold_update_flag++;
		}

		if( mask_flag == 250 )
		{
			mask_flag = 0;

			if( state == STATE_SET_CLOCK_MIN )
			{
				disp_mask = ~(disp_mask&0x03);
			}
			else if( state == STATE_SET_CLOCK_SEC )
			{
				disp_mask = ~(disp_mask&0x0C);
			}
			else
			{
				disp_mask = 0xFF;
			}
		}

	
		if( sec_flag )
		{
			sec_flag = 0;
		
			if( state == STATE_CLOCK )
			{
				time[SEC]++;			
				if( time[SEC] == 60 )
				{
					time[SEC] = 0;
					time[MIN]++;
				}

				if( time[MIN] == 60 ) time[MIN] = 0;
			}
		}

		if( disp_flag == 1 )
		{
			disp_flag = 0;
			display( time, disp_mask );
		}

		if( debounce_flag == 10 )
		{
			debounce_flag = 0;
			debounce( &button_state );
		}
		
		//HANDLE INPUT AND STATES
		
		//MODE BUTTON
		if( button_state[0] == PRESS || button_state[0] == HOLD )
		{
			switch(state)
			{
				case STATE_CLOCK:
					if( button_state[0] == HOLD )
					{
						state = STATE_SET_CLOCK_MIN;
					}
					break;
				case STATE_SET_CLOCK_MIN:
					state = STATE_SET_CLOCK_SEC;
					break;
				case STATE_SET_CLOCK_SEC:
					state = STATE_CLOCK;
					break;
				
			}
			button_state[0] = UP;
		}

		//UP BUTTON
		if( button_state[1] == PRESS )
		{
			switch( state )
			{
				case STATE_SET_CLOCK_MIN:
					time[MIN]++;
					if( time[MIN] == 60 ) time[MIN] = 0;
					break;
				case STATE_SET_CLOCK_SEC:
					time[SEC]++;
					if( time[SEC] == 60 ) time[SEC] = 0;
					break;
	

			}
			button_state[1] = UP;
		}

		if( button_state[1] == HOLD )
		{
			switch( state )
			{
				case STATE_SET_CLOCK_MIN:
					if( btn_hold_update_flag == 100 )
					{
						btn_hold_update_flag = 0;
						time[MIN]++;
						if( time[MIN] == 60 ) time[MIN] = 0;
					}
					break;
				case STATE_SET_CLOCK_SEC:
					if( btn_hold_update_flag == 100 )
					{
						btn_hold_update_flag = 0;
						time[SEC]++;
						if( time[SEC] == 60 ) time[SEC] = 0;
					}
					break;	

			}
		}

		//DOWN BUTTON
		if( button_state[2] == PRESS )
		{
			switch( state )
			{
				case STATE_SET_CLOCK_MIN:
					if( time[MIN] == 0 ) time[MIN] = 60;
					time[MIN]--;
					break;
				case STATE_SET_CLOCK_SEC:
					if( time[SEC] == 0 ) time[SEC] = 60;
					time[SEC]--;
					break;
	

			}
			button_state[2] = UP;
		}

		if( button_state[2] == HOLD )
		{
			switch( state )
			{
				case STATE_SET_CLOCK_MIN:
					if( btn_hold_update_flag == 100 )
					{
						btn_hold_update_flag = 0;
						if( time[MIN] == 0 ) time[MIN] = 60;
						time[MIN]--;
					}
					break;
				case STATE_SET_CLOCK_SEC:
					if( btn_hold_update_flag == 100 )
					{
						btn_hold_update_flag = 0;
						if( time[SEC] == 0 ) time[SEC] = 60;
						time[SEC]--;
					}
					break;	
			}
		}
	}
}


void display( uint8_t * time, uint8_t disp_mask )
{
	const uint8_t seg[] = { 0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18 };
	static uint8_t cur_digit = 0;

	PORTB = 0x00;
	//MINS
	switch( cur_digit )
	{
		case 0:
			PORTD = (time[MIN]>9)?seg[ (time[MIN]/10) ]:seg[0];
			break;
		case 1:
			PORTD = (time[MIN]>9)?seg[ (time[MIN]%10) ]:seg[time[MIN]];
			break;

		case 2:		
			PORTD = (time[SEC]>9)?seg[ (time[SEC]/10) ]:seg[0];
			break;
		case 3:
			PORTD = (time[SEC]>9)?seg[ (time[SEC]%10) ]:seg[time[SEC]];
			break;
	}
	PORTB = (1<<cur_digit) & disp_mask;

	cur_digit++;
	if( cur_digit == 4 ) cur_digit = 0;
}

//Timer2 init acording datasheet
void sec_timer(void)
{
    //Disable timer2 interrupts
   // TIMSK2  = 0;
    //Enable asynchronous mode
    ASSR  = (1<<AS2);
    //set initial counter value
    //TCNT2=0;
    //set prescaller 128
    TCCR2B |= (1<<CS22)|(1<<CS00);
    //wait for registers update
  //  while (!(ASSR & ((1<<TCN2UB)|(1<<TCR2BUB))));
    //clear interrupt flags
    //TIFR2  = (1<<TOV2);
    //enable TOV2 interrupt
    TIMSK2  = (1<<TOIE2);
}

void millisec_timer()
{
        //interrupt when timer0 equals OCR0A
        TIMSK0 = (1<<OCIE0A);

        //clear on pos compare with OCRA0A (CTC mode)
        TCCR0A = (1<<WGM01);

        //use system clock, prescale by 8
        TCCR0B = (1<<CS01);

        //timer match value - 1 sec interval
        OCR0A = 125;
}

//Overflow ISR
ISR(TIMER2_OVF_vect)
{
	sec_flag = 1;
}

ISR( TIMER0_COMPA_vect )
{
	milli_flag = 1;
}




