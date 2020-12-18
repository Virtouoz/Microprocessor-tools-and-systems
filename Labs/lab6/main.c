#include <msp430.h>
#include "CTS_Layer.h"
#include <math.h>


typedef unsigned char uchar;

#define SET_COLUMN_ADDRESS_LSB        0x00
#define SET_COLUMN_ADDRESS_MSB        0x10
#define SET_PAGE_ADDRESS              0xB0

#define SET_SEG_DIRECTION             0xA0
#define SET_COM_DIRECTION             0xC0

#define SET_POWER_CONTROL             0x2F
#define SET_SCROLL_LINE               0x40
#define SET_VLCD_RESISTOR_RATIO       0x27
#define SET_ELECTRONIC_VOLUME_MSB     0x81
#define SET_ELECTRONIC_VOLUME_LSB     0x0F
#define SET_ALL_PIXEL_ON              0xA4
#define SET_INVERSE_DISPLAY           0xA6
#define SET_DISPLAY_ENABLE            0xAF
#define SET_LCD_BIAS_RATIO            0xA2
#define SET_ADV_PROGRAM_CONTROL0_MSB  0xFA
#define SET_ADV_PROGRAM_CONTROL0_LSB  0x90

#define CD              BIT6
#define CS              BIT4

#define CALADC12_15V_30C *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C *((unsigned int *)0x1A1C)

#define ROWS 9
#define COLUMNS 6
#define PAGES 2
#define DELAY 1000
#define COLUMN_OFFSET_BIG 30
#define COLUMN_OFFSET_NONE 0


void SetupADC();
void SetupLCD();

void __LCD_SetAddress(uchar pa, uchar ca);
void Dogs102x6_writeData(uchar *sData, uchar i);
void Dogs102x6_writeCommand(uchar *sCmd, uchar i);

void Clear(void);
void ShowNumber(long int number);

int getNumberLength(long int number);

int GetS1State();
void SetupControllButton();

void SetupTimer();
int COLUMN_START_ADDRESS = 123;

uchar symbols[12][9] = {
        {0x20, 0x20, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x20, 0x20},
        {0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00},
        {0x78, 0xFC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x78},
        {0x3C, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0xFC, 0xFC},
        {0xFC, 0xFC, 0xC0, 0xC0, 0xFC, 0x0C, 0x0C, 0xFC, 0xFC},
        {0xFC, 0xFC, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xFC, 0xFC},
        {0xCC, 0xCC, 0xCC, 0xFC, 0xFC, 0xC0, 0xC0, 0xC0, 0xC0},
        {0xFC, 0xFC, 0x0C, 0x0C, 0xFC, 0xFC, 0xC0, 0xFC, 0xFC},
        {0xFC, 0xFC, 0x0C, 0xFC, 0xFC, 0xCC, 0xCC, 0xFC, 0xFC},
        {0xFC, 0xFC, 0xC0, 0xC0, 0xE0, 0x70, 0x30, 0x30, 0x30},
        {0xFC, 0xFC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xFC, 0xFC},
        {0xFC, 0xFC, 0xCC, 0xCC, 0xFC, 0xFC, 0xC0, 0xFC, 0xFC}
};

int column_offset = COLUMN_OFFSET_BIG;

uchar Dogs102x6_initMacro[] = {
    SET_SCROLL_LINE,
    SET_SEG_DIRECTION,
    SET_COM_DIRECTION,
    SET_ALL_PIXEL_ON,
    SET_INVERSE_DISPLAY,
    SET_LCD_BIAS_RATIO,
    SET_POWER_CONTROL,
    SET_VLCD_RESISTOR_RATIO,
    SET_ELECTRONIC_VOLUME_MSB,
    SET_ELECTRONIC_VOLUME_LSB,
    SET_ADV_PROGRAM_CONTROL0_MSB,
    SET_ADV_PROGRAM_CONTROL0_LSB,
    SET_DISPLAY_ENABLE,
    SET_PAGE_ADDRESS,
    SET_COLUMN_ADDRESS_MSB,
    SET_COLUMN_ADDRESS_LSB
};
int AMOUNT_OF_COMMANDS_1 = 7;
int AMOUNT_OF_COMMANDS_2 = 6;


#define NUM_KEYS    5
#define LED4        BIT5
#define LED5        BIT4
#define LED6        BIT3
#define LED7        BIT2
#define LED8        BIT1

