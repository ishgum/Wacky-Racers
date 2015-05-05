
#include <string.h>
#include <stdio.h>
#include "pio.h"
#include <usb_cdc.h>
#include <sys.h>

#include "pacer.h"
//#include "ir_rc5_rx.h"
#include "target.h"
#include "irq.h"
#include "tc.h"

#define PIO_IR PA26_PIO

#define TIMER_PIO TIOA2_PIO
#define TIMER_PRESCALE 2

#define TIMER_FREQUENCY TC_CLOCK_FREQUENCY(TIMER_PRESCALE)


int irLEDToggle = 0;
int count = 0;


void irInterruptHandler (void) {
	pio_irq_clear (PA26_PIO);
	count++;
}


static const tc_cfg_t tc_cfg =
{
    /* The PIO is not used as an input or output but must be specified
       to select the desired channel.  */
    .pio = TIMER_PIO,
    .mode = TC_MODE_COUNTER,
    .prescale = TIMER_PRESCALE,
	.period = F_CPU / 1000000
};



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

void
interruptInit(void) {
	pio_irq_config_set (PA26_PIO, PIO_IRQ_ANY_EDGE);
	irq_config (PIO_ID(PA26_PIO), 1, irInterruptHandler);
	irq_enable (PIO_ID(PA26_PIO));
    pio_irq_enable (PA26_PIO);
}




/* Define how fast ticks occur.  This must be faster than
   TICK_RATE_MIN.  */
enum {LOOP_POLL_RATE = 200};

int main (void)
{		
	init_pins();
	//ir_rc5_rx_init ();
	
	tc_t tc;
    tc_counter_t time;
    tc_counter_t prev_time;
	
	tc = tc_init (&tc_cfg);
    if (!tc) 
    {
        /* This will fail for an invalid choice of PIO for tc.  */
        while (1)
           continue;
    }
    tc_start (tc);
	
	prev_time = tc_counter_get (tc);
	
	interruptInit();

	
	
	while ((tc_counter_get (tc) - prev_time) < 1000000) {
		continue;
	}
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
