#include <msp430.h>

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //S1 P1.7
    P1DIR &= ~BIT7; //work direction input
    P1REN |= BIT7;  //set up pull-up resistor
    P1OUT |= BIT7;  //set initial value
    P1IE |= BIT7;   //allow interrupt
    P1IES |= BIT7;  //interrupt by down
    P1IFG &= ~BIT7; //clear interrupt the flag

    //S2 P2.2
    P2DIR &= ~BIT2;
    P2REN |= BIT2;
    P2OUT |= BIT2;
    P2IE |= BIT2;
    P2IES |= BIT2;
    P2IFG &= ~BIT2;

    //LED4
    P1DIR |= BIT1;
    P1OUT &= ~BIT1;

    //LED5
    P1DIR |= BIT2;
    P1OUT &= ~BIT2;

    //oscilloscope
    P7DIR |= BIT7;
    P7SEL |= BIT7;

    //frequency
    UCSCTL1 = DCORSEL_0;        //choose range
    UCSCTL2 = 2;                //FLLN
    UCSCTL2 |= FLLD__1;         //FLLD = 1
    UCSCTL3 = SELREF__XT1CLK;   //choose source for FLL
    UCSCTL3 |= FLLREFDIV__1;    //FLLREFDIV = 1
    UCSCTL4 = SELM__DCOCLK;     //choose source for MCLK
    UCSCTL5 = DIVM__8;          //div for MCLK
//    UCSCTL5 = DIVM__16;

    _bis_SR_register(GIE);      //global allow of interruption

    __no_operation();
}

volatile int short FREQUENCY_MODE = 0;
volatile int short LOW_POWER_MODE = 0;
volatile int short INTERRUPT_DELAY = 1000;

#pragma vector = PORT1_VECTOR
__interrupt void S1(void) {
    volatile int i;
    volatile int k = 1;

    for (i = 0; i < INTERRUPT_DELAY; i++) {
        k++;
    }

    if (LOW_POWER_MODE) {
        LOW_POWER_MODE = 0;
        P1OUT &= ~BIT1;
        _bic_SR_register_on_exit(LPM3_bits);//bit mask
    } else {
        LOW_POWER_MODE = 1;
        P1OUT |= BIT1;
        _bis_SR_register_on_exit(LPM3_bits);//no bit mask

    }

    P1IFG &= ~BIT7;
}

#pragma vector = PORT2_VECTOR
__interrupt void S2(void) {
    volatile int i;
    volatile int k = 1;

    for (i = 0; i < INTERRUPT_DELAY; i++) {
        k++;
    }

    if (FREQUENCY_MODE) {
        UCSCTL4 = SELM__DCOCLK;  //choose source for MCLK
        UCSCTL5 = DIVM__8;
        P1OUT &= ~BIT2;
        FREQUENCY_MODE = 0;
    } else {
        UCSCTL4 = SELM__REFOCLK; //choose source for MCLK
        UCSCTL5 = DIVM__2;
        P1OUT |= BIT2;
        FREQUENCY_MODE = 1;
    }

    P2IFG &= ~BIT2;
}
