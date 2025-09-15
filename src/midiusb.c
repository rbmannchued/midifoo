#include "midiusb.h"

usbd_device *usbd_dev;
uint8_t usbd_control_buffer[128];

static const struct usb_device_descriptor dev_descr = {
  /* Type: uint8_t   Size: 1   
   * Description: Size of this descriptor in bytes
   */
  .bLength = USB_DT_DEVICE_SIZE,

  /* Type: uint8_t   Size: 1   
   * Descriptor: Device Descriptor Type = 1
   */
  .bDescriptorType = USB_DT_DEVICE,  

  /* Type: uint16_t   Size: 2   
   * Description: This field identifies the release of the USB 
   * Specification with which the device and its descriptors are 
   * compliant. 
   */
  .bcdUSB = 0x0200,

  /* Type: uint8_t   Size: 1   
   * Description: Class code (assigned by the USB-IF)   0 = each 
   * interface within a configuration specifies its own class information 
   * and the various interfaces operate independently.
   */
  .bDeviceClass = 0,

  /* Type: uint8_t   Size: 1   
   * Description: Subclass code (assigned by the USB-IF)   
   * if bDeviceClass = 0 then bDeviceSubClass = 0
   */
  .bDeviceSubClass = 0,

  /* Type: uint8_t   Size: 1   
   * Description: Protocol code (assigned by the USB-IF)   
   * 0 = the device does not use class specific protocols on a device 
   * basis. However, it may use class specific protocols on an interface 
   * basis
   */
  .bDeviceProtocol = 0,

  /* Type: uint8_t   Size: 1   
   * Description: Maximum packet size for Endpoint zero 
   * (only 8, 16, 32, or 64 are valid)
   */
  .bMaxPacketSize0 = 64,

  /* Type: uint16_t   Size: 2   
   * Description: Vendor ID (assigned by the USB-IF)
   */
  .idVendor = 0x0666,

  /* Type: uint16_t   Size: 2   
   * Description: Product ID (assigned by the manufacturer)
   */
  .idProduct = 0x0815,

  /* Type: uint16_t   Size: 2   
   * Description: Device release number in binary-coded decimal
   */
  .bcdDevice = 0x0101,

  /* Type: uint8_t   Size: 1   
   * Description: Index of string descriptor describing manufacturer
   */
  .iManufacturer = 1,

  /* Type: uint8_t   Size: 1   
   * Description: Index of string descriptor describing product
   */
  .iProduct = 2,

  /* Type: uint8_t   Size: 1   
   * Description: Index of string descriptor describing the device's 
   * serial number
   */
  .iSerialNumber = 3,

  /* Type: uint8_t   Size: 1   
   * Description: Number of possible configurations
   */
  .bNumConfigurations = 1,  
};

void usb_setup(usbd_device *dev, uint16_t wValue)
{

  (void)wValue;

  /* Setup USB Receive interrupt. */
  usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, NULL);

  usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    

}

//{ poll USB on interruptg */
void otg_fs_isr(void) {
  usbd_poll(usbd_dev);
}


/* Midi specific endpoint descriptors. */
static const struct usb_midi_endpoint_descriptor midi_usb_endp[] = {
  {
                
    /* Table B-12: MIDI Adapter Class-specific Bulk OUT Endpoint
     */
    .head = {
      .bLength = sizeof(struct usb_midi_endpoint_descriptor),
      .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
      .bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
      .bNumEmbMIDIJack = 1,
    },
    .jack[0] = {
      .baAssocJackID = 0x01,
    },
  }, {
    /* Table B-14: MIDI Adapter Class-specific Bulk IN Endpoint
     * Descriptor 
     */
    .head = {
      .bLength = sizeof(struct usb_midi_endpoint_descriptor),
      .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
      .bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
      .bNumEmbMIDIJack = 1,
    },
    .jack[0] = {
      .baAssocJackID = 0x03,
    },
  } 
};


/* Standard endpoint descriptors */
static const struct usb_endpoint_descriptor usb_endp[] = {
  {
    /* Look above */
    .bLength = USB_DT_ENDPOINT_SIZE,

    /* Look above */
    .bDescriptorType = USB_DT_ENDPOINT,

    /* Look above */
    .bEndpointAddress = 0x81,

    /* Look above */
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,

    /* Look above */
    .wMaxPacketSize = 0x40,

    /* Look above */
    .bInterval = 0x00, 			 

    /* Look above */
    .extra = &midi_usb_endp[1],

    /* Look above */
    .extralen = sizeof(midi_usb_endp[1]), 
  } 
};


/* Table B-4: MIDI Adapter Class-specific AC Interface Descriptor */
static const struct {
    struct usb_audio_header_descriptor_head header_head;
    struct usb_audio_header_descriptor_body header_body;
} __attribute__((packed)) audio_control_functional_descriptors = {
    .header_head = {
	.bLength = sizeof(struct usb_audio_header_descriptor_head) +
	1 * sizeof(struct usb_audio_header_descriptor_body),
	.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
	.bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
	.bcdADC = 0x0100,
	.wTotalLength =
	sizeof(struct usb_audio_header_descriptor_head) +
	1 * sizeof(struct usb_audio_header_descriptor_body),
	.binCollection = 1,
    },
    .header_body = {
	.baInterfaceNr = 0x01,
    },
};


