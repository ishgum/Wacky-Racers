
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pio.h"
#include <usb_cdc.h>
#include <sys.h>
#include "pacer.h"
#include "ir_driver.h"
#include "bt_driver.h"
#include "twi.h"


#define BUTTON_OFF 10
#define BUTTON_FORWARD 11
#define BUTTON_BACK 12
#define BUTTON_LEFT 13
#define BUTTON_RIGHT 14
#define BUTTON_CENTRE 15
#define BUTTON_VOLUME_UP 16
#define BUTTON_VOLUME_DOWN 17
#define BUTTON_MUTE 20

#define CAPTURE_ADDRESS 0
#define CAPTURE_LINE_SIZE 100
#define FINAL_CAPTURE_LINE 255



#define MOTOR_THROTTLE_ADDRESS 1
#define MOTOR_THROTTLE_FORWARD 145
#define MOTOR_THROTTLE_SUPER 180
#define MOTOR_THROTTLE_BACK 90
#define MOTOR_THROTTLE_TURN 143
#define MOTOR_THROTTLE_STOP 127

#define MOTOR_TURN_ADDRESS 2
#define MOTOR_TURN_LEFT 0
#define MOTOR_TURN_RIGHT 255
#define MOTOR_TURN_CENTRE 127

#define PACER_RATE 1000

#define TIMEOUT_US 2000

#define SLAVE_ADDR_M 0x42
#define SLAVE_ADDR_C 0x21

#define CLOCK_SPEED 40e3




typedef struct motor_struct_first {
	char throttle;
	char turn;
	char desired_throttle;
	char desired_turn;
	unsigned int throttle_update;
	unsigned int turn_update;
} motor_struct;







static const twi_cfg_t twi_cfg =
{
    .period = TWI_PERIOD_DIVISOR (CLOCK_SPEED)
};


typedef enum {STATE_ADDR, STATE_DATA} state_t;

enum {LINEBUFFER_SIZE = 80};

static int char_count = 0;
static char linebuffer[LINEBUFFER_SIZE];
static twi_mode_t mode = TWI_MODE_SLAVE;
static twi_t twi;
static bool read_image = 0;

motor_struct motors;


static void
process_serial_command (void)
{
    uint8_t addr = 0;
    char *msg;
    char message[16] = {0};
    twi_ret_t ret;


    switch (linebuffer[0])
    {
    case 'w':
        mode = TWI_MODE_MASTER;

        addr = atoi (linebuffer + 1);
        msg = index (linebuffer + 1, ' ');
        if (!msg)
        {
            fprintf (stderr, "Syntax error, w addr message\n");
            return;
        }
        msg++;
        strncpy (message, msg, sizeof (message));
        ret = twi_master_addr_write (twi, SLAVE_ADDR_M, addr, 1, message,
                                     sizeof (message));
        if (ret == sizeof (message))
            fprintf (stderr, "Master write %d: %s\n", addr, message);        
        else
            fprintf (stderr, "Master write %d: error %d\n", addr, ret);        
        break;

    case 'r':
        mode = TWI_MODE_MASTER;

        addr = atoi (linebuffer + 1);
        msg = index (linebuffer + 1, ' ');
        if (!msg)
        {
            fprintf (stderr, "Syntax error, r addr message\n");
            return;
        }
        msg++;
        /* NB, this blocks while the slave gets its act together.  */
        ret = twi_master_addr_read_timeout (twi, SLAVE_ADDR_M, addr, 1, message,
                                            sizeof (message) , TIMEOUT_US);
        if (ret == sizeof (message))
            fprintf (stderr, "Master read %d: %s\n", addr, message);        
        else
            fprintf (stderr, "Master read %d: error %d\n", addr, ret);        
        break;
        
    default:
        break;
    }
}




void motor_comms (uint8_t address, char byte) {
    char message[16] = {0};
    twi_ret_t ret;
	message[0] = byte;
	message[1] = 0;
	ret = twi_master_addr_write (twi, SLAVE_ADDR_M, address, 1, message,
								 sizeof (message));
	
}


bool capture_request (void) {
	char camera_message[8] = {0};
    twi_ret_t ret;
	
	ret = twi_master_addr_write (twi, SLAVE_ADDR_C, CAPTURE_ADDRESS, 1, camera_message,
                                            sizeof (camera_message));
	if (ret >= 0) {
		return 1;
	}
	else {
		return 0;
	}
}



char* read_request (char address) {
	char camera_message[CAPTURE_LINE_SIZE] = {0};
    twi_ret_t ret;
	
	ret = twi_master_addr_read_timeout (twi, SLAVE_ADDR_C, address, 1, camera_message,
                                            sizeof (camera_message) , TIMEOUT_US);
											
	if (ret >= 0) {
		return camera_message;
	}
	else {
		read_image = 0;
		return 0;
	}
}



