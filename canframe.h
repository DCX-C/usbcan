#ifndef _CANFRAME_H_
#define _CANFRAME_H_


#define CAN_STD_FRAME               't'
#define CAN_ETX_FRAME               'T'
#define CANFD_STD_FRAME             'd'
#define CANFD_EXT_FRAME             'D'
#define CANFD_STD_FRAME_BRS         'b'
#define CANFD_EXT_FRAME_BRS         'B'


#define CANSTRF_TYPE      "%c"
#define CANSTRF_STDID     "%03x"  
#define CANSTRF_EXTID     "%08x"  
#define CANSTRF_DLC       "%c"
#define CANSTRF_END       "%c"
#define CANSTRF_DAT       "%s"

struct can_frame {
    char ftype;
    uint32_t id;
    uint32_t dlc;
    unsigned char data[64];
};

struct usb_can_frame {
    char ftype;
    char id[9];
    char dlc;
    char data[256];
};

void can_frame2str(struct can_frame *f, char *str);
void str2can_frame(struct can_frame *f, char *str);

extern int dlc2len[];


#endif