/* MIDI Adapter Standard AC Interface Descriptor */
static const struct usb_interface_descriptor audio_control_iface[] = {
    {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.extra = &audio_control_functional_descriptors,
	.extralen = sizeof(audio_control_functional_descriptors)
    }
};


/* Class-specific MIDI streaming interface descriptor */
static const struct {
    struct usb_midi_header_descriptor header;
    struct usb_midi_in_jack_descriptor in_embedded;
} __attribute__((packed)) midi_streaming_functional_descriptors = {
    /* Table B-6: Midi Adapter Class-specific MS Interface Descriptor */
    .header = {
	.bLength = sizeof(struct usb_midi_header_descriptor),
	.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
	.bDescriptorSubtype = USB_MIDI_SUBTYPE_MS_HEADER,
	.bcdMSC = 0x0100,
	.wTotalLength = sizeof(midi_streaming_functional_descriptors),
    },
    /* Table B-7: MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
    .in_embedded = {
	.bLength = sizeof(struct usb_midi_in_jack_descriptor),
	.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
	.bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
	.bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
	.bJackID = 0x01,
	.iJack = 0x00,
    },
};


/* Table B-5: MIDI Adapter Standard MS Interface Descriptor */
static const struct usb_interface_descriptor midi_streaming_iface[] = {
    {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_MIDISTREAMING,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = usb_endp,

	.extra = &midi_streaming_functional_descriptors,
	.extralen = sizeof(midi_streaming_functional_descriptors)
    }
};

/* Struct with the Interface Descriptors */
static const struct usb_interface ifaces[] = {
    {
	/* Audio Control Interface Descriptor*/
	.num_altsetting = 1,
	.altsetting = audio_control_iface,
    },
    {
	/* MIDI Streaming Interface Descriptor*/
	.num_altsetting = 1,
	.altsetting = midi_streaming_iface,
    }
};


/* Configuration Descriptor */
static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,

    /* can be anything, it is updated automatically
     * when the usb code prepares the descriptor 
     */
    .wTotalLength = 0,

    /* control and data */
    .bNumInterfaces = 2, 
    .bConfigurationValue = 1,
    .iConfiguration = 0,

    /* bus powered (power settings)*/
    .bmAttributes = 0x80, 
    .bMaxPower = 0x64,

    .interface = ifaces,
};


/* USB Strings (Look at USB-Device-Descriptor) */
static const char * usb_strings[] = {

    /* Manufacturer */
    "Rafael Bormann Chuede",

    /* Product */
    "MIDI SENDER STM32",

    /* SerialNumber */
    "AHSM00003\0"			
};

void usb_send_cc(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel){
  /* Prepare MIDI packet for Note On message */
  char buf[4] = {
    0x08,                   /* Midi USB Byte */
    0xB0 + channel,                   /* cc byte on + channel */
    note,                   /* Note number */
    velocity                    /* Velocity (max) */
  };

  while (usbd_ep_write_packet(dev, 0x81, buf, sizeof(buf)) == 0);
}


void usb_send_noteOn(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel){
  /* Prepare MIDI packet for Note On message */
  char buf[4] = {
    0x08,                   /* Midi USB Byte */
    0x90 + channel,                   /* Note ON byte on channel */
    note,                   /* Note number (middle C) */
    velocity                    /* Velocity (max) */
  };

  while (usbd_ep_write_packet(dev, 0x81, buf, sizeof(buf)) == 0);
}
  
void usb_send_noteOff(usbd_device *dev, uint8_t note, uint8_t velocity, uint8_t channel){
  /* Prepare MIDI packet for Note On message */
  char buf[4] = {
    0x08,                   /* Command (Note On) */
    0x80 + channel,                   /* MIDI channel (Note On with channel 0) */
    note,                   /* Note number (middle C) */
    velocity                    /* Velocity (max) */
  };

  while (usbd_ep_write_packet(dev, 0x81, buf, sizeof(buf)) == 0);
}

void usbMidiInit(void)
{

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_OTGFS);
  
  OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS; // very important! if using blackpill
  
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
  gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);


  nvic_enable_irq(NVIC_OTG_FS_IRQ);

  usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descr,
		       &config, usb_strings, 3, usbd_control_buffer,
		       sizeof(usbd_control_buffer));
  usbd_register_set_config_callback(usbd_dev, usb_setup);

  /* Wait for USB to register on the PC */
  for (int i = 0; i < 0x800000; i++) { __asm__("nop"); }

  /* Wait for USB Vbus. */
//  while (gpio_get(GPIOA, GPIO8) == 0) { __asm__("nop"); }

}