struct Element* keypressed;

const struct Element* address_list[NUM_KEYS] = {
		&PAD1,
		&PAD2,
		&PAD3,
		&PAD4,
		&PAD5
};
const uint8_t ledMask[NUM_KEYS] = {
LED4,
LED5,
LED6,
LED7,
LED8
};

void SetupControllButton()
{
	P1DIR &= ~BIT7;
	P1OUT |= BIT7;
	P1REN |= BIT7;
	P1IES |= BIT7;
	P1IE |= BIT7;
	P1IFG |= BIT7;
}

int GetS1State()
{
	return (P1IN & BIT7) ? 0 : 1;
}

void SetupLCD()
{

        P5DIR |= BIT7;
        P5OUT &= BIT7;
        P5OUT |= BIT7;

        P7DIR |= CS;

        P5DIR |= CD;
        P5OUT &= ~CD;

        P4SEL |= BIT1;
        P4DIR |= BIT1;

        P4SEL |= BIT3;
        P4DIR |= BIT3;

        UCB1CTL1 = UCSSEL_2 + UCSWRST;
        UCB1BR0 = 0x02;
        UCB1BR1 = 0;

        UCB1CTL0 = UCCKPH + UCMSB + UCMST + UCMODE_0 + UCSYNC;

        UCB1CTL1 &= ~UCSWRST;
        UCB1IFG &= ~UCRXIFG;

        Dogs102x6_writeCommand(Dogs102x6_initMacro, 13);
}

void __LCD_SetAddress(uchar pa, uchar ca)
{
    uchar cmd[1];

       if (pa > 7)
       {
           pa = 7;
       }

       cmd[0] = SET_PAGE_ADDRESS + (7 - pa);
       uchar H = 0x00;
       uchar L = 0x00;
       uchar ColumnAddress[] = { SET_COLUMN_ADDRESS_MSB, SET_COLUMN_ADDRESS_LSB };

       L = (ca & 0x0F);
       H = (ca & 0xF0);
       H = (H >> 4);

       ColumnAddress[0] = SET_COLUMN_ADDRESS_LSB + L;
       ColumnAddress[1] = SET_COLUMN_ADDRESS_MSB + H;

       Dogs102x6_writeCommand(cmd, 1);
       Dogs102x6_writeCommand(ColumnAddress, 2);
}

void Dogs102x6_writeCommand(uchar *sCmd, uchar i)
{
	P7OUT &= ~BIT4;

	P5OUT &= ~BIT6;
	while (i)
	{
		while (!(UCB1IFG & UCTXIFG)) ;

		UCB1TXBUF = *sCmd;
		sCmd++;
		i--;
	}

	while (UCB1STAT & UCBUSY);

	UCB1RXBUF;

	P7OUT |= BIT4;
}

void Dogs102x6_writeData(uchar *sData, uchar i)
{
	P7OUT &= ~BIT4;

	P5OUT |= BIT6;

	while (i)
	{
		while (!(UCB1IFG & UCTXIFG));
		UCB1TXBUF = *sData++;
		i--;
	}
	while (UCB1STAT & UCBUSY);

	UCB1RXBUF;

	P7OUT |= BIT4;
}

void ShowNumber(long int number)
{
    int nDigits = getNumberLength(number);

    __LCD_SetAddress(nDigits, COLUMN_START_ADDRESS);
        Dogs102x6_writeData(number > 0 ? symbols[0] : symbols[1], 9);

        int i = 0;
        long int divider = pow(10, nDigits - 1);

        number = fabsl(number);

        for (i = nDigits; i > 0; i--) {
            int digit = number / divider;

            __LCD_SetAddress(i-1, COLUMN_START_ADDRESS);
            Dogs102x6_writeData(symbols[digit + 2], 9);

            number = number % divider;
            divider /= 10;
        }
}

void Clear(void)
{
	uchar lcd_data[] = {0x00};
	uchar page, column;

	for (page = 0; page < 8; page++)
	{
		__LCD_SetAddress(page, 0);
		for (column = 0; column < 132; column++)
		{
			Dogs102x6_writeData(lcd_data, 1);
		}
	}
}

void SetupTimer()
{
    TA0CTL = TASSEL__SMCLK | MC__UP | ID__1 | TACLR;
    long int second = 32768;
    long int period = second / 2;
    TA0CCR0 = second;
    TA0CCR1 = period;
    TA0CCTL1 = OUTMOD_3;
}

