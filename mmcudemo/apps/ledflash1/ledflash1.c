
#include <string.h>
#include <stdio.h>
#include <pio.h>
#include <usb_cdc.h>
#include <sys.h>

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

#define BUFFER_SIZE 20
usb_cdc_t usb_cdc;
void init_usb(void)
{
	usb_cdc = usb_cdc_init ();
	sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);
}

void wait_for_usb(void)
{
	while (! usb_cdc_update ()) continue;
}

/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};

int main (void)
{		
	init_pins();
	init_usb();
	
	pio_output_high(PIO_LED_G);
	
    pacer_init (LOOP_POLL_RATE);
	
	short aux_power = 0;
    while (1)
    {
		/* Wait until next clock tick.  */
		pacer_wait ();
		
		pio_output_set (PIO_LED_R, usb_cdc_update ());
		
		short state;
		state = !pio_input_get(PIO_DIP_4);
		if (aux_power != state)
		{
			aux_power = state;
			pio_output_set(PIO_AUX_ENABLE, !aux_power);

		}
		
		/*
		while (usb_cdc_read_ready_p (usb_cdc))
        {
			char ch = fgetc (stdin);
			fputc (ch, stdout);
			pio_output_toggle(PIO_LED_Y);
		}
		*/
		
		static char ch_buffer[BUFFER_SIZE] = {0};
		static int ch_count = 0;
		
		if ( usb_cdc_update() )
		{
			while (usb_cdc_read_ready_p (usb_cdc))
			{
				ch_buffer[ch_count] = usb_cdc_getc(usb_cdc);
				if (ch_buffer[ch_count] == '\n')
				{
					ch_buffer[ch_count] = 0;
					if (strcmp("on", &ch_buffer) == 0)
					{
						pio_output_high(PIO_LED_Y);
					}
					if (strcmp("off", &ch_buffer) == 0)
					{
						pio_output_low(PIO_LED_Y);
					}
					if (strcmp("query", &ch_buffer) == 0)
					{
						usb_write (usb_cdc->usb, "ok\n", 3);
						usb_write (usb_cdc->usb, "", 0);
						//printf("ok\n");
					}
					else
					{
						char ch2 = atoi(&ch_buffer);
						putc(ch2,stdout);
					}
					ch_count = 0;
				}
				else if (++ch_count == BUFFER_SIZE)
				{
					ch_count = 0;
				}
			}
			
		}
    }
}
