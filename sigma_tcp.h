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
 * 20170803 aventuri:
 * reworkd from info of: https://wiki.analog.com/resources/tools-software/sigmastudio/usingsigmastudio/tcpipchannels
 *
 * 20170501 aventuri, obsolete:
 * put ths offset for ADAU1701 as per mismatch between sigmStudio 3.14.1
 * packet header and the one expected from the sigma_tcp "daemon":
 * following report: https://ez.analog.com/thread/92834
 * see my page: http://localhost/w/index.php/Sigma_Studio_and_Adau1701
 */

#include <stdint.h>

struct adauWriteHeader_s {
	uint8_t controlBit;
	uint8_t safeload;
	uint8_t channelNum;
	uint8_t totalLen[4];
	uint8_t chipAddr;
	uint8_t dataLen[4];
	uint8_t paramAddr[2];
	// follow data to be received
};
struct adauReqHeader_s {
	uint8_t controlBit;
	uint8_t totalLen[4];
	uint8_t chipAddr;
	uint8_t dataLen[4];
	uint8_t paramAddr[2];
	//uint8_t reserved[2];
};
struct adauRespHeader_s {
	uint8_t controlBit;
	uint8_t totalLen[4];
	uint8_t chipAddr;
	uint8_t dataLen[4];
	uint8_t paramAddr[2];
	uint8_t success;
	uint8_t reserved[1];
	// follow data to be sent back
};


#include <stdint.h>

struct backend_ops {
	int (*open)(int argc, char *argv[]);
	int (*read)(unsigned int addr, unsigned int len, uint8_t *data);
	int (*write)(unsigned int addr, unsigned int len, const uint8_t *data);
};

extern struct backend_ops i2c_backend_ops;
extern struct backend_ops regmap_backend_ops;

#endif
