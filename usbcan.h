#ifndef _USBCAN_H_
#define _USBCAN_H_
#include "canframe.h"


struct ucan {
	libusb_device_handle *udh;
	pthread_mutex_t mutex; 
};

struct ucan* usbcan_init();
int usbcan_exit(struct ucan *can);
int usbcan_send(struct ucan *can, struct can_frame *cf);
int usbcan_recv(struct ucan *can, struct can_frame *cf);

#endif
