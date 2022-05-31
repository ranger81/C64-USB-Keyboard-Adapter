/*********************************************************************
 * main.c - Main firmware (ATmega8 version)                          *
 * $Id: $
 * Version 1.98�                                                     *
 *********************************************************************
 * c64key is Copyright (C) 2006-2007 Mikkel Holm Olsen               *
 * based on HID-Test by Christian Starkjohann, Objective Development *
 *********************************************************************
 * Spaceman Spiff's Commodire 64 USB Keyboard (c64key for short) is  *
 * is free software; you can redistribute it and/or modify it under  *
 * the terms of the OBDEV license, as found in the licence.txt file. *
 *                                                                   *
 * c64key is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * OBDEV license for further details.                                *
 *********************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

/* Now included from the makefile */
#include "keymaps/key_c64_us_de.h"

#include "usbdrv.h"
#define DEBUG_LEVEL 0
#include "oddebug.h"

#define MAX_SUSPEND_CNT 10 // in Seconds * 2. No USB Interrupt for this amount of time and the keyboard will send a wakeup call on next keypress instead of the key

/* Hardware documentation:
 * ATmega-8 @12.000 MHz
 *
 * PB0..PB5: Keyboard matrix Row0..Row5 (pins 12,11,10,5,8,7 on C64 kbd)
 * PB6..PB7: 12MHz X-tal
 * PC0..PC5: Keyboard matrix Col0..Col5 (pins 13,19,18,17,16,15 on C64 kbd)
 * PD0     : D- USB negative (needs appropriate zener-diode and resistors)
 * PD1     : UART TX
 * PD2/INT0: D+ USB positive (needs appropriate zener-diode and resistors)
 * PD3     : Keyboard matrix Row8 (Restore key)
 * PD4     : Keyboard matrix Row6 (pin 6 on C64 kbd)
 * PD5     : Keyboard matrix Row7 (pin 9 on C64 kbd)
 * PD6     : Keyboard matrix Col6 (pin 14 on C64 kbd)
 * PD7     : Keyboard matrix Col7 (pin 20 on C64 kbd)
 *
 * USB Connector:
 * -------------
 *  1 (red)    +5V
 *  2 (white)  DATA-
 *  3 (green)  DATA+
 *  4 (black)  GND
 *    
 *
 *
 *                                     VCC
 *                  +--[4k7]--+--[2k2]--+
 *      USB        GND        |                     ATmega-8
 *                            |
 *      (D-)-------+----------+--------[82r]------- PD0
 *                 |
 *      (D+)-------|-----+-------------[82r]------- PD2/INT0
 *                 |     |
 *                 _     _
 *                 ^     ^  2 x 3.6V 
 *                 |     |  zener to GND
 *                 |     |
 *                GND   GND
 */

/* The LED states */
#define LED_NUM     0x01
#define LED_CAPS    0x02
#define LED_SCROLL  0x04
#define LED_COMPOSE 0x08
#define LED_KANA    0x10


/* Originally used as a mask for the modifier bits, but now also
   used for other x -> 2^x conversions (lookup table). */
const char modmask[8] PROGMEM = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
  };


/* USB report descriptor (length is defined in usbconfig.h)
   This has been changed to conform to the USB keyboard boot
   protocol */
char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] 
  PROGMEM = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION  
};

/* This buffer holds the last values of the scanned keyboard matrix */
static uchar bitbuf[NUMROWS]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

/* The ReportBuffer contains the USB report sent to the PC */
static uchar reportBuffer[8];    /* buffer for HID reports */
static uchar idleRate;           /* in 4 ms units */
static uchar protocolVer=1;      /* 0 is the boot protocol, 1 is report protocol */

volatile uchar LEDstate=0;

static void hardwareInit(void) {
  PORTB = 0x3F;   /* Port B are row drivers */
  DDRB  = 0x00;   /* TODO: all pins output */

  PORTC = 0xff;   /* activate all pull-ups */
  DDRC  = 0x00;   /* all pins input */
  
  PORTD = 0xfa;   /* 1111 1000 bin: activate pull-ups except on USB lines */
  DDRD  = 0x07;   /* 0000 0111 bin: all pins input except USB (-> USB reset) */

  /* USB Reset by device only required on Watchdog Reset */
  _delay_us(11);   /* delay >10ms for USB reset */ 

  DDRD = 0x02;    /* 0000 0010 bin: remove USB reset condition */
  /* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
  TCCR0 = 5;      /* timer 0 prescaler: 1024 */


// Test LED Timer1
TCCR1A = 0;
TCCR1B = 0;
TCNT1 = 0;
OCR1A = 5000;
TCCR1B |= (1 << WGM12);
TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
TIMSK |= (1 << OCIE1A);
}

uint8_t suspendFlag = 0 ;

