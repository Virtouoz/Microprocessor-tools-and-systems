#include <msp430.h>

void delay(volatile unsigned int time){
//  volatile unsigned int time = 16000;
    while(time){--time;}
}

int lightOn=0;


#pragma vector=PORT2_VECTOR
__interrupt void INT(){
    P2IE &= ~BIT2;
    if(P1IN & BIT7){
//      P1OUT ^= BIT0;
        delay(10000);
        if(lightOn){
            lightOn=0;
            P1OUT &= ~BIT0;
        }
        else{
            lightOn=1;

            P1OUT |= BIT0;
        }
    }

    P2IFG &= ~BIT2;
    P2IE |= BIT2;
}


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    //P1 BIT0 - LED1
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    //P1 BIT7 - S1
    P1OUT |= BIT7;
    P1REN |= BIT7;

    //P2 BIT2 - S2
    P2OUT |= BIT2;
    P2REN |= BIT2;

//  P2IE &= ~BIT2;
    P2IES &= ~BIT2;
    P2IE |= BIT2;
    P2IFG &= ~BIT2;

    __bis_SR_register(GIE);
    __no_operation();

//  return 0;
}



//Без обработки прерываний

//#include <msp430.h>
//
//void setupLed(){
//  //P1 BIT0 - LED1
//  P1DIR |= BIT0;
//  P1OUT &= ~BIT0;
//}
//
//void setupButtons(){
//  //P1 BIT7 - S1
//  P1OUT |= BIT7;
//  P1REN |= BIT7;
//
//  //P2 BIT2 - S2
//  P2OUT |= BIT2;
//  P2REN |= BIT2;
//}
//
//void delay(){
//  volatile unsigned int time = 3000;
//  while(time){--time;}
//}
//
///*
// * main.c
// */
//int main(void) {
//    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
//
//  setupLed();
//  setupButtons();
//
//  int S1_CurPressed = 0;
//  int S2_CurPressed = 0;
//
//  int S2_PrevPressed=0;
//
//  int lightOn = 0;
//
//  while(1){
//
//      if(lightOn){
//          P1OUT |= BIT0;
//      }else{
//          P1OUT &= ~BIT0;
//      }
//
//      S1_CurPressed = (P1IN & BIT7)? 0 : 1;
//      S2_CurPressed = (P2IN & BIT2)? 0 : 1;
//
//      if(S2_PrevPressed && !S2_CurPressed && !S1_CurPressed){
//          if(lightOn){
//              lightOn=0;
//          }
//          else{
//              lightOn=1;
//          }
//      }
//      S2_PrevPressed = S2_CurPressed;
//      delay();
//  }
//  return 0;
//}



