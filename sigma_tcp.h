/**
 * Copyright 2012(c) Analog Devices, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *
 **/

#ifndef __SIGMA_TCP_H__
#define __SIGMA_TCP_H__

/*
 * 20170501 aventuri:
 * put ths offset for ADAU1701 as per mismatch between sigmStudio 3.14.1
 * packet header and the one expected from the sigma_tcp "daemon":
 * following report: https://ez.analog.com/thread/92834
 * see my page: http://localhost/w/index.php/Sigma_Studio_and_Adau1701
 */
#define OFFSET 2

#include <stdint.h>

struct backend_ops {
	int (*open)(int argc, char *argv[]);
	int (*read)(unsigned int addr, unsigned int len, uint8_t *data);
	int (*write)(unsigned int addr, unsigned int len, const uint8_t *data);
};

extern const struct backend_ops i2c_backend_ops;
extern const struct backend_ops regmap_backend_ops;

#endif
