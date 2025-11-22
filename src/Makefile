PROJECT = main
BUILD_DIR = bin

OPENCM3_DIR=../libopencm3
DEVICE=stm32f411ce


CFILES = main.c \
services/pots_service.c \
services/buttons_service.c \
services/display_service.c \
services/tuner_service.c \
services/led_service.c \
services/battery_service.c \
hardware/hc4053_hal.c \
hardware/usart_hal.c \
hardware/hc05_hal.c \
hardware/midiusb_hal.c \
hardware/i2c_hal.c \
hardware/buttons_hal.c \
hardware/adc_hal.c \
hardware/timer_hal.c \
middleware/hc4053_middleware.c \
middleware/midibt_middleware.c \
middleware/midiusb_middleware.c \
dsp/dsp.c \
dsp/fir_coeffs.c \
dsp/notes.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/list.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/queue.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/tasks.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/timers.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/event_groups.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/MemMang/heap_4.c \
libs/ssd1306-libopencm3/ssd1306_libopencm3/ssd1306.c \
libs/ssd1306-libopencm3/ssd1306_libopencm3/ssd1306_fonts.c

CMSISDSP_LIB = libs/build/libCMSISDSP.a

LDLIBS += $(CMSISDSP_LIB) -lm

$(PROJECT).elf: $(CMSISDSP_LIB)

 $(CMSISDSP_LIB):
	$(MAKE) -C libs/




# OOCD_FILE = board/stm32f4discovery.cfg
# VPATH += $(SHARED_DIR)
# INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))

INCLUDES += -Idsp
INCLUDES += -Ihardware
INCLUDES += -Imiddleware
INCLUDES += -Iservices
INCLUDES += -Ilibs/CMSIS-DSP/Include
INCLUDES += -Ilibs/CMSIS-DSP/Include/dsp
INCLUDES += -Ilibs/CMSIS_6/CMSIS/Core/Include
INCLUDES += -I.

INCLUDES += -I ../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/
INCLUDES += -I ../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/include/
INCLUDES += -I libs/ssd1306-libopencm3/ssd1306_libopencm3/

#CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

PHONY: all
.DEFAULT_GOAL := all
all: $(PROJECT).elf $(PROJECT).bin

flashbin:
	st-flash write main.bin 0x8000000
