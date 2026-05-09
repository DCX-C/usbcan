#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "fcntl.h"
#include "unistd.h"
#include "pthread.h"

#include "./inc/libusb.h"
#include "usbcan.h"
#include "canframe.h"

#define EP_OUT 0x01
#define EP_IN  0x81


static struct ucan gcan = {
	.udh = NULL, 
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

static char start_measure[2][3] = {
    {0x53, 0x38, 0x0d},
    {0x4f, 0x0d, 0x0a},
};

static char stop_measure[2] = {
    0x43, 0x0d
};

struct ucan* usbcan_init()
{
    int r;
    ssize_t cnt;
    int done;
 
    r = libusb_init(NULL);
    if (r < 0)
        return NULL;

    pthread_mutex_init(&gcan.mutex, NULL);

    gcan.udh = libusb_open_device_with_vid_pid(NULL, (uint16_t)0x16d0, (uint16_t)0x117e);
    if (gcan.udh == NULL) {
        printf("libusb open fail\n");
    }

    libusb_set_auto_detach_kernel_driver(gcan.udh, 1);
    r = libusb_claim_interface(gcan.udh, 1);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(gcan.udh);
        printf("claim interface fail\n");
        return NULL;
    }
    
    uint8_t buf[16];
    //class 0x21 d2h
    memset(buf, 0, 16);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    //class 0x20 h2d
    memset(buf, 0, 16);
    buf[0] = 0x40;
    buf[1] = 0x42;
    buf[2] = 0x0f;
    buf[6] = 0x08;
    libusb_control_transfer(gcan.udh, 0x21, 0x20, 0x0000, 0x0000, buf, 7, 1000);
    
    memset(buf, 0, 16);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    //class 0x22 h2d
    libusb_control_transfer(gcan.udh, 0x21, 0x22, 0x0000, 0x0000, NULL, 0, 1000);
    
    //class 0x20 h2d
    memset(buf, 0, 16);
    buf[0] = 0;
    buf[1] = 0xc2;
    buf[2] = 0x01;
    buf[6] = 0x08;
    libusb_control_transfer(gcan.udh, 0x21, 0x20, 0x0000, 0x0000, buf, 7, 1000);
    
    memset(buf, 0, 16);
    libusb_control_transfer(gcan.udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    usleep(4000);
    libusb_bulk_transfer(gcan.udh, EP_OUT, start_measure[0], 3, &r, 1000);
    usleep(4000);
    libusb_bulk_transfer(gcan.udh, EP_OUT, start_measure[1], 3, &r, 1000);

    printf("start measure\n");
    usleep(5);
    return &gcan;
}

int usbcan_exit(struct ucan *can)
{
    int r = 0;
    usleep(5);
    
    libusb_release_interface(can->udh, 1);
    libusb_close(can->udh); 
    libusb_exit(NULL);
    
    printf("usb exit\n");
    return 0;
}

int usbcan_send(struct ucan *can, struct can_frame *cf)
{
    int done;
    char data[666];
    can_frame2str(cf, data);
    //printf("send len: %d\n", strlen(data));
    pthread_mutex_lock(&can->mutex); // 加锁
    printf("send: %s\n", data);
    printf("sending: %d\n", strlen(data));
    libusb_bulk_transfer(can->udh, EP_OUT, data, strlen(data), &done, 1000);
    printf("send done: %d\n", strlen(data));
    pthread_mutex_unlock(&can->mutex); // 加锁
    return done;
}

int usbcan_recv(struct ucan *can, struct can_frame *cf)
{
    int r;
    int done = 0;
    char buf[666];

    pthread_mutex_lock(&can->mutex); // 加锁
    //printf(".\n");
    r = libusb_bulk_transfer(can->udh, EP_IN, buf, 666, &done, 5);
    //printf("recv done\n");
    pthread_mutex_unlock(&can->mutex); // 加锁
    if (r == 0) {
        buf[done] = 0;
        printf("recv: %s\n", buf);
        str2can_frame(cf, buf);
        #if 1
        printf("ftype: %c\n", cf->ftype);
        printf("can id : 0x%x\n", cf->id);
        printf("dlc : 0x%x\n", cf->dlc);
        for (int i = 0;i<2*dlc2len[cf->dlc];i++) {
    	    printf("data: %x, ", cf->data[i]);
        }
        #endif
    }
    return r;
}



