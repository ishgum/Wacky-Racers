
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
#define TIMER_PRESCALE 128

#define TIMER_FREQUENCY TC_CLOCK_FREQUENCY(TIMER_PRESCALE)

#define START_BIT 6800
#define HIGH_BIT 1690
#define LOW_BIT 850
#define TOLERANCE 100
#define IR_BUFFER 32

#define OFF_BUTTON 3772793023
#define UP_BUTTON 3772778233
#define DOWN_BUTTON 3772810873
#define LEFT_BUTTON 3772819033
#define RIGHT_BUTTON  3772794553
#define CENTRE_BUTTON 3772782313


int irCount = 0;
bool startFound = 0;
bool readArray = 0;

tc_t tc;
tc_counter_t prev_time;
uint64_t difference = 0;
uint64_t differenceArray[IR_BUFFER] = {0};

void irInterruptHandler (void) {
	if (!readArray) {
		difference = (tc_counter_get (tc) - prev_time);
		prev_time = tc_counter_get(tc);
		
		
		if (abs(START_BIT - difference) < TOLERANCE) {
		startFound = 1;
			irCount = 0;
		}
		else if (startFound && irCount < IR_BUFFER) {
			differenceArray[irCount++] = difference;
		}
		
		else if (irCount == IR_BUFFER){
			startFound = 0;
			readArray = 1;
		}
	}
	pio_irq_clear (PA26_PIO);

}


static const tc_cfg_t tc_cfg =
{
    /* The PIO is not used as an input or output but must be specified
       to select the desired channel.  */
    .pio = TIMER_PIO,
    .mode = TC_MODE_COUNTER,
    .prescale = TIMER_PRESCALE,
	//.period = F_CPU / 1000000
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
	pio_irq_config_set (PA26_PIO, PIO_IRQ_FALLING_EDGE);
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

	
	
	/*while ((tc_counter_get (tc) - prev_time) < 100000000) {
		continue;
	}
	*/
	pio_output_high(PIO_LED_G);
	
    pacer_init (LOOP_POLL_RATE);
	
	short aux_power = 0;
	unsigned long data = 0;
	unsigned long finalData = 0;
	unsigned long previousData = 0;
		
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
			

		}
		
		if (usb_cdc_update()){
			if (readArray) {
				int i = 0;
				for (; i < IR_BUFFER; i++) {
					printf("%llu\n\r", differenceArray[i]);
				}
			}
		}
        
		if (readArray) {
			int j = 0;
			data = 0;
			for (; j < IR_BUFFER; j++) {
				data <<= 1;
				if (abs(HIGH_BIT - differenceArray[j]) < TOLERANCE) {
					data += 1;
				}
				else if (abs(LOW_BIT - differenceArray[j]) < TOLERANCE){
				}
				else {
					printf("Got some bad data at %u", j);
					data = 0;
					break;
				}
			}
			printf("Data: %lu\n\r", data);
		}
		
		if (data != previousData) {
			finalData = data;
		}
		else {
			finalData = 0;
		}
		
		
		if (finalData == OFF_BUTTON) {
			pio_output_low(PIO_LED_G);
			pio_output_low(PIO_LED_Y);
			pio_output_low(PIO_LED_R);	
		}
		if (finalData == UP_BUTTON) {
			pio_output_toggle(PIO_LED_R);
		}
		if (finalData == CENTRE_BUTTON) {
			pio_output_toggle(PIO_LED_Y);
		}
		if (finalData == DOWN_BUTTON) {
			pio_output_toggle(PIO_LED_G);
		}


		
		readArray = 0;
		
    }
}
