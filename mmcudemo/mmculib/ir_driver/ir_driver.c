
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

//These number are for a Samsung remote only!
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
#define ONE_BUTTON 3772784863
#define TWO_BUTTON 3772817503
#define THREE_BUTTON 3772801183
#define FOUR_BUTTON 3772780783
#define FIVE_BUTTON 3772813423
#define SIX_BUTTON 3772797103
#define SEVEN_BUTTON 3772788943
#define EIGHT_BUTTON 3772821583
#define NINE_BUTTON 3772805263
#define ZERO_BUTTON  3772811383
#define VOLUME_UP_BUTTON 3772833823
#define VOLUME_DOWN_BUTTON 3772829743
#define CHANNEL_UP_BUTTON 3772795063
#define CHANNEL_DOWN_BUTTON 3772778743
#define MUTE_BUTTON 3772837903

#define NUM_BUTTONS 6

static int irCount = 0;
static bool startFound = 0;
static volatile bool readArray = 0;

static tc_t tc;
static tc_counter_t prev_time;
static uint64_t difference = 0;
static uint64_t differenceArray[IR_BUFFER] = {0};


static unsigned long button[] = {ZERO_BUTTON,		//0
								 ONE_BUTTON,		//1
								 TWO_BUTTON,		//2
								 THREE_BUTTON,		//3
								 FOUR_BUTTON,		//4
								 FIVE_BUTTON,		//5
								 SIX_BUTTON,		//6
								 SEVEN_BUTTON,		//7
								 EIGHT_BUTTON,		//8
								 NINE_BUTTON,		//9
								 OFF_BUTTON,		//10
								 UP_BUTTON, 		//11
								 DOWN_BUTTON, 		//12
								 LEFT_BUTTON, 		//13
								 RIGHT_BUTTON, 		//14
								 CENTRE_BUTTON,		//15								 
								 VOLUME_UP_BUTTON,	//16
								 VOLUME_DOWN_BUTTON,//17
								 CHANNEL_UP_BUTTON, //18
								 CHANNEL_DOWN_BUTTON,//19
								 MUTE_BUTTON};		//20


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
		
		if (startFound && irCount == IR_BUFFER){
			startFound = 0;
			irCount = 0;
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
	irCount = 0;
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
			return 127;
		}
	}	
	printf("Final: %lu\n\r", data);  	// Debug
	irClear();
	
	int i = 0;
	for (; i < NUM_BUTTONS; i++) {
		if (data == button[i]) {
			return i;
		}
	}
	return 128;
}

