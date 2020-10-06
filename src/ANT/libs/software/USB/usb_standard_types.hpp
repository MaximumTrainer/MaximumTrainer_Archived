/*
 * Public libusb header file
 * Copyright (C) 2007-2008 Daniel Drake <dsd@gentoo.org>
 * Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __USB_STANDARD_TYPES_H__
#define __USB_STANDARD_TYPES_H__

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif



/* standard USB stuff */


/** \ingroup desc
 * Device and/or Interface Class codes */
enum libusb_class_code {
   /** In the context of a \ref libusb_device_descriptor "device descriptor",
    * this bDeviceClass value indicates that each interface specifies its
    * own class information and all interfaces operate independently.
    */
   LIBUSB_CLASS_PER_INTERFACE = 0,

   /** Audio class */
   LIBUSB_CLASS_AUDIO = 1,

   /** Communications class */
   LIBUSB_CLASS_COMM = 2,

   /** Human Interface Device class */
   LIBUSB_CLASS_HID = 3,

   /** Printer dclass */
   LIBUSB_CLASS_PRINTER = 7,

   /** Picture transfer protocol class */
   LIBUSB_CLASS_PTP = 6,

   /** Mass storage class */
   LIBUSB_CLASS_MASS_STORAGE = 8,

   /** Hub class */
   LIBUSB_CLASS_HUB = 9,

   /** Data class */
   LIBUSB_CLASS_DATA = 10,

   /** Class is vendor-specific */
   LIBUSB_CLASS_VENDOR_SPEC = 0xff,
};

/** \ingroup desc
 * Descriptor types as defined by the USB specification. */
enum libusb_descriptor_type {
   /** Device descriptor. See libusb_device_descriptor. */
   LIBUSB_DT_DEVICE = 0x01,

   /** Configuration descriptor. See libusb_config_descriptor. */
   LIBUSB_DT_CONFIG = 0x02,

   /** String descriptor */
   LIBUSB_DT_STRING = 0x03,

   /** Interface descriptor. See libusb_interface_descriptor. */
   LIBUSB_DT_INTERFACE = 0x04,

   /** Endpoint descriptor. See libusb_endpoint_descriptor. */
   LIBUSB_DT_ENDPOINT = 0x05,

   /** HID descriptor */
   LIBUSB_DT_HID = 0x21,

   /** HID report descriptor */
   LIBUSB_DT_REPORT = 0x22,

   /** Physical descriptor */
   LIBUSB_DT_PHYSICAL = 0x23,

   /** Hub descriptor */
   LIBUSB_DT_HUB = 0x29,
};

/* Descriptor sizes per descriptor type */
#define LIBUSB_DT_DEVICE_SIZE       18
#define LIBUSB_DT_CONFIG_SIZE       9
#define LIBUSB_DT_INTERFACE_SIZE    9
#define LIBUSB_DT_ENDPOINT_SIZE     7
#define LIBUSB_DT_ENDPOINT_AUDIO_SIZE  9  /* Audio extension */
#define LIBUSB_DT_HUB_NONVAR_SIZE      7

#define LIBUSB_ENDPOINT_ADDRESS_MASK   0x0f    /* in bEndpointAddress */
#define LIBUSB_ENDPOINT_DIR_MASK    0x80

/** \ingroup desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum libusb_endpoint_direction {
   /** In: device-to-host */
   LIBUSB_ENDPOINT_IN = 0x80,

   /** Out: host-to-device */
   LIBUSB_ENDPOINT_OUT = 0x00,
};

#define LIBUSB_TRANSFER_TYPE_MASK         0x03    /* in bmAttributes */

/** \ingroup desc
 * Endpoint transfer type. Values for bits 0:1 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "endpoint attributes" field.
 */
enum libusb_transfer_type {
   /** Control endpoint */
   LIBUSB_TRANSFER_TYPE_CONTROL = 0,

   /** Isochronous endpoint */
   LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1,

   /** Bulk endpoint */
   LIBUSB_TRANSFER_TYPE_BULK = 2,

   /** Interrupt endpoint */
   LIBUSB_TRANSFER_TYPE_INTERRUPT = 3,
};

/** \ingroup misc
 * Standard requests, as defined in table 9-3 of the USB2 specifications */
enum libusb_standard_request {
   /** Request status of the specific recipient */
   LIBUSB_REQUEST_GET_STATUS = 0x00,

   /** Clear or disable a specific feature */
   LIBUSB_REQUEST_CLEAR_FEATURE = 0x01,

   /* 0x02 is reserved */

   /** Set or enable a specific feature */
   LIBUSB_REQUEST_SET_FEATURE = 0x03,

   /* 0x04 is reserved */

   /** Set device address for all future accesses */
   LIBUSB_REQUEST_SET_ADDRESS = 0x05,

   /** Get the specified descriptor */
   LIBUSB_REQUEST_GET_DESCRIPTOR = 0x06,

