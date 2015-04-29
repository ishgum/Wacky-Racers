#include <stdio.h>
#include "usb_cdc.h"
#include "pio.h"
#include "sys.h"
#include "pacer.h"


#define PACER_RATE 1000


int main (void)
{
    usb_cdc_t usb_cdc;

    pio_config_set (PIO_LED_G, PIO_OUTPUT_LOW);                
    pio_config_set (PIO_LED_Y, PIO_OUTPUT_LOW);                

    usb_cdc = usb_cdc_init ();
    
    sys_redirect_stdin ((void *)usb_cdc_read, usb_cdc);
    sys_redirect_stdout ((void *)usb_cdc_write, usb_cdc);
    sys_redirect_stderr ((void *)usb_cdc_write, usb_cdc);


    /* Wait until USB configured.  */
    while (! usb_cdc_update ())
        continue;

    pacer_init (PACER_RATE);

    printf ("Hello world!\n");

    while (1)
    {
        pacer_wait ();

        if (usb_cdc_read_ready_p (usb_cdc))
        {
            char ch;

            ch = fgetc (stdin);
            fputc (ch, stderr);
            if (ch == '\r')
                fputc ('\n', stderr);

            switch (ch)
            {
            case '0':
                pio_output_set (PIO_LED_Y, 0);
                break;

            case '1':
                pio_output_set (PIO_LED_Y, 1);
                break;

            default:
                break;
            }
        }

        /* Check if USB disconnected.  */
        pio_output_set (PIO_LED_G, usb_cdc_update ());
    }
}