void send_image (char* image_string, uint8_t read_address) {
	int i = 0;
	for (; i < CAPTURE_LINE_SIZE; i++) {
		bt_write(image_string[i]);
	}
	bt_write(read_address);
	bt_write('\n');
}



void process_bt_command( char * string )
{
	if (string[0] == 'M') {
		if (string[1] == 1) {
			motors.throttle_update = 0;
			motors.throttle = string[2];
		}
		if (string[1] == 2) {
			motors.turn_update = 0;
			motors.turn = string[2];
		}
	}

	if (string[0] == 'C') {
		read_image = capture_request();
	}
}


void process_ir_command (unsigned int ir_data) {
	printf("IR_Data: %u\n\r", ir_data);
	
	if (ir_data == BUTTON_FORWARD) {
		motors.throttle_update = 0;
		motors.throttle = MOTOR_THROTTLE_FORWARD;
		printf("Forward");
	}
	if (ir_data == BUTTON_BACK) {
		motors.throttle_update = 0;
		motors.throttle = MOTOR_THROTTLE_BACK;
		printf("Back");
	}
	
	if (ir_data == BUTTON_LEFT) {
		motors.turn_update = 0;
		motors.throttle_update = 0;
		motors.turn = MOTOR_TURN_LEFT;
		motors.throttle = MOTOR_THROTTLE_TURN;
		printf("Left");
	}
	if (ir_data == BUTTON_RIGHT) {
		motors.turn_update = 0;
		motors.throttle_update = 0;
		motors.turn = MOTOR_TURN_RIGHT;
		motors.throttle = MOTOR_THROTTLE_TURN;
		printf("Right");
	}
	if (ir_data == BUTTON_VOLUME_UP) {
		motors.throttle_update = 0;
		motors.throttle = MOTOR_THROTTLE_SUPER;
		printf("Super");
	}
	if (ir_data == BUTTON_MUTE && !read_image) {
		pio_output_toggle(PIO_LED_Y);
		read_image = capture_request();
		pio_output_set(PIO_LED_R, read_image);
	}
		
}


void image_poll(void) {
	static uint8_t read_address = 0;
	
	if (read_image) {
		char* read_message = read_request(read_address);
		send_image(read_message, read_address++);
	}
	else if (read_address == FINAL_CAPTURE_LINE ){
		read_image = 0;
		read_address = 0;
	}
	else{
		read_address = 0;
	}
}



void motors_init(void) {
	motors.throttle = MOTOR_THROTTLE_STOP;
	motors.desired_throttle = MOTOR_THROTTLE_STOP;
	motors.turn = MOTOR_TURN_CENTRE;
	motors.desired_turn = MOTOR_TURN_CENTRE;
	motors.throttle_update = 0;
	motors.turn_update = 0;
}

void motor_poll(void) {
	if (motors.throttle_update > 10) {
		motors.throttle = MOTOR_THROTTLE_STOP;
	}
	else {motors.throttle_update++;}
	
	if (motors.turn_update > 10) {
		motors.turn = MOTOR_TURN_CENTRE;
	}
	else {motors.turn_update++;}
	
	
	motor_comms(1, motors.throttle);
	motor_comms(2, motors.turn);
	printf("Current Values, Motor: %u, Turn: %u\n\r", motors.throttle, motors.turn);
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
	
	motors_init();
	pio_output_set(PIO_AUX_ENABLE, 0);
                
    twi = twi_init(&twi_cfg);

    usb_cdc = usb_cdc_init ();
    

    /* Wait until USB configured.  */                

    fprintf (stderr, "Slave M listening on address %d\n", SLAVE_ADDR_M);
	fprintf (stderr, "Slave C listening on address %d\n", SLAVE_ADDR_C);
	
	unsigned long tick = 0;
	
    while (1)
    {
		/* Wait until next clock tick.  */
		pacer_wait ();
		
		
		/* Check if USB disconnected.  */
		//pio_output_set (PIO_LED_R, pio_input_get(PIO_DIP_4));
		//pio_output_set (PIO_LED_Y, usb_cdc_update ());
		//pio_output_set(PIO_LED_G, bt_connected());
		
		if (pio_input_get(PIO_DIP_3)) {
			if (usb_cdc_read_ready_p (usb_cdc))
				process_serial_command ();
		}
		
		if (pio_input_get(PIO_DIP_4)) {
			if (bt_read()) {
				process_bt_command(bt_data);
			}
		}
		
		if (!pio_input_get(PIO_DIP_4)) {
			if (irCTR()) {
				process_ir_command(irRead());
			}
		}
		
		motor_poll();
		
		if (tick % 200) {
			//image_poll();
			pio_output_toggle(PIO_LED_G);
		}
		
		tick++;

    }
}
