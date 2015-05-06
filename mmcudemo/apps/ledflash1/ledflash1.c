#include <string.h>
#include <stdio.h>
#include <pio.h>
#include <busart.h>
#include <sys.h>
#include <usb_cdc.h>

#include "target.h"
#include "pacer.h"


void init_pins( void )
{
	pio_config_set (PIO_LED_R, PIO_OUTPUT_LOW);
	pio_config_set (PIO_LED_Y, PIO_OUTPUT_LOW);
	pio_config_set (PIO_LED_G, PIO_OUTPUT_LOW);
	
	pio_config_set (PIO_H_1, PIO_OUTPUT_LOW);
	pio_config_set (PIO_H_2, PIO_OUTPUT_LOW);
	pio_config_set (PIO_H_3, PIO_OUTPUT_LOW);
	pio_config_set (PIO_H_4, PIO_OUTPUT_LOW);
	
	pio_config_set (PIO_DIP_1, PIO_PULLUP); pio_init(PIO_DIP_1);
	pio_config_set (PIO_DIP_2, PIO_PULLUP); pio_init(PIO_DIP_2);
	pio_config_set (PIO_DIP_3, PIO_PULLUP); pio_init(PIO_DIP_3);
	pio_config_set (PIO_DIP_4, PIO_PULLUP); pio_init(PIO_DIP_4);
	pio_config_set (PIO_SW_SLEEP, PIO_PULLUP); pio_init(PIO_SW_SLEEP);
	
	pio_config_set (PIO_AUX_ENABLE, PIO_OUTPUT_HIGH); 
}


void bt_pins_off( void )
{
	pio_config_set (PIO_BT_RESET, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_DISCOVER, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_RECONNECT, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_CONNECTED, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_RTS, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_CTS, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_RX, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_TX, PIO_OUTPUT_LOW);
}

void bt_pins_on( void )
{
	pio_config_set (PIO_BT_RESET, PIO_OUTPUT_HIGH);
	pio_config_set (PIO_BT_DISCOVER, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_RECONNECT, PIO_OUTPUT_LOW);
	pio_config_set (PIO_BT_CONNECTED, PIO_PULLDOWN);
	pio_config_set (PIO_BT_RTS, PIO_PULLDOWN);
	pio_config_set (PIO_BT_CTS, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_RX, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_TX, PIO_INPUT);
}

void bt_pins_init( void )
{
	bt_pins_off();
	//pio_init(PIO_BT_TX);
	pio_init(PIO_BT_RTS);
	pio_init(PIO_BT_CONNECTED);
}


/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};


#define BUFFER_SIZE 64
char tx0buffer[BUFFER_SIZE] = {0};
char rx0buffer[BUFFER_SIZE] = {0};

char ch_buffer[BUFFER_SIZE] = {0};

int main (void)
{		
	init_pins();
	bt_pins_init();

	
	usb_cdc_t usb_cdc;
	usb_cdc = usb_cdc_init ();
    sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);
	
	busart_t busart0;
    busart0 = busart_init (0, BUSART_BAUD_DIVISOR (115200),
                          tx0buffer, sizeof (tx0buffer),
                          rx0buffer, sizeof (rx0buffer));
	
    pacer_init (LOOP_POLL_RATE);
	
	short bt_connection = 0;
	short aux_power = 0;
    while (1)
    {		
		/* Wait until next clock tick.  */
		pacer_wait ();
		
		if (!pio_input_get(PIO_DIP_1))
		{
			pio_output_low(PIO_LED_Y);
		}
		static last = 0;
		if (!pio_input_get(PIO_DIP_2))
		{
			if(!last)
			{
				busart_putc (busart0, '$');
				busart_putc (busart0, '$');
				busart_putc (busart0, '$');
				last = 1;
			}
		}
		else
			last = 0;
		
		short state;
		state = !pio_input_get(PIO_DIP_4);
		if (aux_power != state)
		{
			aux_power = state;
			pio_output_set(PIO_AUX_ENABLE, !aux_power);
			
			if (aux_power)
			{
				bt_pins_on();
			}
			else
			{
				bt_pins_off();
				bt_connection = 0;
				pio_output_set( PIO_LED_G, bt_connection );
			}
		}
		if (aux_power)
		{
			bt_connection = pio_input_get(PIO_BT_CONNECTED);
			pio_output_set( PIO_LED_G, bt_connection );
			
			while (busart_read_ready_p(busart0))
			{
				char ch;
				ch = busart_getc (busart0);
				usb_cdc_putc(usb_cdc, ch);
				pio_output_high(PIO_LED_Y);
			}
			
			if ( usb_cdc_update() )
			{
				while (usb_cdc_read_ready_p (usb_cdc))
				{
					char ch = usb_cdc_getc(usb_cdc);
					if (ch == '\r')
						ch = '\n';
					busart_putc (busart0, ch);
				}	
			}
		}
		else
		{
			static int ticks = 0;
			if (ticks++ == 100)
			{
				ticks = 0;
				busart_putc (busart0, '?');
			}
		}
    }
}