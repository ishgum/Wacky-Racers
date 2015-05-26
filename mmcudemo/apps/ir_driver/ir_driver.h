/** @file   ir_driver.h
    @author Michael McAdam
    @date   26 May 2015
    @brief 
*/
#ifndef IR_DRIVER
#define IR_DRIVER

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

#define NUM_BUTTONS 6

void irInit(void); 


bool irCTR (void);

void irClear (void);


unsigned long irRead (void);
void irInterruptInit(void);
#endif