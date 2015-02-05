/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2015  Barnaby <b@Zi.iS>

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
 *  USB adapter for vending machine keypad
 */

#include "main.h"
#include <util/atomic.h>

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];

/** Track time **/
volatile unsigned long _ms;
ISR(TIMER1_COMPA_vect) {++_ms;}

unsigned long millis(void)
{
    unsigned long r;
    ATOMIC_BLOCK(ATOMIC_FORCEON)
    {        r = _ms;    }
    return r;
}

/** Debounce and rate limit buttons **/
static unsigned long next[3][7];
volatile uint8_t next_row;
volatile uint8_t next_col = 255;

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
	RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

	GlobalInterruptEnable();

	for (;;)
	{
		/* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
		if (!(RingBuffer_IsFull(&USBtoUSART_Buffer)))
		{
			int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			/* Store received byte into the USART transmit buffer */
			if (!(ReceivedByte < 0))
			  RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
		}

		uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
		if (BufferCount)
		{
			Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

			/* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
			 * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */
			if (Endpoint_IsINReady())
			{
				/* Never send more than one bank size less one byte to the host at a time, so that we don't block
				 * while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
				uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

				/* Read bytes from the USART receive buffer into the USB IN endpoint */
				while (BytesToSend--)
				{
					/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
					if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
											RingBuffer_Peek(&USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
					{
						break;
					}

					/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
					RingBuffer_Remove(&USARTtoUSB_Buffer);
				}
			}
		}

        for(uint8_t col=0; col<3; col++) //Scan the coloumns
        {
            LOW(key_col[col]);
            for(uint8_t row=0; row<7; row++)
            {
                if(!READ(key_row[row])) //Check each row
                {
                    if(millis() > next[col][row]) //Debounce and rate limit
                    {
                        next[col][row] = millis() + 100;
                        RingBuffer_Insert(&USARTtoUSB_Buffer, pgm_read_byte(&(keys[col][row]))); //Send to PC
                    }
                }
            }
            HIGH(key_col[col]);
        }

        if(next_col == 255 && !RingBuffer_IsEmpty(&USBtoUSART_Buffer)) //Ready next output
        {
            next_row = 6;
            switch(RingBuffer_Remove(&USBtoUSART_Buffer))
            {
                case 'l':
                    --next_row;
                case 'h':
                    --next_row;
                case '0':
                    --next_row;
                case '8':
                    --next_row;
                case '6':
                    --next_row;
                case '4':
                    --next_row;
                case '2':
                    next_col = 2;
                    break;
                case 'k':
                    --next_row;
                case 'g':
                    --next_row;
                case '9':
                    --next_row;
                case '7':
                    --next_row;
                case '5':
                    --next_row;
                case '3':
                    --next_row;
                case '1':
                    next_col = 1;
                    break;
                case 'j':
                    --next_row;
                case 'f':
                    --next_row;
                case 'e':
                    --next_row;
                case 'd':
                    --next_row;
                case 'c':
                    --next_row;
                case 'b':
                    --next_row;
                case 'a':
                    next_col = 0;
                    break;
                case 's':
                    DDRD |= (1<<7); //Pull Low
                    _delay_ms(100);
                    DDRD &= ~(1<<7); //Hi-Z
                    break;
            }
            if(next_col < 255)
            {
                RingBuffer_Insert(&USARTtoUSB_Buffer, '='); //Send to PC
            }
        }

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#endif

    /* Timmer for millis() */
    TCCR1B |= _BV(WGM12) | _BV(CS11); // CTC mode, Clock/8
    OCR1A = ((F_CPU / 1000) / 8);
    TIMSK1 |= _BV(OCIE1A); //compare match int

    /* PCINTs */
    PCICR |= _BV(PCIE1);// | _BV(PCIE0); // enable both 0:7 and 8:12
//    PCMSK0 |= _BV(PCINT0) | _BV(PCINT1); //trigger on 0 and 1
    PCMSK1 |= _BV(PCINT11); //trigger on 11

	/* Hardware Initialization */
	USB_Init();

    /* GPIO */
    DDRB = 0b00000000; //2 VMC inputs and 6 Keypad Inputs
    PORTB = 0b11111100; //Pull up keypad inputs

    DDRC = 0b01110000; //Keypad input, 3 keypad outputs, NC, VMC input, NC, NC
    PORTC = 0b11110000; //Pull up keypad input, Keypad outputs rest high

    DDRD = 0b01111111; // Service, VMC outputs
    PORTD = 0b01111111; //Open, Enable pullups on all VMC

}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}


ISR(PCINT0_vect)
{
    //HACK: This takes long enough to ensure we only see the first edge
    if(next_col < 255 && !READ(vmc_col[next_col])) {
        _delay_loop_2(60000);
        _delay_loop_2(59600);
        LOW(vmc_row[next_row]);
        _delay_loop_2(50); //~5µs
        //Col3: leave high, Col2: drop 200, col3: drop 500
        if(next_col<2) {
            HIGH(vmc_row[next_row]);
            _delay_loop_2(200);
            if(next_col==0) {
                _delay_loop_2(300);
            }
            LOW(vmc_row[next_row]);
        }
        _delay_loop_2(10000); //~5µs
        HIGH(vmc_row[next_row]);
        next_col = 255;
    }
}

ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
