#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/audio.h>
#include <libopencm3/usb/midi.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

extern usbd_device *usbd_dev;

// if you board does not have VBUS pin connected(like blackpill) you will have to add these defines
#define USB_OTG_FS_BASE		(PERIPH_BASE_AHB2 + 0x00000)
#define OTG_GCCFG		0x038

#define OTG_FS_GCCFG		MMIO32(USB_OTG_FS_BASE + OTG_GCCFG)
#define OTG_GCCFG_NOVBUSSENS	(1 << 21)

void usb_setup(usbd_device *dev, uint16_t wValue);

void otg_fs_isr(void);

void usb_send_noteOn(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel);
void usb_send_noteOff(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel);
void usb_send_cc(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel);

void usbMidiInit(void);

/* Buffer to be used for control requests. */
extern uint8_t usbd_control_buffer[128];

