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
	
	pio_config_set (PIO_H_1, PIO_PULLUP); pio_init(PIO_H_1);
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
	//pio_config_set (PIO_BT_RTS, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_CTS, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_RX, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_TX, PIO_OUTPUT_LOW);
}

void bt_pins_on( void )
{
	pio_config_set (PIO_BT_RESET, PIO_OUTPUT_HIGH);
	pio_config_set (PIO_BT_DISCOVER, PIO_OUTPUT_HIGH);
	//pio_config_set (PIO_BT_RTS, PIO_PULLDOWN);
	//pio_config_set (PIO_BT_CTS, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_RX, PIO_OUTPUT_LOW);
	//pio_config_set (PIO_BT_TX, PIO_INPUT);
}

void bt_pins_init( void )
{
	bt_pins_off();
	//pio_init(PIO_BT_TX);
	pio_init(PIO_BT_RTS);
}


/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};
//enum {LOOP_POLL_RATE = 100000};


#define BUFFER_SIZE 64
char tx0buffer[BUFFER_SIZE] = {0};
char rx0buffer[BUFFER_SIZE] = {0};
char tx1buffer[BUFFER_SIZE] = {0};
char rx1buffer[BUFFER_SIZE] = {0};


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
	
	/*
	busart_t busart0;
    busart0 = busart_init (0, BUSART_BAUD_DIVISOR (9600),
                          tx0buffer, sizeof (tx0buffer),
                          rx0buffer, sizeof (rx0buffer));
	
	busart_t busart1;
    busart1 = busart_init (1, BUSART_BAUD_DIVISOR (9600),
                          tx1buffer, sizeof (tx1buffer),
                          rx1buffer, sizeof (rx1buffer));
	*/
	
	pio_output_high(PIO_LED_G);
    pacer_init (LOOP_POLL_RATE);
	
	short aux_power = 0;
    while (1)
    {		
		/* Wait until next clock tick.  */
		pacer_wait ();
		
		short state;
		state = !pio_input_get(PIO_DIP_4);
		if (aux_power != state)
		{
			aux_power = state;
			pio_output_set(PIO_AUX_ENABLE, !aux_power);
			
			if (aux_power)
			{
				bt_pins_on();
				//busart_putc (busart, '$');
				//busart_putc (busart, '$');
				//busart_putc (busart, '$');
			}
			else
				bt_pins_off();
			
		}
		
		/*
		while (busart_read_ready_p(busart0))
		{
			char ch;
			ch = busart_getc (busart0);
			busart_putc (busart0, ch);
			pio_output_toggle(PIO_LED_Y);
		}
		while (busart_read_ready_p(busart1))
		{
			char ch;
			ch = busart_getc (busart1);
			busart_putc (busart1, ch);
			pio_output_toggle(PIO_LED_R);
		}
		*/
		
		
		static int ch_count = 0;
		if ( usb_cdc_update() )
		{
			while (usb_cdc_read_ready_p (usb_cdc))
			{
				ch_buffer[ch_count] = usb_cdc_getc(usb_cdc);
				if (ch_buffer[ch_count] == '\n' || ch_buffer[ch_count] == '\r' || ch_buffer[ch_count] == 0)
				{
					ch_buffer[ch_count] = 0;
					if (strcmp("on", &ch_buffer) == 0)
					{
						pio_output_high(PIO_LED_Y);
					}
					else if (strcmp("off", &ch_buffer) == 0)
					{
						pio_output_low(PIO_LED_Y);
					}
					else if (strcmp("yo", &ch_buffer) == 0)
					{
						//usb_write (usb_cdc->usb, "ok\n", 3);
						//usb_write (usb_cdc->usb, "", 0);
						int result = udp_write(usb_cdc->usb->udp, "ok\n",3);
						pio_output_set(PIO_LED_Y, result <= 0 );
						//udp_endpoint_reset(usb_cdc->usb->udp, UDP_EP_OUT);
					}
					ch_count = 0;
				}
				else if (++ch_count == BUFFER_SIZE)
				{
					ch_count = 0;
				}
			}
			pio_output_high(PIO_LED_R);
			
		}
		else
		{
			pio_output_low(PIO_LED_R);
		}
		
    }
}