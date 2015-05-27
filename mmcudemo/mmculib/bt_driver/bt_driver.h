/** @file   ir_driver.h
    @author Michael McAdam
    @date   26 May 2015
    @brief 
*/
#ifndef BT_DRIVER
#define BT_DRIVER





char* bt_init(void); 

void bt_write(char ch);

bool bt_connected(void);

bool bt_read(void);

#endif