void SetupADC()
{
    P8DIR |= BIT1;
    P8OUT &= ~BIT1;

    REFCTL0 &= ~REFMSTR;
	ADC12CTL0 =
			ADC12SHT0_8
			+ ADC12REFON
			+ ADC12ON;

	ADC12CTL1 =
			ADC12SHP
			+ ADC12SHS_1
			+ ADC12SSEL_0;

	ADC12MCTL0 =
			ADC12SREF_1
			 + ADC12INCH_10;

	ADC12IE = ADC12IE0;

	__delay_cycles(100);

	ADC12CTL0 |= ADC12ENC;
}


int getNumberLength(long int number) {
    int length = 0;
    number = fabsl(number);

    if(number == 0) {
        return 1;
    }

    while(number) {
        number /= 10;
        length++;
    }

    return length;
}

#define CALADC12_15V_30C *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C *((unsigned int *)0x1A1C)


#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR() {
	P8OUT ^= BIT1;

	TA0CTL &= ~MC__UP;
	TA0CTL |= MC__STOP | TACLR;
	TA0CCTL1 &= ~BIT2;

	unsigned short int probe = ADC12MEM0 & 0x0FFF;
	ADC12MEM0 = 0;

	volatile long int temp1 = probe;
	volatile long int temp2 = temp1 - CALADC12_15V_30C;
	volatile float temp3 = temp2 * (85 - 30);
	volatile float temp4 = temp3 / (CALADC12_15V_85C - CALADC12_15V_30C);
	temp4 += 30;
	volatile long int deg_c = (long int)temp4;

	Clear();
	ShowNumber(deg_c);

	ADC12CTL0 &= ~ADC12ENC;
}

#pragma vector = PORT1_VECTOR
__interrupt void HandleS1ISR()
{

	if (GetS1State())
	{
		if (!(ADC12CTL1 & ADC12BUSY))
		{
			SetupTimer();
			ADC12CTL0 |= ADC12ENC;
		}
	}

	P1IFG &= ~BIT7;
}

void SetVcoreUp(uint16_t level)
{
    PMMCTL0_H = PMMPW_H;
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
    while ((PMMIFG & SVSMLDLYIFG) == 0);
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    PMMCTL0_L = PMMCOREV0 * level;
    if ((PMMIFG & SVMLIFG))
        while ((PMMIFG & SVMLVLRIFG) == 0);
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
    PMMCTL0_H = 0x00;
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;

	uint8_t i;
    SetupLCD();
    P7DIR |= BIT6;
    P7OUT |= BIT6;
    P7SEL &= ~BIT6;
    Clear();

	P1DIR = 0xFF;
	P2DIR = 0xFF;
	P8DIR = 0xFF;
	P1OUT = 0;
	P2OUT = 0;
	P8OUT = 0;

	SetVcoreUp(0x01);
	SetVcoreUp(0x02);
	SetVcoreUp(0x03);

	UCSCTL3 = SELREF_2;
	UCSCTL4 |= SELA_2;

	__bis_SR_register(SCG0);
	UCSCTL0 = 0x0000;
	UCSCTL1 = DCORSEL_7;    
	UCSCTL2 = FLLD_1 + 762;


	__bic_SR_register(SCG0);
	__delay_cycles(782000);
	do
	{
	  UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
	  SFRIFG1 &= ~OFIFG;
	}
	while (SFRIFG1 & OFIFG);

	TI_CAPT_Init_Baseline(&keypad);
	TI_CAPT_Update_Baseline(&keypad, 5);

	SetupControllButton();
	SetupADC();

	  __bis_SR_register(GIE);

	while (1)
	{
	  P1OUT &= ~(LED4 + LED5 + LED6 + LED7 + LED8);
	  keypressed = (struct Element *) TI_CAPT_Buttons(&keypad);
	  __no_operation();
	  if (keypressed)
	  {
		  for (i = 0; i < NUM_KEYS; i++)
		  {
			  if (keypressed == address_list[3])
			  {
				  P1OUT |= BIT4;

				  if (!(ADC12CTL1 & ADC12BUSY))
				  		{
				  			SetupTimer();
				  			ADC12CTL0 |= ADC12ENC;
				  		}
			  }
		  }
	  }
	  __delay_cycles(900000);
	}

  return 0;
}