   /** Used to update existing descriptors or add new descriptors */
   LIBUSB_REQUEST_SET_DESCRIPTOR = 0x07,

   /** Get the current device configuration value */
   LIBUSB_REQUEST_GET_CONFIGURATION = 0x08,

   /** Set device configuration */
   LIBUSB_REQUEST_SET_CONFIGURATION = 0x09,

   /** Return the selected alternate setting for the specified interface */
   LIBUSB_REQUEST_GET_INTERFACE = 0x0A,

   /** Select an alternate interface for the specified interface */
   LIBUSB_REQUEST_SET_INTERFACE = 0x0B,

   /** Set then report an endpoint's synchronization frame */
   LIBUSB_REQUEST_SYNCH_FRAME = 0x0C,
};

/** \ingroup misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. */
enum libusb_request_type {
   /** Standard */
   LIBUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

   /** Class */
   LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

   /** Vendor */
   LIBUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

   /** Reserved */
   LIBUSB_REQUEST_TYPE_RESERVED = (0x03 << 5),
};

/** \ingroup misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved. */
enum libusb_request_recipient {
   /** Device */
   LIBUSB_RECIPIENT_DEVICE = 0x00,

   /** Interface */
   LIBUSB_RECIPIENT_INTERFACE = 0x01,

   /** Endpoint */
   LIBUSB_RECIPIENT_ENDPOINT = 0x02,

   /** Other */
   LIBUSB_RECIPIENT_OTHER = 0x03,
};

#define LIBUSB_ISO_SYNC_TYPE_MASK      0x0C

/** \ingroup desc
 * Synchronization type for isochronous endpoints. Values for bits 2:3 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_sync_type {
   /** No synchronization */
   LIBUSB_ISO_SYNC_TYPE_NONE = 0,

   /** Asynchronous */
   LIBUSB_ISO_SYNC_TYPE_ASYNC = 1,

   /** Adaptive */
   LIBUSB_ISO_SYNC_TYPE_ADAPTIVE = 2,

   /** Synchronous */
   LIBUSB_ISO_SYNC_TYPE_SYNC = 3,
};

#define LIBUSB_ISO_USAGE_TYPE_MASK 0x30

/** \ingroup desc
 * Usage type for isochronous endpoints. Values for bits 4:5 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_usage_type {
   /** Data endpoint */
   LIBUSB_ISO_USAGE_TYPE_DATA = 0,

   /** Feedback endpoint */
   LIBUSB_ISO_USAGE_TYPE_FEEDBACK = 1,

   /** Implicit feedback Data endpoint */
   LIBUSB_ISO_USAGE_TYPE_IMPLICIT = 2,
};

/** \ingroup desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_device_descriptor {
   /** Size of this descriptor (in bytes) */
   uint8_t  bLength;

   /** Descriptor type. Will have value
    * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE in this
    * context. */
   uint8_t  bDescriptorType;

   /** USB specification release number in binary-coded decimal. A value of
    * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
   uint16_t bcdUSB;

   /** USB-IF class code for the device. See \ref libusb_class_code. */
   uint8_t  bDeviceClass;

   /** USB-IF subclass code for the device, qualified by the bDeviceClass
    * value */
   uint8_t  bDeviceSubClass;

   /** USB-IF protocol code for the device, qualified by the bDeviceClass and
    * bDeviceSubClass values */
   uint8_t  bDeviceProtocol;

   /** Maximum packet size for endpoint 0 */
   uint8_t  bMaxPacketSize0;

   /** USB-IF vendor ID */
   uint16_t idVendor;

   /** USB-IF product ID */
   uint16_t idProduct;

   /** Device release number in binary-coded decimal */
   uint16_t bcdDevice;

   /** Index of string descriptor describing manufacturer */
   uint8_t  iManufacturer;

   /** Index of string descriptor describing product */
   uint8_t  iProduct;

   /** Index of string descriptor containing device serial number */
   uint8_t  iSerialNumber;

   /** Number of possible configurations */
   uint8_t  bNumConfigurations;
};

