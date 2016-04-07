#ifndef __TFTPC_H__
#define __TFTPC_H__
#include "tftp.h"
typedef struct tftpc_cfg {
    char   *host;		
    char   *port;	
    char   *mode;		
    char   *file;	
}tftpc_cfg_t;


void tftp_finish();
#endif
