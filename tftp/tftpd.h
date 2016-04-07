#ifndef __TFTPD_H__
#define __TFTPD_H__
#include "tftp.h"
typedef struct tftpd_cfg{
    char  * port;
    char  * path;
}tftpd_cfg_t;

typedef struct tftpd_clt_ctx{
    uv_work_t    work;
    uint16_t     opcode;
    char         filename[TFTP_BLOCKSIZE + 1];
    tftp_ctx_t   ctx;
}tftpd_clt_ctx_t;

typedef struct tftpd_ctx{
    uv_loop_t   *loop;
    uv_udp_t     udp_handle;
    char        buf[TFTP_MAX_MSGSIZE];
}tftpd_ctx_t;

#endif
