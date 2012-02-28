/**
\brief This program shows the use of the "radio" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012
*/

#include "msp430x26x.h"

#include "stdint.h"
#include "string.h"
#include "board.h"
#include "radio.h"
#include "leds.h"

//=========================== defines =========================================

#define LENGTH_PACKET 48+LENGTH_CRC    // maximum length is 127 bytes
#define CHANNEL       11               // 2.405GHz

//=========================== variables =======================================

typedef struct {
   uint8_t radio_busy;
   uint8_t num_overflow;
   uint8_t num_compare;
   uint8_t num_startFrame;
   uint8_t num_endFrame;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_radioTimerOverflows();
void cb_radioTimerCompare();
void cb_startFrame(uint16_t timestamp);
void cb_endFrame(uint16_t timestamp);

//=========================== main ============================================

uint8_t waitForTimer;

/**
\brief The program starts executing here.
*/
int main(void)
{  
   uint8_t packet[LENGTH_PACKET];
   uint8_t i;
   
   // clear local variables
   memset(&app_vars,0,sizeof(app_vars_t));
   
   // poipoi
   BCSCTL3   |=  LFXT1S_0;                       // ACLK sources from external 32kHz
   TACCTL0    =  CCIE;                           // capture/compare interrupt enable
   TACCR0     =  32768;                          // 32768 @ 832kHz  1000ms
   TACTL      =  MC_1+TASSEL_1;                  // up mode, using ACLK
   
   // initialize board
   board_init();
   
   // add callback functions from radio
   radio_setOverflowCb(cb_radioTimerOverflows);
   radio_setCompareCb(cb_radioTimerCompare);
   radio_setStartFrameCb(cb_startFrame);
   radio_setEndFrameCb(cb_endFrame);
   
   // prepare packet
   for (i=0;i<sizeof(packet);i++) {
      packet[i] = i;
   }
   
   while (1) {
      
      // send packet
      radio_setFrequency(CHANNEL);
      radio_rfOn();
      radio_loadPacket(packet,sizeof(packet));
      radio_txEnable();
      radio_txNow();
      app_vars.radio_busy = 1;
      while (app_vars.radio_busy==1) {
         board_sleep();
      }
      radio_rfOff();
      
      waitForTimer = 1;
      while(waitForTimer==1) {
         board_sleep();
      }
      leds_error_toggle();
   }
}

//=========================== callbacks =======================================

void cb_radioTimerOverflows() {
   app_vars.num_overflow++;
}

void cb_radioTimerCompare() {
   app_vars.num_compare++;
}

void cb_startFrame(uint16_t timestamp) {
   app_vars.num_startFrame++;
}

void cb_endFrame(uint16_t timestamp) {
   app_vars.radio_busy = 0;
   app_vars.num_endFrame++;
}

// poipoi
#pragma vector = TIMERA0_VECTOR
__interrupt void TIMERA0_ISR (void) {
   waitForTimer = 0;
   __bic_SR_register_on_exit(CPUOFF);
}