void sendRemoteWakeUp(void){

	cli();
	uint8_t ddr_init = USBDDR, port_init = USBOUT; 	// Get current direction register
	USBDDR |= USBMASK; 						// D+ and D- as Output

	USBOUT |= (1<< USBMINUS); 				// D- high, D+ ist implizit Low, weil war high-impedance input ohne pullup
	
	// set k state
	USBOUT ^= USBMASK; 						// D+ und D- werden invertiert
	// wait
	_delay_ms(10); 							// Pause 10ms
	// set idle
	USBOUT ^= USBMASK; 						// D+ und D- werden invertiert

// revert ddr
	USBDDR = ddr_init; 						// Reset direction register
	// set port without pullup ie D+,D- = 0
	//USBOUT &= ~( 1 << USBMINUS ); 			// D- int. Pullup deaktivieren
	USBOUT = port_init;

	sei();

}

uchar lastSOFcount = 0;
volatile uchar standbyCounter = 0;

ISR(TIMER1_COMPA_vect) {
// LED aus
PORTD&=~0x02;

standbyCounter++;

}


/* Table for decoding bit positions of the last three rows */
const char extrows[3] PROGMEM = { 0x10, 0x20, 0x08 };


/* This function scans the entire keyboard, debounces the keys, and
   if a key change has been found, a new report is generated, and the
   function returns true to signal the transfer of the report. */
static uchar scankeys(void) {
  uchar reportIndex=1; /* First available report entry is 2 */
  uchar retval=0;
  uchar row,data,key, modkeys;
  volatile uchar col, mask;
  static uchar debounce=5;

  for (row=0;row<NUMROWS;++row) { /* Scan all rows */
    DDRD&=~0b00111000; /* all 3 are input */
    PORTD|=0b00111000; /* pull-up enable */
    #ifdef PLUS4
        if (row<6) {
      data=pgm_read_byte(&modmask[row]);
      DDRB=data;
      PORTB=~data;
    } else { // 3 extra rows are on PORTD
      DDRB=0;
      PORTB=0xFF;
      data=pgm_read_byte(&extrows[row-6]);
      DDRD|=data;
      PORTD&=~data;
    }
    
    #else
    if (row<6) {
      data=pgm_read_byte(&modmask[row]);
      DDRB=data;
      PORTB=~data;
    } else if(row<8) { // 3 extra rows are on PORTD
      DDRB=0;
      PORTB=0xFF;
      data=pgm_read_byte(&extrows[row-6]);
      DDRD|=data;
      PORTD&=~data;
    } else { // special for row 8 (restore on c64)
      DDRD&=~0x08;
      PORTD|= 0x08;
    }
    #endif
    
    _delay_us(30); /* Used to be small loop, but the compiler optimized it away ;-) */
  #ifdef PLUS4 
     data=(PINC&0x3F)|(PIND&0xC0);
     #else
         if(row<8) {
      data=(PINC&0x3F)|(PIND&0xC0);
    }
    else {
      
      if((PIND & 0x08) == 0) {
        data = ~(0x08);
      } else {
        data = 0xFF;
      }
     
    }
      #endif
    if (data^bitbuf[row]) { 
      debounce=20; /* If a change was detected, activate debounce counter */ // ge�ndert auf 20, damit weniger doppelbuchstaben kommen, von 10 auf 20
    }
    bitbuf[row]=data; /* Store the result */
  }

  if (debounce==1) { /* Debounce counter expired */
    modkeys=0;
    memset(reportBuffer,0,sizeof(reportBuffer)); /* Clear report buffer */
    for (row=0;row<NUMROWS;++row) { /* Process all rows for key-codes */
      data=bitbuf[row]; /* Restore buffer */
      
      if (data!=0xFF) { /* Anything on this row? - optimization */
        for (col=0,mask=1;col<8;++col,mask<<=1) { /* yes - check individual bits */
          if (!(data&mask)) { /* Key detected */
            key=pgm_read_byte(&keymap[row][col]); /* Read keyboard map */

// LED AN
TCNT1 = 0; // Reset Timer1 Counter
PORTD|=0x02;

if(suspendFlag == 1)
{

sendRemoteWakeUp();

}
// TODO: Danach noch Taste senden? Kommt evtl. nicht an....


            if (key>KEY_Special) { /* Special handling of shifted keys */
              /* Modifiers have not been decoded yet - handle manually */
              uchar keynum=key-(KEY_Special+1);
              #ifdef PLUS4
              if (((bitbuf[4]&0b01000000) || (key >=SPC_grave))&& /* Rshift */
                   ((bitbuf[7]&0b00000010))) {/* Lshift */ // war ((bitbuf[7]&0b00000010)||(key>=SPC_crsrud))) aus irgendeinem Grund....
              #elif defined(C16)
                if (bitbuf[7]&0b00000010) {/* Both shifts */
              #else
              if ((bitbuf[4]&0b01000000)&& /* Rshift */
                   ((bitbuf[7]&0b00000010))) {/* Lshift */ // war ((bitbuf[7]&0b00000010)||(key>=SPC_crsrud))) aus irgendeinem Grund....
              #endif
                key=pgm_read_byte(&spec_keys[keynum][0]); /* Unmodified */
                modkeys=pgm_read_byte(&spec_keys[keynum][1]);
              } else {
                key=pgm_read_byte(&spec_keys[keynum][2]); /* Shifted */
                modkeys=pgm_read_byte(&spec_keys[keynum][3]);
              }
            } else if (key>KEY_Modifiers) { /* Is this a modifier key? */
              reportBuffer[0]|=pgm_read_byte(&modmask[key-(KEY_Modifiers+1)]);
              key=0;
            }
            if (key) { /* Normal keycode should be added to report */
              if (++reportIndex>=sizeof(reportBuffer)) { /* Too many keycodes - rollOver */
                if (!retval&0x02) { /* Only fill buffer once */
                  memset(reportBuffer+2, KEY_errorRollOver, sizeof(reportBuffer)-2);
                  retval|=2; /* continue decoding to get modifiers */
                }
              } else {
                reportBuffer[reportIndex]=key; /* Set next available entry */
              }
            }
          }
        }
      }
    }
    if (modkeys&0x80) { /* Clear RSHIFT */
      reportBuffer[0]&=~0x20;
    }
    if (modkeys&0x08) { /* Clear LSHIFT */
      reportBuffer[0]&=~0x02;
    }
    reportBuffer[0]|=modkeys&0x77; /* Set other modifiers */

    retval|=1; /* Must have been a change at some point, since debounce is done */
  }
  if (debounce) debounce--; /* Count down, but avoid underflow */
  return retval;
}

uchar expectReport=0;

uchar usbFunctionSetup(uchar data[8]) {
  usbRequest_t *rq = (void *)data;
  usbMsgPtr = reportBuffer;
  if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
    if(rq->bRequest == USBRQ_HID_GET_REPORT){  
      /* wValue: ReportType (highbyte), ReportID (lowbyte) */
      /* we only have one report type, so don't look at wValue */
      return sizeof(reportBuffer);
    }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
      if (rq->wLength.word == 1) { /* We expect one byte reports */
        expectReport=1;
        return 0xFF; /* Call usbFunctionWrite with data */
      }  
    }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
      usbMsgPtr = &idleRate;
      return 1;
    }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
      idleRate = rq->wValue.bytes[1];
    }else if(rq->bRequest == USBRQ_HID_GET_PROTOCOL) {
      if (rq->wValue.bytes[1] < 1) {
        protocolVer = rq->wValue.bytes[1];
      }
    }else if(rq->bRequest == USBRQ_HID_SET_PROTOCOL) {
      usbMsgPtr = &protocolVer;
      return 1;
    }
  }
  return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
  if ((expectReport)&&(len==1)) {
    //LEDstate=data[0]; /* Get the state of all 5 LEDs */
    //if (LEDstate&LED_CAPS) { /* Check state of CAPS lock LED */
    //  PORTD|=0x02;
    //} else {
    //  PORTD&=~0x02;
    //}
    expectReport=0;
    return 1;
  }
  expectReport=0;
  return 0x01;
}

