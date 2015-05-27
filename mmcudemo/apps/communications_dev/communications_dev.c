
#include <string.h>
#include <stdio.h>
#include "pio.h"
#include <usb_cdc.h>
#include <sys.h>
#include "pacer.h"
#include "ir_driver.h"
#include "bt_driver.h"


#define BUTTON_OFF 10
#define BUTTON_FORWARD 11
#define BUTTON_BACK 12
#define BUTTON_LEFT 13
#define BUTTON_RIGHT 14
#define BUTTON_CENTRE 15



void process_bt_command( char * string )
{
	if ( strcmp( "on", string ) == 0 )
	{
		pio_output_high(PIO_LED_Y);
	}
	else if (strcmp("off", string) == 0 )
	{
		pio_output_low(PIO_LED_Y);
	}
}


void process_ir_command (unsigned int ir_data) {
		if (ir_data == BUTTON_OFF) {
			pio_output_low(PIO_LED_G);
			pio_output_low(PIO_LED_Y);
			pio_output_low(PIO_LED_R);	
		}
		if (ir_data == BUTTON_FORWARD) {
			pio_output_toggle(PIO_LED_R);
		}
		if (ir_data == BUTTON_CENTRE) {
			pio_output_toggle(PIO_LED_Y);
		}
		if (ir_data == BUTTON_BACK) {
			pio_output_toggle (PIO_LED_G);
		}
}



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
	char* bt_data = bt_init();
	
	pacer_init (LOOP_POLL_RATE);
	
	usb_cdc_t usb_cdc;
	usb_cdc = usb_cdc_init ();
	sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);
	
	
	
	pio_output_set(PIO_AUX_ENABLE, 0);
	
		
    while (1)
    {
		/* Wait until next clock tick.  */
		pacer_wait ();
		usb_cdc_update();
		
		pio_output_set(PIO_LED_G, bt_connected());
		
		if (bt_read()) {
			process_bt_command(bt_data);
		}
		
		
		if (irCTR()) {
			process_ir_command(irRead());
		}
		

    }
}
