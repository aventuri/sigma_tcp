/**
 * Copyright (C) 2012 Analog Devices, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

#include "sigma_tcp.h"

#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>


static void addr_to_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
	switch(sa->sa_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
				s, maxlen);
		break;
	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
				s, maxlen);
	break;
	default:
		strncpy(s, "Unkown", maxlen);
	}
}

static int show_addrs(int sck)
{
	char buf[256];
	char ip[INET6_ADDRSTRLEN];
	struct ifconf ifc;
	struct ifreq *ifr;
	unsigned int i, n;
	int ret;

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	ret = ioctl(sck, SIOCGIFCONF, &ifc);
	if (ret < 0) {
		perror("ioctl(SIOCGIFCONF)");
		return 1;
	}

	ifr = ifc.ifc_req;
	n = ifc.ifc_len / sizeof(struct ifreq);

	printf("IP addresses:\n");

	for (i = 0; i < n; i++) {
		struct sockaddr *addr = &ifr[i].ifr_addr;

		if (strcmp(ifr[i].ifr_name, "lo") == 0)
			continue;

		addr_to_str(addr, ip, INET6_ADDRSTRLEN);
		printf("%s: %s\n", (char *) &ifr[i].ifr_name, ip);
	}

	return 0;
}

#define CMD_WRITE 0x09
#define CMD_READ 0x0a
#define CMD_RESP 0x0b

static uint8_t debug_data[256];

static int debug_read(unsigned int addr, unsigned int len, uint8_t *data)
{
	if (addr < 0x4000 || addr + len > 0x4100) {
		memset(data, 0x00, len);
		return 0;
	}

	printf("read: %.2x %d\n", addr, len);

	addr -= 0x4000;
	memcpy(data, debug_data + addr, len);

	return 0;
}

static int debug_write(unsigned int addr, unsigned int len, const uint8_t *data)
{
	if (addr < 0x4000 || addr + len > 0x4100)
		return 0;

	printf("write: %.2x %d\n", addr, len);

	addr -= 0x4000;
	memcpy(debug_data + addr, data, len);

	return 0;
}

static int debug_open(int argc, char *argv[])
{
        //int ret;
        //char *endp;

	fprintf(stderr, "debug: no param requested; passed %d\n", argc);
	return 0;
}

static struct backend_ops debug_backend_ops
#if 0
 = (struct backend_ops) {
	.open = debug_open,
	.read = debug_read,
	.write = debug_write,
 }
#endif
;

static struct backend_ops *backend_ops = &debug_backend_ops;

static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

enum STATE_ENUM { FSM_IDLE, \
	FSM_CMD_READ, FSM_CMD_WRITE, \
	FSM_NET_WAITING_DATA, \
	FSM_I2C_WRITE, FSM_I2C_READ, \
	FSM_NET_RESP, \
	FSM_STOP, FSM_ERR };

#define MAX_BUF_SIZE 512

static void handle_connection(int fd)
{
	enum STATE_ENUM state = FSM_IDLE;
	int count, ret, start;

	count = 0;

	uint8_t buf[MAX_BUF_SIZE];
	uint8_t *p = buf;

	struct adauReqHeader_s *req;
	struct adauWriteHeader_s *regWrite;

	start=0; // index of first transaction byte
	while (state != FSM_STOP) {
		memmove(buf, p, count);
		p = buf + count;

		if (state == FSM_IDLE || state == FSM_NET_WAITING_DATA) {
			ret = read(fd, p, MAX_BUF_SIZE - count);
			if (ret <= 0)
				break;

			p = buf;
			count += ret;
		}

		switch (state) {
			case FSM_IDLE:
				if (count > 0) {
					switch(*p) {
						case CMD_READ:
							printf("start a read transaction\n");
							state = FSM_CMD_READ;
							break;
						case CMD_WRITE:
							printf("start a write transaction\n");
							state = FSM_CMD_WRITE;
							break;
						default:
							printf("command %x not managed for packet\n", buf[start]);
							state = FSM_ERR;
					}
				}
				break;
			case FSM_CMD_READ:
				if (count >= sizeof(struct adauReqHeader_s)) {
					req = (struct adauReqHeader_s*) p;
					// got enough info, send a read req
					state = FSM_I2C_READ;
					printf("read CMD: got the NET header, need to read %d byte on chip %x at param addr %d\n",
							req->dataLen, req->chipAddr, req->paramAddr );
				}
				break;
			case FSM_CMD_WRITE:
				if (count >= sizeof(struct adauWriteHeader_s)) {
					regWrite = (struct adauWriteHeader_s *) p;
					// got enough info, send a read req
					state = FSM_NET_WAITING_DATA;
					printf("write CMD: got the NET header, need to write %d bytes on chip %x at param addr %d\n",
							regWrite->dataLen, regWrite->chipAddr, regWrite->paramAddr );
				} else {
					state = FSM_NET_WAITING_DATA;
					break;
				}
			case FSM_NET_WAITING_DATA:
				if (count >= sizeof(struct adauWriteHeader_s) + regWrite->dataLen) { // maybe just regWrite->totalLen?
					state = FSM_I2C_WRITE;
				}
				break;
			case FSM_I2C_WRITE:
				printf("write to I2C addr %x, param %d, data length %d", regWrite->chipAddr, regWrite->paramAddr, regWrite->dataLen);
				backend_ops->write(regWrite->paramAddr, regWrite->dataLen, p + sizeof(struct adauWriteHeader_s));
				p += regWrite->totalLen;
				count -= regWrite->totalLen;
				state = FSM_IDLE;
				break;
			case FSM_I2C_READ:
				printf("read I2C addr %x, param %d, data length %d", req->chipAddr, req->paramAddr, req->dataLen);
				if (req->dataLen < (MAX_BUF_SIZE- sizeof(struct adauReqHeader_s))) {
					uint8_t bufResp[MAX_BUF_SIZE];
					struct adauRespHeader_s *resp = (struct adauRespHeader_s *) bufResp;
					int respLen = backend_ops->read(req->paramAddr, req->dataLen, bufResp + sizeof(struct adauRespHeader_s));
					if (respLen>0) {
						resp->controlBit = CMD_RESP;
						resp->totalLen = sizeof(struct adauRespHeader_s) + respLen;
						resp->chipAddr = req->chipAddr;
						resp->dataLen = respLen;
						resp->paramAddr = req->paramAddr;
						resp->success = (respLen == req->dataLen)?0:1 ;
						resp->reserved1 = 0;
						write(fd, bufResp, resp->totalLen);
					}
				} else {
					printf("cant manage too large reply! %d bytes", req->dataLen);

				}
				p += req->totalLen;
				count -= req->totalLen;
				state = FSM_IDLE;
				break;
			case FSM_ERR:
				printf("bailing out..  no recover procedure now\n");
				state = FSM_STOP;
				break;
			default:
				printf("unamanged state %d..  no recover procedure now\n", state);
				state = FSM_STOP;
				break;
		}
	}
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct addrinfo *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    //struct sigaction sa;
    int reuse = 1;
    char s[INET6_ADDRSTRLEN];
    int ret;

    debug_backend_ops.open = debug_open;
    debug_backend_ops.read = debug_read;
    debug_backend_ops.write = debug_write;
// already fn bound on structt *_ops
#if 0
    i2c_backend_ops.open = i2c_open;
    i2c_backend_ops.read = i2c_read;
    i2c_backend_ops.write = i2c_write;
    regmap_backend_ops.open = regmap_open;
    regmap_backend_ops.read = regmap_read;
    regmap_backend_ops.write = regmap_write;
#endif

	if (argc >= 2) {
		if (strcmp(argv[1], "debug") == 0)
			backend_ops = &debug_backend_ops;
		else if (strcmp(argv[1], "i2c") == 0)
			backend_ops = &i2c_backend_ops;
		else if (strcmp(argv[1], "regmap") == 0)
			backend_ops = &regmap_backend_ops;
		else {
			printf("Usage: %s <backend> <backend arg0> ...\n"
				   "Available backends: debug, i2c, regmap\n", argv[0]);
			exit(0);
		}

		printf("Using %s backend\n", argv[1]);
	}

	if (backend_ops->open) {
		ret = backend_ops->open(argc, argv);
		if (ret)
			exit(1);
	}

#if 0
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

	ret = getaddrinfo(NULL, "8086", &hints, &servinfo);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }
#endif


// forcing params as my test 
	servinfo = malloc (sizeof( struct addrinfo ));
	servinfo->ai_family = AF_INET;
	servinfo->ai_socktype = SOCK_STREAM;
	servinfo->ai_protocol = 0;
	if (servinfo->ai_next) {
		perror ("flushing more then one returned structs");
		servinfo->ai_next = NULL;
	}

// end of manual cfg

    struct sockaddr_in serv_addr;
    int portno = 8086;

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
	
        //if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 

	bzero((char *) &serv_addr, sizeof(*servinfo->ai_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
        if (bind(sockfd,  (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "Failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    if (listen(sockfd, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Waiting for connections on port %d...\n", portno);
	show_addrs(sockfd);

    fd_set readfds;

    while (true) {
        sin_size = sizeof their_addr;
	//clear the socket set
	FD_ZERO(&readfds);

	//add master socket to set
	FD_SET(sockfd, &readfds);
	int max_sd = sockfd;

	int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	if ((activity < 0) && (errno!=EINTR)) {
		printf("select error");
	}

        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);

        printf("New connection from %s\n", s);
		handle_connection(new_fd);
        printf("Connection closed\n");
    }

    return 0;
}
