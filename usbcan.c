#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "fcntl.h"
#include "unistd.h"
#include "pthread.h"

#include "./inc/libusb.h"

#define CAN_STD_FRAME               't'
#define CAN_ETX_FRAME               'T'
#define CANFD_STD_FRAME             'd'
#define CANFD_EXT_FRAME             'D'
#define CANFD_STD_FRAME_BRS         'b'
#define CANFD_EXT_FRAME_BRS         'B'
#define CAN_END '.'


#define CANSTRF_TYPE   "%c"
#define CANSTRF_STDID     "%03x"  
#define CANSTRF_EXTID     "%08x"  
#define CANSTRF_DLC    "%c"
#define CANSTRF_END    "%c"
#define CANSTRF_DAT    "%s"


int dlc2len[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 
    12, 16, 20, 24, 32, 48, 64};


char dlc2udlc[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', 'A', 'B', 'C', 'D', 'E', 'F', };


#define EP_OUT 0x01
#define EP_IN  0x81


int cdlc2len(char dlc)
{
    if (dlc < '9') {
        return dlc2len[dlc-'0'];
    } else if (dlc == '9') {
        return dlc2len[9];
    } else {
        return dlc2len[10+dlc-'a'];
    }
}


char start_measure[2][3] = {
    {0x53, 0x38, 0x0d},
    {0x4f, 0x0d, 0x0a},
};

char stop_measure[2] = {
    0x43, 0x0d
};

struct can_frame {
    char ftype;
    uint32_t id;
    uint32_t dlc;
    char data[64];
};

struct usb_can_frame {
    char ftype;
    char id[9];
    char dlc;
    char data[];
};



void get_can_frame(struct can_frame *cf, struct usb_can_frame *ucf)
{
    char strlbuf[16] = {0};
    cf->ftype = ucf->ftype;
    strlbuf[0] = ucf->dlc;
    cf->dlc = strtol(strlbuf, NULL, 16);
    strcpy(strlbuf, ucf->id);
    cf->id = strtol(strlbuf, NULL, 16);
    printf("ucf->cf\n");
    printf("ftype: %c\n", cf->ftype);
    printf("can id : 0x%x\n", cf->id);
    printf("dlc : 0x%x\n", cf->dlc);
    for (int i = 0;i<2*dlc2len[cf->dlc];i++) {
        printf("%2x ", ucf->data[i]);
    }
    printf("\n");
    for (int i = 0;i<dlc2len[cf->dlc];i++) {
        memcpy(strlbuf, &ucf->data[2*i], 2);
        strlbuf[2] = 0;
        cf->data[i] = strtol(strlbuf, NULL, 16);
        printf("data : %2x\n", cf->data[i]);
    }

}

void get_usbcan_frame(struct can_frame *cf, struct usb_can_frame *ucf)
{
    ucf->ftype = cf->ftype;
    if (cf->ftype > 'A' && cf->ftype < 'Z') {
        sprintf(ucf->id, CANSTRF_EXTID, cf->id);
    } else {
        sprintf(ucf->id, CANSTRF_STDID, cf->id);
    }
    
    ucf->dlc = dlc2udlc[cf->dlc];
    char data_str[16];
    for (int i = 0;i<dlc2len[cf->dlc];i++) 
    {
        if (cf->ftype > 'A' && cf->ftype < 'Z') {
            sprintf(data_str, "%2X", cf->data[i]);
        } else {
            sprintf(data_str, "%2X", cf->data[i]);
        }
        if (data_str[0] == ' ') {
            data_str[0] = '0';
            if (data_str[1] == ' ') {
                data_str[1] = '0';
            }
        }
        memcpy(&ucf->data[2*i], data_str, 2);
    }
    ucf->data[2*dlc2len[cf->dlc]] = 0;
    printf("cf->ucf:\n");
    printf("ftype: %c\n", ucf->ftype);
    printf("can id : %s\n", ucf->id);
    printf("dlc : %c\n", ucf->dlc);
    printf("data : %s\n", ucf->data);
}

void can_frame2str(struct can_frame *f, char *str)
{
    struct usb_can_frame *uf = malloc(sizeof(struct usb_can_frame) + 2*dlc2len[f->dlc]+1);
    if (!uf) {
        printf("malloc uf fail, %s\r\n", __FUNCTION__);
        return;
    }

    get_usbcan_frame(f, uf);
    //ext frame
    if (f->ftype > 'A' && f->ftype < 'Z') {
        str[0] = uf->ftype;
        memcpy(&str[1], uf->id, 8);
        str[9] = uf->dlc;
        memcpy(&str[10], uf->data, 2*dlc2len[f->dlc]);
        str[10+2*dlc2len[f->dlc]] = 0x0d;
        str[10+2*dlc2len[f->dlc]+1] = 0;
    } else {
        str[0] = uf->ftype;
        memcpy(&str[1], uf->id, 3);
        str[4] = uf->dlc;
        memcpy(&str[5], uf->data, 2*dlc2len[f->dlc]);
        str[5+2*dlc2len[f->dlc]] = 0x0d;
        str[5+2*dlc2len[f->dlc]+1] = 0;
    }
    printf("can frame 2 str: \n");
    for (int i = 0;i<strlen(str);i++)
    {
        printf("%02x ", str[i]);
    }
    printf("\n");
    free(uf);
}

void str2can_frame(struct can_frame *f, char *str)
{
    struct usb_can_frame *uf = malloc(sizeof(struct usb_can_frame) + 2*dlc2len[0xf]+1);
    if (!uf) {
        printf("malloc uf fail, %s\r\n", __FUNCTION__);
        return;
    }

    uint32_t dlc;
    char dlc_str[2] = {0};
    //ext frame
    if (str[0] > 'A' && str[0] < 'Z') {
        uf->ftype = str[0];
        memcpy(uf->id, &str[1], 8);
        uf->id[9] = 0;
        uf->dlc = str[9];
        dlc_str[0] = str[9];
        dlc = strtol(dlc_str, NULL, 16);
        memcpy(uf->data, &str[10], 2*dlc2len[dlc]);
        uf->data[2*dlc2len[dlc]] = 0;
    } else {
        uf->ftype = str[0];
        memcpy(uf->id, &str[1], 3);
        uf->id[4] = 0;
        uf->dlc = str[4];
        dlc_str[0] = str[4];
        dlc = strtol(dlc_str, NULL, 16);
        memcpy(uf->data, &str[5], 2*dlc2len[dlc]);
        uf->data[2*dlc2len[dlc]] = 0;
    }
    get_can_frame(f, uf);
    free(uf);
}



struct ucan {
	libusb_device_handle *udh;
	pthread_mutex_t mutex; 
};


struct ucan gcan = {
	.udh = NULL, 
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

int usbcan_init(struct ucan *can)
{
    int r;
    ssize_t cnt;
    int done;
 
    r = libusb_init(NULL);
    if (r < 0)
        return r;
 
    can->udh = libusb_open_device_with_vid_pid(NULL, (uint16_t)0x16d0, (uint16_t)0x117e);
    if (can->udh == NULL) {
        printf("libusb open fail\n");
    }

    libusb_set_auto_detach_kernel_driver(can->udh, 1);
    r = libusb_claim_interface(can->udh, 1);
    if (r != LIBUSB_SUCCESS) {
        libusb_close(can->udh);
        printf("claim interface fail\n");
        return 1;
    }
    
    uint8_t buf[16];
    //class 0x21 d2h
    memset(buf, 0, 16);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    //class 0x20 h2d
    memset(buf, 0, 16);
    buf[0] = 0x40;
    buf[1] = 0x42;
    buf[2] = 0x0f;
    buf[6] = 0x08;
    libusb_control_transfer(can->udh, 0x21, 0x20, 0x0000, 0x0000, buf, 7, 1000);
    
    memset(buf, 0, 16);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    //class 0x22 h2d
    libusb_control_transfer(can->udh, 0x21, 0x22, 0x0000, 0x0000, NULL, 0, 1000);
    
    //class 0x20 h2d
    memset(buf, 0, 16);
    buf[0] = 0;
    buf[1] = 0xc2;
    buf[2] = 0x01;
    buf[6] = 0x08;
    libusb_control_transfer(can->udh, 0x21, 0x20, 0x0000, 0x0000, buf, 7, 1000);
    
    memset(buf, 0, 16);
    libusb_control_transfer(can->udh, 0xa1, 0x21, 0x0000, 0x0000, buf, 7, 1000);
    printf("class 0x21 ret: %x %x %x %x\n", buf[0], buf[1], buf[2], buf[6]);
    
    
    
    //libusb_bulk_transfer(can->udh, EP_OUT, stop_measure, 2, &r, 1000);
    usleep(4000);
    libusb_bulk_transfer(can->udh, EP_OUT, start_measure[0], 3, &r, 1000);
    usleep(4000);
    libusb_bulk_transfer(can->udh, EP_OUT, start_measure[1], 3, &r, 1000);

    printf("start measure\n");
    usleep(5);
    return 0;
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
    printf("send len: %d\n", strlen(data));
    pthread_mutex_lock(&can->mutex); // 加锁
    libusb_bulk_transfer(can->udh, EP_OUT, data, strlen(data), &done, 1000);
    pthread_mutex_unlock(&can->mutex); // 加锁
    return done;
}


void *usbcan_recv()
{
    int r;
    int done;
    char buf[256];
    struct can_frame cf;
    while(1)
    {
        pthread_mutex_lock(&gcan.mutex); // 加锁
        r = libusb_bulk_transfer(gcan.udh, EP_IN, buf, 256, &done, 10);
        pthread_mutex_unlock(&gcan.mutex); // 加锁
        if (r == 0) {
            buf[done] = 0;
            printf("str2cf\n");
            str2can_frame(&cf, buf);
            #if 0
            printf("ftype: %c\n", cf.ftype);
     	    printf("can id : 0x%x\n", cf.id);
    	    printf("dlc : 0x%x\n", cf.dlc);
    	    for (int i = 0;i<2*dlc2len[cf.dlc];i++) {
        	printf("data: %x, ", cf.data[i]);
    	    }
    	    #endif
        }
        usleep(100);
    }
}

int main()
{
    usbcan_init(&gcan);
    pthread_t tid;
    pthread_mutex_init(&gcan.mutex, NULL);
    struct can_frame cf = {
        .ftype = CAN_STD_FRAME,
        .id = 0x11,
        .dlc = 4,
        .data = {0x11, 0x22, 0x33, 0x44},
    };
    
    usbcan_send(&gcan, &cf);
    cf.id = 0x22;
    usbcan_send(&gcan, &cf);
    cf.id = 0x33;
    usbcan_send(&gcan, &cf);
    
    if (pthread_create(&tid, NULL, usbcan_recv, NULL) != 0) {
        printf("ticker thread create fail\n");
        return 1;
    } 
    printf("pthread id:%d, create\n", tid);
    
    while(1);
    
    usbcan_exit(&gcan);
    return 0;
}

