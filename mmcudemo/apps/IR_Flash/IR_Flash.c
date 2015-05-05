
#include <string.h>
#include <stdio.h>
#include <pio.h>
#include <usb_cdc.h>
#include <sys.h>

#include "pacer.h"
#include "ir_rc5_rx.h"
#include "target.h"
#include "irq.h"

#define PIO_IR PA26_PIO

int irLEDToggle = 0;
int count = 0;


void irInterruptHandler (void) {
	pio_irq_clear (PA26_PIO);
	count++;
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
	
	pio_config_set (PA26_PIO, PIO_INPUT); pio_init(PA26_PIO);
	
	pio_config_set (PIO_AUX_ENABLE, PIO_OUTPUT_HIGH); 
}




/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};

int main (void)
{		
	init_pins();
	ir_rc5_rx_init ();
	
	
	pio_irq_config_set (PA26_PIO, PIO_IRQ_ANY_EDGE);
	irq_config (PIO_ID(PA26_PIO), 1, irInterruptHandler);
	irq_enable (PIO_ID(PA26_PIO));
    pio_irq_enable (PA26_PIO);

	
	pio_output_high(PIO_LED_G);
	
    pacer_init (LOOP_POLL_RATE);
	
	short aux_power = 0;
    while (1)
    {
		int16_t data = 0;
		/* Wait until next clock tick.  */
		pacer_wait ();
		
		
		short state;
		state = !pio_input_get(PIO_DIP_4);
		if (aux_power != state)
		{
			aux_power = state;
			pio_output_set(PIO_AUX_ENABLE, !aux_power);
			

		}
        
		if (count > 100) {
			pio_output_toggle(PIO_LED_Y);
			count = 0;
		}
        /* Poll the IR driver.  */
        //data = ir_rc5_rx_read ();
        //if (data > 0)
	    //pio_output_set(PIO_LED_Y, data % 2);
		
		
		
    }
}
