PROJECT = main
BUILD_DIR = bin

OPENCM3_DIR=../libopencm3
DEVICE=stm32f411ce


CFILES = main.c \
midiusb.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/list.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/queue.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/tasks.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/timers.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/event_groups.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c \
../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/MemMang/heap_4.c \
libs/ssd1306-libopencm3/ssd1306_libopencm3/ssd1306.c \
libs/ssd1306-libopencm3/ssd1306_libopencm3/ssd1306_fonts.c

#CMSISDSP_LIB = libs/cmsis_dsp/Source/build/libCMSISDSP.a

# $(PROJECT).elf: $(CMSISDSP_LIB)

# $(CMSISDSP_LIB):
# 	$(MAKE) -C libs/cmsis_dsp/Source




# OOCD_FILE = board/stm32f4discovery.cfg
# VPATH += $(SHARED_DIR)
# INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))

# INCLUDES += -Ilibs/cmsis_dsp/Include
# INCLUDES += -Ilibs/cmsis_dsp/Include/dsp
# INCLUDES += -Ilibs/cmsis_dsp/Core/Include
INCLUDES += -I.

INCLUDES += -I ../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/
INCLUDES += -I ../FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/include/
INCLUDES += -I libs/ssd1306-libopencm3/ssd1306_libopencm3/
LDLIBS += $(CMSISDSP_LIB) -lm
#CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

$(PROJECT).bin: $(PROJECT).elf
	arm-none-eabi-objcopy -Obinary $< $@
bin: $(PROJECT).bin

flashbin:
	st-flash write main.bin 0x8000000
