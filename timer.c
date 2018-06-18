//***************************************************************************************
// MSP430 Wake Up timer for Solo Sailing
//
// Description: 
//	5 LEDs indicate how much time is remaining
//  When time's up, the BUZZER rings
//  A push button is used to stop the timer and add 5 minutes to the countdown
//  A long push (5 sec) on the button triggers entry into sleep mode
// 
// MSP430x2xx
// -----------------
// 
//***************************************************************************************
#include <msp430g2231.h>

// Real values
/*
#define LED_0  BIT0
#define LED_1  BIT1
#define LED_2  BIT2
#define LED_3  BIT3
#define LED_4  BIT4
#define BUTTON BIT5
#define BUZZER BIT6
*/

// Test values --> for use on the USB debug platform
#define LED_0  BIT0
#define LED_1  BIT1
#define LED_2  BIT2
#define LED_3  BIT4
#define LED_4  BIT5
#define BUTTON BIT3
#define BUZZER BIT6

#define LED_OUT P1OUT
#define LED_DIR P1DIR

unsigned int minutes_to_half_sec = 5; // Test : 5 // Real : 120
unsigned int timer_minutes = 0;
unsigned int timer_half_second = 0; 
unsigned char debounce = 0;
unsigned int button_delay = 0;
unsigned int mode = 0 ;

// Two modes : 0 --> Sleep, wake on button pressed
// 1 --> Active, sleep on long press

void main(void)
{

	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer

	BCSCTL1 = CALBC1_1MHZ;		// Running at 1 MHz
	DCOCTL = CALDCO_1MHZ;

	TACCR0 = 1249           // With the Timer using SMCLK div 8 (125 kHz), this
							// value gives a frequency of 125000/(TACCR0+1) Hz.
							// For TACCR0 = 1249, that's 100 Hz.
							
	TACCTL0 = CCIE;         // Enable interrupts for CCR0.
	TACTL = TASSEL_2 + ID_3 + MC_1 + TACLR;  
							// SMCLK, div 8, up mode, clear timer.
	
	debounce = 0; 			// Reset button debounce
	button_delay = 0; 		// Used to wait for long push of the button 
	mode = 0; 				// Starts in sleep mode
	timer_minutes = 5*minutes_to_half_sec; 
							// We start with 5 min in the timer


	LED_DIR |= (LED_0 + LED_1 + LED_2 + LED_3 + LED_4 + BUZZER);
							// Set ports to output direction

	LED_OUT &= ~(LED_0 + LED_1 + LED_2 + LED_3 + LED_4 + BUZZER);
							// Set the LEDs and buzzer off

	P1IE |= BUTTON;
							// Set interrupt on Button change

	_BIS_SR(LPM1_bits + GIE);	
							// Enter LPM0 and enable interrupts


} // end main


#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void) {

debounce ? debounce-- : 1;
				// we wait 8 heartbeats (40 ms) before accepting a new button interrupt

if (mode == 1) // Active mode
{
	if (timer_minutes > 0) // The time is not over
	{
	       P1OUT &= ~BUZZER; // Buzzer OFF
	
	       timer_half_second = (timer_half_second + 1) % 50; // running at 100 Hz --> 0.5s = 50 beats
	       if (timer_half_second <= 10 ) // the leds are ON only part of the time
	       {
               if (timer_minutes <= 5*minutes_to_half_sec) P1OUT |= LED_0;
               else if(timer_minutes<=10*minutes_to_half_sec) P1OUT |= LED_0 + LED_1;
               else if(timer_minutes<=15*minutes_to_half_sec) P1OUT |= LED_0 + LED_1 + LED_2;
               else if(timer_minutes<=20*minutes_to_half_sec) P1OUT |= LED_0 + LED_1 + LED_2 + LED_3;
               else if(timer_minutes>20*minutes_to_half_sec)  P1OUT |= LED_0 + LED_1 + LED_2 + LED_3 + LED_4;
	       }
	       else 
		   	   P1OUT &= ~(LED_0+ LED_1 + LED_2 + LED_3 + LED_4);
	       
		   if (timer_half_second==0) timer_minutes = timer_minutes - 1; // we remove half a second
	}
	
	else // time is over
	
    {
           P1OUT &= ~(LED_0+ LED_1 + LED_2 + LED_3 + LED_4);
           timer_half_second = (timer_half_second + 1) % 50;               
		   if (timer_half_second <= 25 ) 
		   		P1OUT |= BUZZER;      
			else
		   		P1OUT &= ~BUZZER;      
    }

} // end mode==1


// long push on the button --> sleep mode

if (P1IN & BUTTON)
	{
		button_delay++;
		P1OUT |= BUZZER;
		if (button_delay == 10) // we are waiting several heartbeats.
		{
			mode = 0;  // entering sleep mode
			// Set the LEDs and buzzer off
			LED_OUT &= ~(LED_0 + LED_1 + LED_2 + LED_3 + LED_4 + BUZZER);
		}
	}
else
	{
		button_delay = 0; // button is released, we reset the delay for sleep mode
	}

} // end Timer_A


// Port 1 interrupt service routine

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
if (!debounce) 
	{
	debounce = 8;
	
	mode = 1; // Switch to active mode on button pressed

	P1OUT |= BUZZER; // Beep to confirm button pressed (buzzer will be shut down at next heartbeat
	
	// we add 5 minutes to the timer
	timer_minutes = timer_minutes + 5 * minutes_to_half_sec;
	if(timer_minutes<=5*minutes_to_half_sec) P1OUT |= LED_0;
       else if(timer_minutes<=10*minutes_to_half_sec) P1OUT |= LED_0 + LED_1;
       else if(timer_minutes<=15*minutes_to_half_sec) P1OUT |= LED_0 + LED_1 + LED_2;
       else if(timer_minutes<=20*minutes_to_half_sec) P1OUT |= LED_0 + LED_1 + LED_2 + LED_3;
       else if(timer_minutes>20*minutes_to_half_sec) P1OUT |= LED_0 + LED_1 + LED_2 + LED_3 + LED_4;
	
	}	

	P1IFG &= ~BUTTON; // Interrupt flag cleared

} // end interrupt Port1