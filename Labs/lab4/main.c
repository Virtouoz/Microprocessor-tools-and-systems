#include <msp430.h> 
#include "stdlib.h"

#define CD              BIT6
#define CS              BIT4

typedef unsigned char uint8_t;
typedef short int uint16_t;

uint8_t init_cmds[] =
                    {0x40, //установка начальной строки скроллинга = 0 (без скроллинга)
                     0xA1, //нормальный режим адресации столбцов
                     0xC8, //нормальный режим адресации строк
                     0xA4, //запрет режима включения всех пикселей (на экран отображается содержимое памяти)
                     0xA6, //отключение инверсного режима экрана
                     0xA2, //смещение напряжения делителя 1/9
                     0x2F, //включение питания усилителя, регулятора и повторителя
                     0x27, //установка контраста
                     0x81,
                     0x10,
                     0xFA, //установка температурной компенсации -0.11%/°С
                     0x90,
                     0xAF  //включение экрана
                    };

uint8_t mirror_mode = 0xC0;
uint8_t normal_mode = 0xC8;

volatile int display_mode = 0; //0 - NORMAL, 1 - MIRROR


int num_to_display = -2014;
const int num_to_add = 301;

uint8_t distance[] = { 0x00, 0x00, 0x00 };
uint8_t digit_0[] = {0x7F, 0x80, 0x80, 0x80, 0x7F,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t digit_1[] = {0x10, 0x20, 0x40, 0x80, 0xFF,
                    0x00, 0x00, 0x00, 0x00, 0x80 };
uint8_t digit_2[] = {0x41, 0x82, 0x84, 0x88, 0x70,
                    0x80, 0x80, 0x80, 0x80, 0x80 };
uint8_t digit_3[] = {0x41, 0x80, 0x88, 0x88, 0x77,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t digit_4[] = {0xF8, 0x08, 0x08, 0x08, 0xFF,
                    0x00, 0x00, 0x00, 0x00, 0x80 };
uint8_t digit_5[] = {0xF1, 0x88, 0x88, 0x88, 0x87,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t digit_6[] = {0x7F, 0x88, 0x88, 0x88, 0x47,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t digit_7[] = {0x80, 0x80, 0x80, 0x80, 0xFF,
                    0x00, 0x00, 0x00, 0x00, 0x80 };
uint8_t digit_8[] = {0x77, 0x88, 0x88, 0x88, 0x77,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t digit_9[] = {0x71, 0x88, 0x88, 0x88, 0x7F,
                    0x00, 0x80, 0x80, 0x80, 0x00 };
uint8_t plus_sign[] = { 0x08, 0x08, 0x3E, 0x08, 0x08,
                       0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t minus_sign[] = { 0x08, 0x08, 0x08, 0x08, 0x08,
                        0x00, 0x00, 0x00, 0x00, 0x00 };

int s1, s2, s_int;

void onS1();
void onS2();
void startTimerA1();

void init_LCD_pins();
void init_USCI();
void clear_LCD();
void display_symbol(uint8_t *digit, uint8_t column);
void display_num();

void Dogs102x6_writeCommand(uint8_t *sCmd, uint8_t i);
void Dogs102x6_writeData(uint8_t *sData, uint8_t i);

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    s_int = 0;
    s1 = 1;
    s2 = 1;

    P8OUT &= ~BIT1;
    P8DIR |= BIT1; //set LED2 off
    P8OUT &= ~BIT2;
    P8DIR |= BIT2; //set LED3 off


    P1DIR &= ~BIT7; // S1 set to input
    P1IE |= BIT7;   // S1 allow interrupt
    P1IFG &= ~BIT7; // S1 reset interrupt flag
    P1REN |= BIT7;
    P1OUT |= BIT7;
    P1IES |= BIT7;

    P2DIR &= ~BIT2; // S2 set to input
    P2IE |= BIT2;   // S2 allow interrupt
    P2IFG &= ~BIT2; // S2 reset interrupt flag
    P2REN |= BIT2;
    P2OUT |= BIT2;
    P2IES |= BIT2;

    init_LCD_pins();//??

    init_USCI();//??

    Dogs102x6_writeCommand(init_cmds, 13);//??

    clear_LCD();

    display_num();

    _bis_SR_register(GIE);

    __no_operation();
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0(void) {
    if (s_int == 1) {
        onS1();
    }
    if (s_int == 2) {
        onS2();
    }
    TA1CCTL0 &= ~CCIE;
    TA1CTL = MC__STOP;
    s_int = 0;
}
#pragma vector = PORT1_VECTOR
__interrupt void PORT1(void)
{
    s_int = 1;
    startTimerA1();
    P1IFG &= ~BIT7;

}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2(void)
{
    s_int = 2;
    startTimerA1();
    P2IFG &= ~BIT2;
}

void onS1() {
    if (s1 == 1) {
        if ((P1IN & BIT7) == 0) {
            //действие по нажатию S1
            num_to_display += num_to_add;
            clear_LCD();
            display_num();
            s1 = 0;
            P1IES &= ~BIT7;
        }
    }
    else {
        if ((P1IN & BIT7) != 0) {
            s1 = 1;
            P1IES |= BIT7;
        }
    }
}

void onS2() {
    if (s2 == 1) {
        if ((P2IN & BIT2) == 0) {
            //действие по нажатию S2
            if (display_mode == 0)
            {
                Dogs102x6_writeCommand(&mirror_mode, 1);
                display_mode = 1;
            }else{
                Dogs102x6_writeCommand(&normal_mode, 1);
                display_mode = 0;
            }
            s2 = 0;
            P2IES &= ~BIT2;
        }
    }
    else {
        if ((P2IN & BIT2) != 0) {
            s2 = 1;
            P2IES |= BIT2;
        }
    }
}

void startTimerA1() {
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL__ACLK | ID_1 | MC__UP | TACLR;
    TA1CCR0 = 64;
    TA1CCTL0 &= ~CCIFG;
}


void init_LCD_pins()
{
    P5DIR |= BIT7;
    P5SEL &= ~BIT7;

    P5OUT &= ~BIT7;

    __delay_cycles(25000);
    P5OUT |= BIT7;
    __delay_cycles(125000);

    P4DIR |= BIT1;
    P4SEL |= BIT1;

    P4DIR |= BIT3;
    P4SEL |= BIT3;

    P5DIR |= BIT6;
    P5SEL &= ~BIT6;

    P7DIR |= BIT4;
    P7SEL &= ~BIT4;
    P7OUT |= BIT4;

    P7DIR |= BIT6;
    P7SEL &= ~BIT6;
    P7OUT |= BIT6;


}

void init_USCI()
{
    UCB1CTL1 |= UCSWRST;

    UCB1CTL0 |= UCMSB;
    UCB1CTL0 |= UCMST;
    UCB1CTL0 |= UCCKPH;

    UCB1CTL1 |= UCSSEL1;

    UCB1CTL1 &= ~UCSWRST;
}

void clear_LCD()
{
    uint8_t page_cmd;
    uint8_t column_cmd[2];

    uint8_t data = 0x00;

    int p, c;

    for(p = 0; p < 8; p++)
    {
        page_cmd = 0xB0 + p;

        Dogs102x6_writeCommand(&page_cmd, 1);

        column_cmd[0] = 0x00;
        column_cmd[1] = 0x10;

        Dogs102x6_writeCommand(column_cmd, 2);

        for(c = 0; c < 132; c++)
        {

            Dogs102x6_writeData(&data, 1);
        }

    }
}

void display_symbol(uint8_t *digit, uint8_t column)
{
    uint8_t page_cmd;
    uint8_t column_cmd[2];

    uint8_t column_LSB;
    uint8_t column_MSB;
    int p;

    column_LSB = 0x0F & column;
    column_MSB = (0xF0 & column) >> 4;

    column_cmd[0] = 0x00 + column_LSB;
    column_cmd[1] = 0x10 + column_MSB;

    for(p = 0; p < 2; p++){
        page_cmd = 0xB0 + (7-p);//??p

        Dogs102x6_writeCommand(&page_cmd, 1);
        Dogs102x6_writeCommand(column_cmd, 2);

        Dogs102x6_writeData(distance, 3);
        Dogs102x6_writeData(digit + 5 * p, 5);
    }
}

void display_num(){
    int divided_num = num_to_display < 0 ? num_to_display * (-1) : num_to_display;
    int digit;
    int digit_count = 1;

    while(divided_num != 0){
        digit = divided_num % 10;

        switch(digit){
            case 0:
                display_symbol(digit_0, 102 - digit_count * 8);
                break;
            case 1:
                display_symbol(digit_1, 102 - digit_count * 8);
                break;
            case 2:
                display_symbol(digit_2, 102 - digit_count * 8);
                break;
            case 3:
                display_symbol(digit_3, 102 - digit_count * 8);
                break;
            case 4:
                display_symbol(digit_4, 102 - digit_count * 8);
                break;
            case 5:
                display_symbol(digit_5, 102 - digit_count * 8);
                break;
            case 6:
                display_symbol(digit_6, 102 - digit_count * 8);
                break;
            case 7:
                display_symbol(digit_7, 102 - digit_count * 8);
                break;
            case 8:
                display_symbol(digit_8, 102 - digit_count * 8);
                break;
            case 9:
                display_symbol(digit_9, 102 - digit_count * 8);
                break;
        }

        divided_num = divided_num / 10;

        digit_count++;
    }

    if(num_to_display > 0)
    {
        display_symbol(plus_sign, 102 - digit_count * 8);
    }
    else if(num_to_display < 0)
    {
        display_symbol(minus_sign, 102 - digit_count * 8);
    }
}

void Dogs102x6_writeData(uint8_t *sData, uint8_t i)
{
    // Store current GIE state
    uint16_t gie = __get_SR_register() & GIE;

    // Make this operation atomic
    __disable_interrupt();
      // CS Low
      P7OUT &= ~CS;
      //CD High
      P5OUT |= CD;

      while (i)
      {
          // USCI_B1 TX buffer ready?
          while (!(UCB1IFG & UCTXIFG)) ;

          // Transmit data and increment pointer
          UCB1TXBUF = *sData++;

          // Decrement the Byte counter
          i--;
      }

      // Wait for all TX/RX to finish
      while (UCB1STAT & UCBUSY) ;

      // Dummy read to empty RX buffer and clear any overrun conditions
      UCB1RXBUF;

      // CS High
      P7OUT |= CS;

    // Restore original GIE state
    __bis_SR_register(gie);
}

void Dogs102x6_writeCommand(uint8_t *sCmd, uint8_t i)
{
    // Store current GIE state
    uint16_t gie = __get_SR_register() & GIE;

    // Make this operation atomic
    __disable_interrupt();

    // CS Low
    P7OUT &= ~CS;

    // CD Low
    P5OUT &= ~CD;
    while (i)
    {
        // USCI_B1 TX buffer ready?
        while (!(UCB1IFG & UCTXIFG)) ;

        // Transmit data
        UCB1TXBUF = *sCmd;

        // Increment the pointer on the array
        sCmd++;

        // Decrement the Byte counter
        i--;
    }

    // Wait for all TX/RX to finish
    while (UCB1STAT & UCBUSY) ;

    // Dummy read to empty RX buffer and clear any overrun conditions
    UCB1RXBUF;

    // CS High
    P7OUT |= CS;

    // Restore original GIE state
    __bis_SR_register(gie);
}
