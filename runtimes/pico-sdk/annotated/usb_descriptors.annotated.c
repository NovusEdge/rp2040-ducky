// WHAT THIS FILE IS
// When you plug in any USB device, the host asks it a set of questions before it
// works. "What are you? Who made you? What can you do?" The device answers with
// descriptors: small tables of bytes in a fixed format. This step is called
// enumeration. This file holds the descriptors for the keyboard and the small
// callback functions that hand each table to TinyUSB when the host asks.
//
// You do not call these functions. TinyUSB calls them for you during
// enumeration. That is why each one ends in _cb, for "callback".

#include <string.h>

#include "tusb.h"

// The vendor ID and product ID name the device. A real vendor buys a VID from
// the USB group. 0xCAFE is a fake VID for hobby use. The host uses this pair to
// pick a driver. Any pair works here because the host has a built-in keyboard
// driver.
#define USB_VID 0xCAFE
#define USB_PID 0x4004
// bcdUSB is the USB spec version the device claims, in packed decimal. 0x0200
// means USB 2.0.
#define USB_BCD 0x0200

// THE DEVICE DESCRIPTOR
// This is the first table the host reads. It answers "what are you, at the top
// level?". The field names come straight from the USB spec, so they look
// cryptic. The comments below say what each one means.
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),   // size of this table in bytes
    .bDescriptorType = TUSB_DESC_DEVICE,     // "I am a device descriptor"
    .bcdUSB = USB_BCD,                        // USB 2.0
    // Class 0 here means "the class is not set at the device level; look at the
    // interface instead". The keyboard class is declared later, per interface.
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,  // 64, from tusb_config.h
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,       // device release number, your own versioning
    // These three are indexes, not text. Index 1, 2, 3 point into the string
    // table near the bottom of this file. The host asks for the strings later.
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,  // one configuration, defined below
};

// TinyUSB calls this to get the device descriptor. It just returns the address
// of the table above, cast to a byte pointer, because the host wants raw bytes.
uint8_t const* tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

// THE HID REPORT DESCRIPTOR
// This table describes the SHAPE of a keyboard report: how many bytes, what each
// byte means. The macro TUD_HID_REPORT_DESC_KEYBOARD() expands to the standard
// boot-keyboard layout, so you never hand-write it. This is how the host learns
// that byte 0 is modifiers and bytes 2..7 are key slots.
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(),
};

// TinyUSB calls this to fetch the report descriptor above. There is one HID
// interface, so the instance argument is ignored.
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return desc_hid_report;
}

// An interface is one function of the device. This device has one interface: the
// keyboard. ITF_NUM_HID is 0 (its index). ITF_NUM_TOTAL is 1 (the count). The
// enum trick makes the count equal the number of names before TOTAL.
enum { ITF_NUM_HID, ITF_NUM_TOTAL };

// The configuration descriptor lists every interface. Its total length must
// cover the config header plus each interface block. The macros give the two
// lengths, so you add them.
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
// The endpoint that carries key reports to the host. 0x81 means endpoint 1, and
// the high bit (0x80) means IN, which is "device to host". The keyboard sends
// reports IN to the host.
#define EPNUM_HID 0x81

// THE CONFIGURATION DESCRIPTOR
// This one table, sent as a block, tells the host the whole plan: one config,
// one interface, one endpoint, and the timing.
uint8_t const desc_configuration[] = {
    // config number 1, ITF_NUM_TOTAL interfaces, string index 0 (none),
    // total length, attributes, and 100 mA of bus power. REMOTE_WAKEUP lets the
    // device ask the host to wake from sleep.
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // the HID interface: its number, no string, boot-keyboard protocol, the
    // size of the report descriptor above, the endpoint number, the endpoint
    // buffer size, and 10 ms poll interval. The host polls this endpoint every
    // 10 ms and asks for a fresh report.
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(desc_hid_report), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
};

// TinyUSB calls this to fetch the configuration block above. One config, so the
// index is ignored.
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

// THE STRING TABLE
// The device descriptor pointed at string indexes 1, 2, 3. Here are the actual
// strings. Index 0 is special: it is not text but the list of languages the
// device speaks.
//
// Index 0 is the supported-language list. The other indexes return UTF-16
// strings on request.
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // English (0x0409), sent low byte first
    "Generic",                   // Manufacturer (index 1)
    "USB Keyboard",              // Product      (index 2)
    "000000000001",              // Serial       (index 3)
};

// A scratch buffer to build one string reply. USB string descriptors use UTF-16,
// so each character takes two bytes. 32 entries hold 31 characters plus a header.
static uint16_t _desc_str[32];

// TinyUSB calls this when the host asks for one string by index. The function
// builds the reply in _desc_str and returns it.
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    // The language list is already two raw bytes. Copy them in after the header
    // slot. chr_count is 1 because one 16-bit value follows the header.
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    // Guard against an index past the end of the table. Return NULL and TinyUSB
    // reports the string as missing.
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
      return NULL;
    const char* str = string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;   // the buffer holds 31 characters max
    // Widen each ASCII byte to a 16-bit UTF-16 value. This works for plain
    // ASCII because the high byte is zero.
    for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
  }

  // Slot 0 is the header: the high byte is the descriptor type, the low byte is
  // the total length in bytes. Length = 2 per character, plus 2 for the header.
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
