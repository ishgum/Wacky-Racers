#include <string.h>
#include <stdio.h>
#include <pio.h>
#include <busart.h>
#include <sys.h>

#include "bt_driver.h"
#include "target.h"





void process_command( char * string )
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


#define BUFFER_SIZE 64
static char tx0buffer[BUFFER_SIZE] = {0};
static char rx0buffer[BUFFER_SIZE] = {0};

static unsigned int ch_count = 0;
static char ch_buffer[BUFFER_SIZE] = {0};
char read_bt_string[BUFFER_SIZE] = {0};
static busart_t busart0;

char* init_bt(void) {
	pio_init(PIO_BT_CONNECTED);
	busart0 = busart_init (0, BUSART_BAUD_DIVISOR (115200),
                          tx0buffer, sizeof (tx0buffer),
                          rx0buffer, sizeof (rx0buffer));
	return read_bt_string;
}





void bt_write(char ch) {
	busart_putc (busart0, ch);
}



bool bt_connected(void) {
	return pio_input_get(PIO_BT_CONNECTED);
}





bool bt_read(void) {			
	while ( busart_read_ready_p(busart0) )
	{
		char ch = busart_getc(busart0);
	
		if (ch == '\n') {
			ch = 0;
			ch_buffer[ch_count] = ch;
			strcpy(read_bt_string, ch_buffer);
			ch_count = 0;
			return 1;
		}
		
		else {
			ch_buffer[ch_count++] = ch;
		}
		
		if(ch_count == BUFFER_SIZE)
		{
			ch_count = 0;
		}
	}
	return 0;
}
