BT_DRIVER_DIR = $(DRIVER_DIR)/bt_driver

VPATH += $(BT_DRIVER_DIR)
SRC += bt_driver.c

DRIVERS += busart

INCLUDES += -I$(BT_DRIVER_DIR)