int main(void) {
  uchar   updateNeeded = 0;
  uchar   idleCounter = 0;

  wdt_enable(WDTO_2S); /* Enable watchdog timer 2s */
  hardwareInit(); /* Initialize hardware (I/O) */
  
  odDebugInit();

  usbInit(); /* Initialize USB stack processing */
  sei(); /* Enable global interrupts */
  
  for(;;){  /* Main loop */
    wdt_reset(); /* Reset the watchdog */
    usbPoll(); /* Poll the USB stack */

    updateNeeded|=scankeys(); /* Scan the keyboard for changes */
    
    /* Check timer if we need periodic reports */
    if(TIFR & (1<<TOV0)){
      TIFR = 1<<TOV0; /* Reset flag */
      if(idleRate != 0){ /* Do we need periodic reports? */
        if(idleCounter > 4){ /* Yes, but not yet */
          idleCounter -= 5;   /* 22 ms in units of 4 ms */
        }else{ /* Yes, it is time now */
          updateNeeded = 1;
          idleCounter = idleRate;
        }
      }
    }


    /* If an update is needed, send the report */
    if(updateNeeded && usbInterruptIsReady()){
      updateNeeded = 0;
      usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
    }


if(usbSofCount != 0) {
usbSofCount = 0;
suspendFlag = 0;
standbyCounter = 0;
//PORTD&=~0x02;
} else {
if (standbyCounter > 5) {
suspendFlag = 1;
//PORTD|=0x02;
}
}

  }
  return 0;
}
