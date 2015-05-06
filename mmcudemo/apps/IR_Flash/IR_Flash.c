
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

#define START_BIT 1
#define HIGH_BIT 1
#define LOW_BIT 1
#define TOLERANCE 1
#define IR_BUFFER 20


int irLEDToggle = 0;
int count = 0;
bool startFound = 0;
bool differenceFound = 0;
bool readArray = 0;

tc_t tc;
tc_counter_t prev_time;
uint64_t difference = 0;
uint64_t differenceArray[IR_BUFFER] = {0};

void irInterruptHandler (void) {
	if (!readArray) {
		prev_time = tc_counter_get(tc);
		difference = (tc_counter_get (tc) - prev_time);
		
		/*
		if (abs(difference - START_BIT) < TOLERANCE) {
			startFound = 1;
			irCount = 0;
		}
		if (startFound && irCount < IR_BUFFER) {
			differenceArray[count++] = difference;
		}
		
		else {
			startFound = 0;
			readArray = 1;
		}
		*/
		
		differenceFound = 1;
		count++;
		pio_irq_clear (PA26_PIO);
	}
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
	
	usb_cdc_t usb_cdc;
	usb_cdc = usb_cdc_init ();
	sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);
	

    tc_counter_t time;
	
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

	
	
	while ((tc_counter_get (tc) - prev_time) < 100000000) {
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
		
        
		if (readArray) {
			int i = 0;
			for (; i < IR_BUFFER; i++) {
				data <<= 1;
				if ((differenceArray[i] - HIGH_BIT) < TOLERANCE) {
					data += 1;
				}
				else if ((differenceArray[i] - LOW_BIT) < TOLERANCE){
				}
				else {
					data = 0;
					break;
				}
			}
			readArray = 0;
		}
		
		if (count > 1000) {
			pio_output_toggle(PIO_LED_Y);
			count = 0;
		}
        /* Poll the IR driver.  */
        //data = ir_rc5_rx_read ();
		//printf("%u", data);
        //if (data > 0)
	    //pio_output_set(PIO_LED_Y, data % 2);
		
		
		
    }
}