/** \ingroup desc
 * A structure representing the standard USB endpoint descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_endpoint_descriptor {
   /** Size of this descriptor (in bytes) */
   uint8_t  bLength;

   /** Descriptor type. Will have value
    * \ref libusb_descriptor_type::LIBUSB_DT_ENDPOINT LIBUSB_DT_ENDPOINT in
    * this context. */
   uint8_t  bDescriptorType;

   /** The address of the endpoint described by this descriptor. Bits 0:3 are
    * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction,
    * see \ref libusb_endpoint_direction.
    */
   uint8_t  bEndpointAddress;

   /** Attributes which apply to the endpoint when it is configured using
    * the bConfigurationValue. Bits 0:1 determine the transfer type and
    * correspond to \ref libusb_transfer_type. Bits 2:3 are only used for
    * isochronous endpoints and correspond to \ref libusb_iso_sync_type.
    * Bits 4:5 are also only used for isochronous endpoints and correspond to
    * \ref libusb_iso_usage_type. Bits 6:7 are reserved.
    */
   uint8_t  bmAttributes;

   /** Maximum packet size this endpoint is capable of sending/receiving. */
   uint16_t wMaxPacketSize;

   /** Interval for polling endpoint for data transfers. */
   uint8_t  bInterval;

   /** For audio devices only: the rate at which synchronization feedback
    * is provided. */
   uint8_t  bRefresh;

   /** For audio devices only: the address if the synch endpoint */
   uint8_t  bSynchAddress;

   /** Extra descriptors. If libusb encounters unknown endpoint descriptors,
    * it will store them here, should you wish to parse them. */
   const unsigned char *extra;

   /** Length of the extra descriptors, in bytes. */
   int extra_length;
};

/** \ingroup desc
 * A structure representing the standard USB interface descriptor. This
 * descriptor is documented in section 9.6.5 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_interface_descriptor {
   /** Size of this descriptor (in bytes) */
   uint8_t  bLength;

   /** Descriptor type. Will have value
    * \ref libusb_descriptor_type::LIBUSB_DT_INTERFACE LIBUSB_DT_INTERFACE
    * in this context. */
   uint8_t  bDescriptorType;

   /** Number of this interface */
   uint8_t  bInterfaceNumber;

   /** Value used to select this alternate setting for this interface */
   uint8_t  bAlternateSetting;

   /** Number of endpoints used by this interface (excluding the control
    * endpoint). */
   uint8_t  bNumEndpoints;

   /** USB-IF class code for this interface. See \ref libusb_class_code. */
   uint8_t  bInterfaceClass;

   /** USB-IF subclass code for this interface, qualified by the
    * bInterfaceClass value */
   uint8_t  bInterfaceSubClass;

   /** USB-IF protocol code for this interface, qualified by the
    * bInterfaceClass and bInterfaceSubClass values */
   uint8_t  bInterfaceProtocol;

   /** Index of string descriptor describing this interface */
   uint8_t  iInterface;

   /** Array of endpoint descriptors. This length of this array is determined
    * by the bNumEndpoints field. */
   const struct libusb_endpoint_descriptor *endpoint;

   /** Extra descriptors. If libusb encounters unknown interface descriptors,
    * it will store them here, should you wish to parse them. */
   const unsigned char *extra;

   /** Length of the extra descriptors, in bytes. */
   int extra_length;
};


/** \ingroup desc
 * A structure representing the standard USB configuration descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 2.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_config_descriptor {
   /** Size of this descriptor (in bytes) */
   uint8_t  bLength;

   /** Descriptor type. Will have value
    * \ref libusb_descriptor_type::LIBUSB_DT_CONFIG LIBUSB_DT_CONFIG
    * in this context. */
   uint8_t  bDescriptorType;

   /** Total length of data returned for this configuration */
   uint16_t wTotalLength;

   /** Number of interfaces supported by this configuration */
   uint8_t  bNumInterfaces;

   /** Identifier value for this configuration */
   uint8_t  bConfigurationValue;

   /** Index of string descriptor describing this configuration */
   uint8_t  iConfiguration;

   /** Configuration characteristics */
   uint8_t  bmAttributes;

   /** Maximum power consumption of the USB device from this bus in this
    * configuration when the device is fully opreation. Expressed in units
    * of 2 mA. */
   uint8_t  MaxPower;

   /** Array of interfaces supported by this configuration. The length of
    * this array is determined by the bNumInterfaces field. */
   const struct libusb_interface *interface;

   /** Extra descriptors. If libusb encounters unknown configuration
    * descriptors, it will store them here, should you wish to parse them. */
   const unsigned char *extra;

   /** Length of the extra descriptors, in bytes. */
   int extra_length;
};

/** \ingroup asyncio
 * Setup packet for control transfers. */
struct libusb_control_setup {
   /** Request type. Bits 0:4 determine recipient, see
    * \ref libusb_request_recipient. Bits 5:6 determine type, see
    * \ref libusb_request_type. Bit 7 determines data transfer direction, see
    * \ref libusb_endpoint_direction.
    */
   uint8_t  bmRequestType;

   /** Request. If the type bits of bmRequestType are equal to
    * \ref libusb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
    * "LIBUSB_REQUEST_TYPE_STANDARD" then this field refers to
    * \ref libusb_standard_request. For other cases, use of this field is
    * application-specific. */
   uint8_t  bRequest;

   /** Value. Varies according to request */
   uint16_t wValue;

   /** Index. Varies according to request, typically used to pass an index
    * or offset */
   uint16_t wIndex;

   /** Number of bytes to transfer */
   uint16_t wLength;
};

#ifdef __cplusplus
}
#endif

#endif
