#include <msp430.h>
#include "CTS_Layer.h"

// Timer
unsigned long timer[3];

// Accelerometer Step
#define STEP	30
#define THRESH	80
//#define OFF_X	+20		// UTEP
//#define OFF_Y	0		// UTEP
//#define OFF_Z	+50		// UTEP
#define OFF_X	+20		// NASA
#define OFF_Y	0		// NASA
#define OFF_Z	+50		// NASA

void accInit() {
	// Connect inputs to ADC
	P1SEL |= BIT5 + BIT6 + BIT7;
	// Configure ADC
	ADC10CTL1 = INCH_7 + CONSEQ_1;	// A7/A6/A5, Multi-Channel Once
	ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON + ADC10IE;
	ADC10AE0 = BIT5 + BIT6 + BIT7;
	ADC10DTC1 = 3;					// 3 Conversions
}

void accRead(int * buffer) {
	ADC10CTL0 &= ~ENC;					// Disable ADC
	while (ADC10CTL1 & BUSY);			// Wait if ADC10 is active
	ADC10SA = (unsigned int) buffer;	// Data buffer
	ADC10CTL0 |= ENC + ADC10SC;			// Sampling and conversion ready
	__bis_SR_register(CPUOFF + GIE);	// LPM0
	buffer[0] -= 512 - OFF_X;
	buffer[1] -= 512 - OFF_Y;
	buffer[2] -= 512 - OFF_Z;
}

void wait(unsigned int ms) {
	unsigned temp = timer[0];
	while (timer[0] < temp + ms);
}

void ledInit() {
	P1OUT &= ~(BIT1 + BIT2 + BIT3 + BIT4);
	P1DIR |= BIT1 + BIT2 + BIT3 + BIT4;
}

void ledOn(int led) {
	switch (led) {
	case -1:
		P1OUT |= BIT1 + BIT2 + BIT3 + BIT4;
		break;
	case 0:
		P1OUT |= BIT1;
		break;
	case 1:
		P1OUT |= BIT2;
		break;
	case 2:
		P1OUT |= BIT3;
		break;
	case 3:
		P1OUT |= BIT4;
		break;
	}
}

void ledOff(int led) {
	switch (led) {
	case -1:
		P1OUT &= ~(BIT1 + BIT2 + BIT3 + BIT4);
		break;
	case 0:
		P1OUT &= ~(BIT1);
		break;
	case 1:
		P1OUT &= ~(BIT2);
		break;
	case 2:
		P1OUT &= ~(BIT3);
		break;
	case 3:
		P1OUT &= ~(BIT4);
		break;
	}
}

uint8_t readButton() {
	uint8_t read, temp;
	temp = P1OUT;
	P1OUT = 0;
	read = TI_CAPT_Button(&touchpad);
	P1OUT = temp;
	//wait(10);
	return read;
}

int main() {

    // Initialize System Clocks
	WDTCTL = WDTPW + WDTHOLD;            	 	// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;               	 	// Set DCO to 1, 8, 12 or 16MHz
	DCOCTL = CALDCO_1MHZ;
	BCSCTL2 |= DIVS_0;                   		// divide SMCLK by 4 for 250khz
	BCSCTL3 |= LFXT1S_2;                 		// LFXT1 = VLO

	// Accelerometer
	accInit();

	// Initialize baseline measurement
	TI_CAPT_Init_Baseline(&touchpad);
	TI_CAPT_Update_Baseline(&touchpad, 25);

	// Timer 1 (PWM)
	TA1CCTL0 = CM_3 | CCIS_0 | OUTMOD_0 | CCIE;	// Compare mode
	TA1CCR0 = 500;								// (1MHz / 1000 = 1000Hz)
	TA1CTL = TASSEL_2 | ID_0 | MC_1;			// SMCLK, DIV1, Up Mode

	// Interrupts
	__bis_SR_register(GIE);

	// State machine
	enum State {
		WAIT_FRONT,
		WAIT_BACK,
		WAIT_LEFT,
		WAIT_RIGHT,
		SENSING
	} state = WAIT_FRONT;

	// Accelerometer
	int acc[3];

	// LEDs
	ledInit();

	while(1) {
		// Accelerometer and touch
		if (timer[0] > 100) {
			timer[0] = 0;
			accRead(acc);
		}


		// State Machine
		switch (state) {
		case WAIT_FRONT:
			if (acc[1] > THRESH) {
				//ledOn(-1);
				wait(10);
				ledOff(-1);
				wait(950);
				state = WAIT_BACK;
				timer[1] = 0;
			}
			break;

		case WAIT_BACK:
			if (acc[1] < -THRESH) {
				//ledOn(-1);
				wait(10);
				ledOff(-1);
				wait(950);
				state = WAIT_LEFT;
				timer[1] = 0;
				break;
			}
			if (timer[1] > 3000) {
				state = WAIT_FRONT;
				break;
			}
			break;

		case WAIT_LEFT:
			if (acc[0] > THRESH) {
				//ledOn(-1);
				wait(10);
				ledOff(-1);
				wait(950);
				state = WAIT_RIGHT;
				timer[1] = 0;
				break;
			}
			if (timer[1] > 3000) {
				state = WAIT_FRONT;
				break;
			}
			break;

		case WAIT_RIGHT:
			if (acc[0] < -THRESH) {
				//ledOn(-1);
				wait(10);
				ledOff(-1);
				wait(950);
				state = SENSING;
				timer[1] = 0;
				break;
			}
			if (timer[1] > 3000) {
				state = WAIT_FRONT;
				break;
			}
			break;

		case SENSING:
			// Turn off after 30s
			if (timer[1] > 30000) {
				ledOff(-1);
				state = WAIT_FRONT;
				break;
			}

			// Move LEDs according to accelerometer
			if (abs(acc[0]) < STEP) {
				// If flat, play animation
				static int led = 0;
				static int direction = 1;

				if (led < 1) direction = +1;
				if (led > 2) direction = -1;
				ledOff(-1);
				ledOn(led);

				led += direction;
				wait(333);
			} else {
				// If tilted, display on LEDs
				ledOff(-1);
				if (acc[0] > STEP)	  ledOn(1);
				if (acc[0] > STEP*2)  ledOn(0);
				if (acc[0] < -STEP)   ledOn(2);
				if (acc[0] < -STEP*2) ledOn(3);
				wait(100);
			}
			break;
		}
	}

	return 0;
}

/*
 * Timer 1 Interrupt
 * 1000Hz
 *
 * PWM
 */
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR_HOOK(void) {
	// Timer
	timer[0]++;
	timer[1]++;
	timer[2]++;
}

/*
 * ADC10 ISR
 */
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR() {
	__bic_SR_register_on_exit(CPUOFF);
}
