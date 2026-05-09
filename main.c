#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "fcntl.h"
#include "unistd.h"
#include "pthread.h"
#include "semaphore.h"

#include "./inc/libusb.h"
#include "canframe.h"
#include "usbcan.h"

sem_t sem_recv;

volatile struct can_frame cf_recv;
void * cf_recv_thread(void *arg)
{   
    struct ucan *can = (void *)arg;
    int r;
    while(1)
    {
        r = usbcan_recv(can, &cf_recv);
        if (!r) 
            sem_post(&sem_recv);
    }
}

int usbcan_send_check(struct ucan *can, struct can_frame *cf)
{
    memset(&cf_recv, 0, sizeof(struct can_frame));
    usbcan_send(can, cf);
    sem_wait(&sem_recv);
    if (memcmp(cf, &cf_recv, sizeof(struct can_frame)-64)) {
        printf("usbcan_send_check, check fail\r\n");
        printf("ftype: [%c, %c] \n", cf->ftype, cf_recv.ftype);
        printf("id: [0x%x, 0x%x] \n", cf->id, cf_recv.id);
        printf("dlc: [0x%x, 0x%x] \n", cf->dlc, cf_recv.dlc);
        return 1;
    } 
    for (int i = 0;i<dlc2len[cf->dlc];i++) {
        if (cf->data[i] != cf_recv.data[i]) {
            printf("data[%d], [0x%x, 0x%x]\n", cf->data[i], cf_recv.data[i]);
        }
    }
    return 0;
}

#define CAN_STD_FRAME               't'
#define CAN_ETX_FRAME               'T'
#define CANFD_STD_FRAME             'd'
#define CANFD_EXT_FRAME             'D'
#define CANFD_STD_FRAME_BRS         'b'
#define CANFD_EXT_FRAME_BRS         'B'

int usbcan20_frame_test(struct ucan *can)
{
    int ret = 0;
    char ftype[] = {CAN_STD_FRAME, CAN_ETX_FRAME};
    int canid[2][16] = {
        {   
            0x1,    0x5,    0x8,    0xa, 
            0x1a,   0x5c,   0x82,   0xac,
            0x121,  0x5b1,  0x6ce,  0x7bc,
            0x1b2,  0x332,  0x4ce,  0x7ff,
        },
        {
            0x1,            0x5,            0x8,            0xa, 
            0x121,          0x5b1,          0x6ce,          0x7bc,
            0x32acd,        0x482af,        0x61c8c,        0x7117a,
            0x456789a,      0x174819a2,     0x26487316,     0x3ffffff,
        },
    } ;

    struct can_frame cf;
    memset(&cf, 0, sizeof(struct can_frame));
    for (int ft = 0;ft<sizeof(ftype);ft++)
    {
        cf.ftype = ftype[ft];
        for (int id = 0;id<32;id++)
        {
            cf.id = canid[id/2][id%16];
            for (int dlc = 0;dlc<8;dlc++)
            {
                cf.dlc = dlc;
                for (int l = 0;l<dlc2len[dlc];l++)
                {
                    cf.data[l] = rand()%0xff;
                }
                ret = usbcan_send_check(can, &cf);
                if (ret) {
                    return 1;
                }
            }
        }
    } 
    return 0;
}

int usbcanfd_frame_test(struct ucan *can)
{
    int ret = 0;
    char ftype[] = {CANFD_STD_FRAME, CANFD_EXT_FRAME, 
                    CANFD_STD_FRAME_BRS, CANFD_EXT_FRAME_BRS};
    int canid[2][16] = {
        {   
            0x1,    0x5,    0x8,    0xa, 
            0x1a,   0x5c,   0x82,   0xac,
            0x121,  0x5b1,  0x6ce,  0x7bc,
            0x1b2,  0x332,  0x4ce,  0x7ff,
        },
        {
            0x1,            0x5,            0x8,            0xa, 
            0x121,          0x5b1,          0x6ce,          0x7bc,
            0x32acd,        0x482af,        0x61c8c,        0x7117a,
            0x456789a,      0x174819a2,     0x26487316,     0x3ffffff,
        },
    } ;

    struct can_frame cf;
    memset(&cf, 0, sizeof(struct can_frame));
    for (int ft = 0;ft<sizeof(ftype);ft++)
    {
        cf.ftype = ftype[ft];
        for (int id = 0;id<32;id++)
        {
            cf.id = canid[id/2][id%16];
            for (int dlc = 0;dlc<8;dlc++)
            {
                cf.dlc = dlc;
                for (int l = 0;l<dlc2len[dlc];l++)
                {
                    cf.data[l] = rand()%0xff;
                }
                ret = usbcan_send_check(can, &cf);
                if (ret) {
                    return 1;
                }
            }
        }
    } 
    return 0;
}



int main()
{
    struct ucan * can = usbcan_init();
    sem_init(&sem_recv, 0, 0);

    pthread_t tid;
    if (pthread_create(&tid, NULL, cf_recv_thread, (void*)can) != 0) {
        printf("ticker thread create fail\n");
        return 1;
    } 
    printf("pthread id:%d, create\n", tid);
    
    struct can_frame cf;
    cf.id = 0x123;
    cf.dlc = 2;
    cf.ftype = 't';
    cf.data[0]=1;
    
    usbcan_send(can, &cf);
    
    while(1)
    {
        if (usbcan20_frame_test(can)) {
            printf("usbcan20_frame_test fail\n");
            break;
        }
        if (usbcanfd_frame_test(can)) {
            printf("usbcanfd_frame_test fail\n");
            break;
        }
    }
    
    sem_destroy(&sem_recv);
    usbcan_exit(can);
    return 0;
}

