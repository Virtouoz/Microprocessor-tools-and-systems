#include <msp430.h>

void changeTimer();
void confTimerA1();
void action();

#define WATCHDOG 5
#define TA1 6

int Timer = TA1;

void turnOn()
{
    P1OUT |= BIT0;
    P8OUT |= BIT1;
    P8OUT |= BIT2;
}

void turnoff()
{
    P1OUT &= ~BIT0;
    P8OUT &= ~BIT1;
    P8OUT &= ~BIT2;
}

int StatusS1()
{
    return (P1IN & BIT7) == 0;
}

int StatusS2()
{
    return (P2IN & BIT2) == 0;
}

void changeTimer()
{
    if (Timer == TA1)
    {
        P1OUT |= BIT1;
        Timer = WATCHDOG;
        TA1CCTL0 &= ~CCIE;
        TA1CTL = TACLR | MC__STOP;
    }
    else
    {
        P1OUT &= ~BIT1;
        Timer = TA1;
        SFRIE1 &= ~WDTIE;
        WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;
        confTimerA1();
    }
}

#pragma vector = TIMER1_A1_VECTOR
__interrupt void ta1_12_isr (void){
    if (TA1IV & TA1IV_TACCR1)
    {
        P8OUT &= ~BIT1;
    }
    else if (TA1IV & TA1IV_TACCR2)
    {
        P8OUT &= ~BIT2;
    }
    else
    {
        P1OUT &= ~BIT0;
    }
    TA1IV = 0;
}

void stopTimerA1()
{
    TA1CTL &= ~MC__UP;
    //TA1CTL = MC__STOP;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void ta1_0_isr (void){
    P8OUT &= ~BIT2;
    stopTimerA1();
}

void confTimerA1() {
    int max = 8192 * 0.4; // 0.4 сек (4n)
    TA1CCR0 = max;
    TA1CCR1 = max / 2; // 0.2
    TA1CCR2 = max / 4; // 0.1
    TA1CCTL0 |= CCIE;
    TA1CCTL1 |= CCIE;
    TA1CCTL2 |= CCIE;
    TA1CTL |= TASSEL__ACLK | ID__4 | TACLR;
}

void startTimerA1()
{
    TA1CTL |= MC__UP | TACLR;
    P8OUT |= BIT2;
}

void runTimerB0()
{
    TB0CCR0 = 1000;
    TB0CCTL0 |= CCIE;
    TB0CTL = TBSSEL__ACLK | ID__1 | MC__UP | TBCLR;
}

int state = 1;
#pragma vector = WDT_VECTOR
__interrupt void WDT_interrupt(void) {
    switch (state) {
        case 1:
            P1OUT &= ~BIT0;
            break;
        case 2:
            P8OUT &= ~BIT1;
            break;
        case 3:
            break;
        case 4:
            P8OUT &= ~BIT2;
            WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;
            break;
    }
    state++;
}

void runTimer2()
{
     TA2CTL = TASSEL__ACLK | ID__1 | MC__UP | TACLR;
     TA2CCR0 = 1000;
     TA2CCTL0 |= CCIE;
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void DELAY2(void)
{
        action();
        TB0CCTL0 &= ~CCIE;
        TB0CTL = TBCLR | MC__STOP;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void DELAY1(void)
{
    if(!(P2IN & BIT2))
    {
        changeTimer();
        TA2CCTL0 &= ~CCIE;
        TA2CTL = TACLR | MC__STOP;
    }
}

void action()
{
    turnOn();
    if (Timer == TA1)
    {
        startTimerA1();
    }
    else
    {
        SFRIE1 &= ~WDTIE;
        WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;
        state = 1;
        WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL | WDTSSEL__SMCLK |  WDTIS__32K ;
        SFRIE1 |= WDTIE;
    }
}

#pragma vector = PORT1_VECTOR
__interrupt void S1interrupt()
{
    if(!(TA2CTL & MC__UP))
        runTimerB0();
    P1IFG &= ~BIT7;
    P1IE |= BIT7;
    P1IES ^= BIT7;
    return;
}

#pragma vector = PORT2_VECTOR
__interrupt void S2interrupt()
{
    if(!(TA2CTL & MC__UP))
        runTimer2();
    /*if (Timer == TA1)
        {
            P1OUT |= BIT1;
            Timer = WATCHDOG;
            TA1CCTL0 &= ~CCIE;
            TA1CTL = TACLR | MC__STOP;
        }
        else
        {
            P1OUT &= ~BIT1;
            Timer = TA1;
            SFRIE1 &= ~WDTIE;
            WDTCTL = WDTPW | WDTHOLD | WDTCNTCL;
            confTimerA1();
        }*/
    P2IFG &= ~BIT2;
    return;
}

//сточники частоты
void set_smclk(void){
    UCSCTL4 &= 0;
    UCSCTL5 &= 0;
    UCSCTL1 = DCORSEL_0;
    UCSCTL3 &= 0;
    UCSCTL3 |= SELREF__XT1CLK;
    UCSCTL3 |= FLLREFDIV__1;
    UCSCTL2 &= 0;
    UCSCTL2 |= FLLD__1;
    UCSCTL2 |= 9;
    UCSCTL5 |= DIVS__1;
    UCSCTL4 |= SELS__DCOCLK;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    P1DIR &= ~BIT7;
    P1OUT |= BIT7;
    P1REN |= BIT7;

    P2DIR &= ~BIT2;
    P2OUT |= BIT2;
    P2REN |= BIT2;

    P1DIR |= BIT0;
    P1OUT &=~ BIT0;

    P1DIR |= BIT1;
    P1OUT &=~ BIT1;

    P8DIR |= BIT1;
    P8OUT &=~ BIT1;

    P8DIR |= BIT2;
    P8OUT &=~ BIT2;

    P1DIR |= BIT3;
    P1OUT &=~ BIT3;

    //LED8 дл€ ј0 таймера
    P1DIR |= BIT5;
    P1OUT &= ~BIT5;
    P1SEL |= BIT5;

    //разрещение прерываний по спаду
    P1IES |= BIT7;
    P1IE |= BIT7;
    P1IFG &= ~BIT7;

    P2IES |= BIT2;
    P2IE |= BIT2;
    P2IFG &= ~BIT2;

    set_smclk();
    confTimerA1();

    UCSCTL4 |= SELA__REFOCLK;

    //TA0CCR0 = 6400;
    TA0CCR0 = 32768 / 4;       // XT1 / 4 (0.5 сек)
    TA0CCR4 = (32768 / 4) / 5; // 0.1 сек

    TA0CCTL4 = (OUTMOD_6);
    TA0CTL = TASSEL__ACLK | ID__1 | MC__UP | TACLR; //ID__1 делитель

    _bis_SR_register(GIE);
    __no_operation();
}
