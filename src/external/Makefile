.PHONY: all clean

all:
ifeq ("$(wildcard ./libusb-1.0.9/libusb/.libs/libusb-1.0.a)","")
	cd libusb-1.0.9 && ./configure CFLAGS="-fPIC"
	make -C libusb-1.0.9 CFLAGS="-fPIC"
endif

ifeq ("$(wildcard ./odroid-smartpower-linux/libsmartgauge.a)","")
	make -C odroid-smartpower-linux
	make -C odroid-smartpower-linux lib
endif

clean:
	make -C odroid-smartpower-linux clean
