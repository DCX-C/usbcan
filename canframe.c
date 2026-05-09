#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "canframe.h"

#define CAN_END                     '.'

int dlc2len[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 
    12, 16, 20, 24, 32, 48, 64};

char dlc2udlc[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', 'A', 'B', 'C', 'D', 'E', 'F', };

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

void get_can_frame(struct can_frame *cf, struct usb_can_frame *ucf)
{
    char strlbuf[16] = {0};
    cf->ftype = ucf->ftype;
    strlbuf[0] = ucf->dlc;
    cf->dlc = strtol(strlbuf, NULL, 16);
    strcpy(strlbuf, ucf->id);
    cf->id = strtol(strlbuf, NULL, 16);
    // printf("ucf->cf\n");
    // printf("ftype: %c\n", cf->ftype);
    // printf("can id : 0x%x\n", cf->id);
    // printf("dlc : 0x%x\n", cf->dlc);
    for (int i = 0;i<2*dlc2len[cf->dlc];i++) {
        printf("%2x ", ucf->data[i]);
    }
    printf("\n");
    for (int i = 0;i<dlc2len[cf->dlc];i++) {
        memcpy(strlbuf, &ucf->data[2*i], 2);
        strlbuf[2] = 0;
        cf->data[i] = strtol(strlbuf, NULL, 16);
        // printf("data : %2x\n", cf->data[i]);
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
    printf("id: 0x%x, ids: %s\n", cf->id, ucf->id);
    
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
    // printf("cf->ucf:\n");
    // printf("ftype: %c\n", ucf->ftype);
    // printf("can id : %s\n", ucf->id);
    // printf("dlc : %c\n", ucf->dlc);
    // printf("data : %s\n", ucf->data);
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
    // printf("can frame 2 str: \n");
    for (int i = 0;i<strlen(str);i++)
    {
        // printf("%02x ", str[i]);
    }
    // printf("\n");
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



