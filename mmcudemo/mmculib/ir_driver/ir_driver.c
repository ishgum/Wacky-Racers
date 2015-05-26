
#include <stdio.h>
#include <stdlib.h>
#include "pio.h"
#include <sys.h>

#include "pacer.h"
#include "target.h"
#include "irq.h"
#include "tc.h"
#include "ir_driver.h"

#define PIO_IR PA26_PIO

#define TIMER_PIO TIOA2_PIO
#define TIMER_PRESCALE 128

#define TIMER_FREQUENCY TC_CLOCK_FREQUENCY(TIMER_PRESCALE)




static int irCount = 0;
static bool startFound = 0;
static volatile bool readArray = 0;

static tc_t tc;
static tc_counter_t prev_time;
static uint64_t difference = 0;
static uint64_t differenceArray[IR_BUFFER] = {0};


static unsigned long button[] = {OFF_BUTTON, UP_BUTTON, DOWN_BUTTON, LEFT_BUTTON, RIGHT_BUTTON, CENTRE_BUTTON}


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


void
irInterruptInit(void) {
	pio_irq_config_set (PA26_PIO, PIO_IRQ_FALLING_EDGE);
	irq_config (PIO_ID(PA26_PIO), 1, irInterruptHandler);
	irq_enable (PIO_ID(PA26_PIO));
    pio_irq_enable (PA26_PIO);
}

void irInit(void) {
	pio_config_set (PA26_PIO, PIO_INPUT); 
	pio_init(PA26_PIO);
	
	tc = tc_init (&tc_cfg);
	if (!tc) 
    {
        /* This will fail for an invalid choice of PIO for tc.  */
        while (1)
           continue;
    }
    tc_start (tc);
	
	prev_time = tc_counter_get (tc);
	irInterruptInit();
}


bool irCTR (void) {
	return readArray;
}

void irClear (void) {
	readArray = 0;
}


unsigned long irRead (void)
{		
	
	unsigned long data = 0;
	int j = 0;
	
	for (; j < IR_BUFFER; j++) {
		data <<= 1;
		if (abs(HIGH_BIT - differenceArray[j]) < TOLERANCE) {
			data += 1;
		}
		else if (abs(LOW_BIT - differenceArray[j]) < TOLERANCE){
		}
		else {
			printf("Got some bad data at %u", j);
			return 128;
		}
	}		
	int i = 0;
	for (; i < NUM_BUTTONS; i++) {
		if (data == button[i]) {
			return i;
		}
	}
	return 128;
}

