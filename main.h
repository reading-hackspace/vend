/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for USBtoSerial.c.
 */

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/interrupt.h>
		#include <avr/power.h>

		#include "Descriptors.h"

		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Drivers/Peripheral/Serial.h>
		#include <LUFA/Drivers/Misc/RingBuffer.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Platform/Platform.h>

	/* Function Prototypes: */
		void SetupHardware(void);

		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);

#endif

struct pin {
    volatile uint8_t * port;
    uint8_t pin;
};

const struct pin vmc_col[] PROGMEM = {
    {.port = &PINC, .pin = 2},
    {.port = &PINB, .pin = 0},
    {.port = &PINB, .pin = 1}
};

const struct pin vmc_row[] PROGMEM = {
    {.port = &PORTD, .pin = 0},
    {.port = &PORTD, .pin = 1},
    {.port = &PORTD, .pin = 2},
    {.port = &PORTD, .pin = 3},
    {.port = &PORTD, .pin = 4},
    {.port = &PORTD, .pin = 5},
    {.port = &PORTD, .pin = 6}
};

const struct pin key_col[] PROGMEM = {
    {.port = &PORTC, .pin = 4},
    {.port = &PORTC, .pin = 5},
    {.port = &PORTC, .pin = 6}
};

const struct pin key_row[] PROGMEM = {
    {.port = &PINB, .pin = 2},
    {.port = &PINB, .pin = 3},
    {.port = &PINB, .pin = 4},
    {.port = &PINB, .pin = 5},
    {.port = &PINB, .pin = 6},
    {.port = &PINB, .pin = 7},
    {.port = &PINC, .pin = 7}
};

const char keys[3][7] PROGMEM = {
    {'2', '4', '6', '8', '0', 'h', 'l'},
    {'1', '3', '5', '7', '9', 'g', 'k'},
    {'a', 'b', 'c', 'd', 'e', 'f', 'j'}
};

#define HIGH(x) *(volatile uint8_t *)pgm_read_ptr(&x.port) |= _BV(pgm_read_byte(&x.pin))
#define LOW(x) *(volatile uint8_t *)pgm_read_ptr(&x.port) &= ~_BV(pgm_read_byte(&x.pin))
#define READ(x) (*(volatile uint8_t *)pgm_read_ptr(&x.port) & _BV(pgm_read_byte(&x.pin)))
