/*
 * This file is part of OpenCorsairLink.
 * Copyright (C) 2017  Sean Nelson <audiohacked@gmail.com>

 * OpenCorsairLink is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.

 * OpenCorsairLink is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with OpenCorsairLink.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libusb.h>
#include "options.h"
#include "device.h"
#include "print.h"
#include "lowlevel/asetek4.h"
#include "lowlevel/hid.h"
#include "lowlevel/rmi.h"
#include "protocol/hid/core.h"
#include "protocol/rmi/core.h"

extern struct corsair_device_info corsairlink_devices[2];

int corsairlink_find_device(struct corsair_device_info *dev)
{
	int r;
	r = libusb_init(&dev->context);
	if (r < 0) {
		msg_info("Init Error %d\n", r);
		return 1;
	}
	libusb_set_debug(dev->context, 3);

	dev->handle = libusb_open_device_with_vid_pid(dev->context, dev->vendor_id, dev->product_id);
	if(dev->handle == NULL) {
		return -1;
	}

	r = libusb_detach_kernel_driver(dev->handle, 0);
	r = libusb_claim_interface(dev->handle, 1);

	return 0;
}

void corsairlink_close(struct corsair_device_info *dev)
{
	int r;
	r = libusb_release_interface(dev->handle, 0);
	libusb_close(dev->handle);
	libusb_exit(dev->context);
}

int main(int argc, char *argv[])
{
	struct corsair_device_info *dev;
	int r; // result from libusb functions
	int i; // for loops

	struct option_flags flags;
	struct color led_color;
	struct color warning_led;
	struct fan_table fan1;
	struct fan_table pump;
	int8_t warning_led_temp;
	uint8_t pump_mode;

	char name[20];
	name[sizeof(name) - 1] = 0;
	uint32_t time = 0;
	uint16_t supply_volts, supply_watts,
		output_volts, output_amps, output_watts; 
	
	options_parse(argc, argv, &flags,
		&led_color, &warning_led, &warning_led_temp,
		&fan1, &pump, &pump_mode);
	
	for (i=0; i<7; i++) {
		dev = &corsairlink_devices[i];
		r = corsairlink_find_device(dev);
		if (r >= 0) {
			msg_info("CorsairLink Device Found: %s!\n", dev->name);
			break;
		}
	}

	r = dev->driver->init(dev->handle, dev->write_endpoint);

	/* fetch device name, vendor name, product name */
	r = dev->driver->name(dev, name);
	msg_info("Name: %s\n", name);
	r = dev->driver->vendor(dev, name);
	msg_info("Vendor: %s\n", name);
	r = dev->driver->product(dev, name);
	msg_info("Product: %s\n", name);

	r = dev->driver->led(dev, &led_color, &warning_led, warning_led_temp, (warning_led_temp > -1));

	r = dev->driver->deinit(dev->handle, dev->write_endpoint);

exit:
	corsairlink_close(dev);
	return 0;
}
