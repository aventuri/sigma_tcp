#BASEDIR=/home/buildroot/src/buildroot/buildroot-2015.02/output/host
# BASEDIR=/home/buildroot/buildroot_outputs/host

# /home/buildroot/src/buildroot/buildroot-2015.02/output/host/usr/bin/arm-linux-gnueabihf-gcc
#
# distrobox
#BASEDIR=/home/buildroot/src/buildroot/buildroot-2015.02/output/host

# andrea desktop cherrytrai
BASEDIR=/home/andrea/src/buildroot-2017.02/output/host

CC=$(BASEDIR)/usr/bin/arm-linux-gnueabihf-gcc

CFLAGS= -std=gnu99 -pedantic-errors -ggdb -Wall
#CFLAGS= -std=c99 -pedantic-errors -Wall \
	-I$(BASEDIR)/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include \
	-I$(BASEDIR)/opt/ext-toolchain/arm-linux-gnueabihf/include/c++/4.9.2/ \
	-I$(BASEDIR)/opt/ext-toolchain/arm-linux-gnueabihf/include/c++/4.9.2/arm-linux-gnueabihf/


DESTDIR=/usr/local

sigma_tcp: i2c.c regmap.c

install:
	install -d $(DESTDIR)/bin
	install sigma_tcp $(DESTDIR)/bin

flash:
	scp sigma_tcp root@192.168.0.106:

clean:
	rm -rf sigma_tcp *.o
