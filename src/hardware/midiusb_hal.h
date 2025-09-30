#ifndef MIDIUSB_HAL_H
#define MIDIUSB_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/audio.h>
#include <libopencm3/usb/midi.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


#define MANUFACTURER "Rafael Bormann Chuede"
#define PRODUCT_NAME "MIDI FOO"
#define PRODUCT_SERIAL "AHSM00003\0"

#define USB_OTG_FS_BASE		(PERIPH_BASE_AHB2 + 0x00000)
#define OTG_GCCFG		0x038

#define OTG_FS_GCCFG		MMIO32(USB_OTG_FS_BASE + OTG_GCCFG)
#define OTG_GCCFG_NOVBUSSENS	(1 << 21)

extern usbd_device *usbd_dev;

extern bool usb_configured;

void midiusb_hal_init(void);

bool usb_is_ready(void);

int midiusb_hal_write(const void *buf, uint16_t len);

#endif
