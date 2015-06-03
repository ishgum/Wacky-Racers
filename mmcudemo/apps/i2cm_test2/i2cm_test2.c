/* File:   i2cm_test1.c
   Author: M. P. Hayes, UCECE
   Date:   13 April 2013
   Descr: 
*/
#include "i2c_master.h"
#include "pacer.h"
#include <stdlib.h>

#define SDA_PIO PA3_PIO
#define SCL_PIO PA4_PIO

#define PACER_RATE 10

#define SLAVE_ADDR 0x32
#define SLAVE2_ADDR 0x33

static const i2c_bus_cfg_t i2c_bus_cfg =
{
    .scl = SCL_PIO,
    .sda = SDA_PIO
};



int
main (void)
{
	pio_config_set (PIO_LED_R, PIO_OUTPUT_LOW);
	pio_config_set (PIO_LED_Y, PIO_OUTPUT_LOW);
	pio_config_set (PIO_LED_G, PIO_OUTPUT_LOW);
	
	
    i2c_t i2c_slave1;
    i2c_slave1 = i2c_master_init (&i2c_bus_cfg);
	
	i2c_t i2c_slave2;
    i2c_slave2 = i2c_master_init (&i2c_bus_cfg);

    pacer_init (PACER_RATE);

    while (1)
    {
        uint8_t tx[] = {1,2};
		uint8_t tx2[] = {1,2};
        uint8_t rx[] = {0,1,1};
        i2c_addr_t addr = 1;

        pacer_wait ();
		
		tx[0] = rand() % 256;
		tx[1] = rand() % 256;
		
		tx2[0] = rand() % 256;
		tx2[1] = rand() % 256;
		
        i2c_master_addr_write (i2c_slave1, SLAVE_ADDR, addr, 1, tx, sizeof(tx));
		i2c_master_addr_write (i2c_slave2, SLAVE2_ADDR, addr, 1, tx2, sizeof(tx2));

        //i2c_master_addr_read (i2c_slave1, SLAVE_ADDR, addr, 1, rx, sizeof(rx));
		
		pio_output_set(PIO_LED_G, rx[0]);
		pio_output_set(PIO_LED_Y, rx[1]);
		pio_output_set(PIO_LED_R, rx[2]);
        /* TODO: check if rx matches tx.  */
    }
}

