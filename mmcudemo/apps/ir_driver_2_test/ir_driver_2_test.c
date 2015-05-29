
#include <string.h>
#include <stdio.h>
#include "pio.h"
#include <usb_cdc.h>
#include <sys.h>
#include "pacer.h"
#include "ir_driver_2.h"


#define BUTTON_OFF 0
#define BUTTON_FORWARD 1
#define BUTTON_BACK 2
#define BUTTON_LEFT 3
#define BUTTON_RIGHT 4
#define BUTTON_CENTRE 5


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



/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};

int main (void)
{		
	init_pins();
	irInit();
	//ir_rc5_rx_init ();
	
	usb_cdc_t usb_cdc;
	usb_cdc = usb_cdc_init ();
	sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);
	


	
	
	/*while ((tc_counter_get (tc) - prev_time) < 100000000) {
		continue;
	}
	*/
	pio_output_high(PIO_LED_G);
	
    pacer_init (LOOP_POLL_RATE);
	
	short aux_power = 0;
	unsigned long finalData = 128;
		
    while (1)
    {
		/* Wait until next clock tick.  */
		pacer_wait ();
		finalData = 128;
		
		short state;
		state = !pio_input_get(PIO_DIP_4);
		if (aux_power != state)
		{
			aux_power = state;
			pio_output_set(PIO_AUX_ENABLE, !aux_power);
			

		}
		
		if (usb_cdc_update()){
			
		}
		
		if (irCTR()) {
			finalData = irRead();
		}
		
		if (finalData == BUTTON_OFF) {
			pio_output_low(PIO_LED_G);
			pio_output_low(PIO_LED_Y);
			pio_output_low(PIO_LED_R);	
		}
		if (finalData == BUTTON_FORWARD) {
			pio_output_toggle(PIO_LED_R);
		}
		if (finalData == BUTTON_CENTRE) {
			pio_output_toggle(PIO_LED_Y);
		}
		if (finalData == BUTTON_BACK) {
			pio_output_toggle(PIO_LED_G);
		}

    }
}
