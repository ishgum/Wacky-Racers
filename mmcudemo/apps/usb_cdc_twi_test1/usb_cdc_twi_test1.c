/* File:   usbd_cdc_twi_test1.c
   Author: M. P. Hayes, UCECE
   Date:   27 April 2015
   Descr:  This program requires two instances communicating via TWI
   with USB CDC operating to provide a command line interface.  By default,
   the program acts as a slave but messages can be read/written as a master
   using the r and w commands.  The s command switches back to slave mode.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usb_cdc.h"
#include "pio.h"
#include "twi.h"
#include "sys.h"
#include "pacer.h"


#define PACER_RATE 1000

#define TIMEOUT_US 2000

#define SLAVE_ADDR 0x21
#define CLOCK_SPEED 40e3

#define GENERAL_LED_PIO  LED1_PIO
#define USB_LED_PIO  LED2_PIO


static const twi_cfg_t twi_cfg =
{
    .period = TWI_PERIOD_DIVISOR (CLOCK_SPEED)
};


enum {LINEBUFFER_SIZE = 80};

static int char_count = 0;
static char linebuffer[LINEBUFFER_SIZE];
static twi_mode_t mode = TWI_MODE_SLAVE;
static twi_t twi;



#define CAPTURE_LINE_SIZE 97
char camera_data[CAPTURE_LINE_SIZE] = {0};


bool read_request (char address) {
    twi_ret_t ret;
	
	ret = twi_master_addr_read_timeout (twi, SLAVE_ADDR, address, 1, camera_data,
                                            sizeof (camera_data) , TIMEOUT_US*2L);
							
	return (ret >= 0);
}

char message_in[97];


static void
process_command (void)
{
    char ch;
    uint8_t addr = 0;
    char *msg;
    char message[8];
    twi_ret_t ret;
    
    ch = fgetc (stdin);
    fputc (ch, stderr);
    if (char_count < LINEBUFFER_SIZE - 1)
        linebuffer[char_count++] = ch;

    if (ch != '\r')
        return;
    fputc ('\n', stderr);
    linebuffer[char_count++] = 0;
    char_count = 0;

    switch (linebuffer[0])
    {
    case 'w':
        mode = TWI_MODE_MASTER;

        addr = atoi (linebuffer + 1);
		msg = strchr(strchr(linebuffer + 1, ' ') + 1, ' ');
		
        msg++;
        strncpy (message, msg, sizeof (message));
        ret = twi_master_addr_write (twi, SLAVE_ADDR, addr, 1, message,
                                     sizeof (message));
        if (ret == sizeof (message))
            fprintf (stderr, "Master write %d: %s\n", addr, message);        
        else
            fprintf (stderr, "Master write %d: error %d\n", addr, ret);        
        break;

    case 'r':
        mode = TWI_MODE_MASTER;

        addr = 2;

        /* NB, this blocks while the slave gets its act together.  */
        ret = twi_master_addr_read_timeout (twi, SLAVE_ADDR, addr, 1, message_in,
                                            sizeof (message_in) , TIMEOUT_US*2L);
        if (ret == sizeof (message_in))
		{
            fprintf (stderr, "Master read %d:\n", addr);
			unsigned int i = 0;
			for (i; i < sizeof(message_in); i++)
			{
				fprintf (stderr, "%i ", message_in[i] );
			}
		}
        else
            fprintf (stderr, "Master read %d: error %d\n", addr, ret);        
        break;
	
	case 'k':
		addr = 2;
		
		bool result = read_request(addr);
		
		if (result)
		{
			fprintf (stderr, "Master read %d:\n", addr);
			unsigned int j = 0;
			for (j; j < sizeof(camera_data); j++)
			{
				fprintf (stderr, "%i ", camera_data[j] );
			}
		}
		else
			fprintf (stderr, "Master read %d: error %d\n", addr, result);

        
    default:
        break;
    }
}



int main (void)
{
    usb_cdc_t usb_cdc;

    pio_config_set (USB_LED_PIO, PIO_OUTPUT_LOW);                
    pio_config_set (GENERAL_LED_PIO, PIO_OUTPUT_HIGH);                

    twi = twi_init(&twi_cfg);

    usb_cdc = usb_cdc_init ();
    
    sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);

    /* Wait until USB configured.  */
    while (! usb_cdc_update ())
        continue;
    pio_config_set (USB_LED_PIO, PIO_OUTPUT_HIGH);                

    fprintf (stderr, "Slave listening on address %d\n", SLAVE_ADDR);

    pacer_init (PACER_RATE);
    while (1)
    {
        pacer_wait ();

        if (usb_cdc_read_ready_p (usb_cdc))
            process_command ();

        /* Check if USB disconnected.  */
        pio_output_set (USB_LED_PIO, usb_cdc_update ());
    }
}
