#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "uv.h"
#include "tftpc.h"

static tftpc_cfg_t cfg = {
    .host = "127.0.0.1",
    .port = "69",
    .mode = TFTP_MODE_OCTET,
};

static tftp_ctx_t ctx = {
    .state = TFTP_STATE_INIT,
    .block_no = 0,
    .retries = TFTP_DEF_RETRIES,
};


int tftp_build_packet(
        uint16_t opcode, 
        uint16_t block_no,
        uv_buf_t * buf)
{
    uint8_t * p = NULL;
    int len = 0; 
    int total_len  = 0; 

    memset(buf->base,0x00,buf->len);

    len = encode_uint16((uint8_t *) buf->base,opcode);
    total_len = len;

    switch(opcode){
        case  TFTP_OPCODE_RRQ:
        case  TFTP_OPCODE_WRQ:
            p = (uint8_t *)buf->base + total_len; 
            len = strlen(cfg.file) + 1 + strlen(cfg.mode) + 1; 
            if(total_len + len > TFTP_MAX_MSGSIZE){
                return 0;
            }

            len = strlen(cfg.file) + 1; 
            memcpy(p, cfg.file,len); 
            total_len += len; 

            p = p + len; 

            len = strlen(cfg.mode) + 1; 
            memcpy(p, cfg.mode,len); 
            total_len += len; 
            break;
        case TFTP_OPCODE_ACK:
        case TFTP_OPCODE_DATA:
            total_len += encode_uint16(((uint8_t *) buf->base) + total_len, block_no);
            break;
    }

    buf->len = total_len;
    return total_len;
}


int tftp_init(uint16_t opcode)
{
    int flags;
    uv_fs_t      open_req;

    ctx.loop = uv_default_loop();
    ctx.buf = tftp_uvbuf_init();

    uv_udp_init(ctx.loop, &ctx.udp_handle);
    uv_timer_init(ctx.loop, &ctx.timer_req);

    uv_ip4_addr(cfg.host,atoi(cfg.port), &ctx.addr); 
    uv_udp_bind(&ctx.udp_handle, (const struct sockaddr *)&ctx.addr, UV_UDP_REUSEADDR);
    ctx.udp_handle.data = &ctx;
    uv_udp_recv_start(&ctx.udp_handle, tftp_recv_alloc_cb, tftp_recv_pkt);

    if(opcode == TFTP_OPCODE_RRQ){
        flags = O_RDWR|O_CREAT;
    }else{
        flags = O_RDONLY;
    }

    ctx.file_handle = uv_fs_open(ctx.loop, 
            &open_req, 
            cfg.file, flags, 0644, NULL);

    TFTP_CHECK(ctx.file_handle > 0);

    uv_fs_req_cleanup(&open_req);
    return 0;
}

int tftp_start(uint16_t opcode)
{
    uv_buf_t     *uvbuf = tftp_uvbuf_get(&ctx,TFTP_SEND_BUF);
    
    tftp_build_packet(opcode,0,uvbuf);

    uv_udp_send(
            &ctx.send_req, 
            &ctx.udp_handle,
            uvbuf, 
            1, 
            (const struct sockaddr*) &ctx.addr, 
            NULL);

    if(opcode == TFTP_OPCODE_RRQ){
        ctx.state = TFTP_STATE_RRQ_SENT; 
        ctx.block_no = 1;
    }else{
        ctx.state = TFTP_STATE_WRQ_SENT; 
        ctx.block_no = 0;
    }

    tftp_start_timer(&ctx);

    return 0;
}

void tftp_help(char * app)
{
  printf("Usage:\n%s server [-h] [-P port] [-g] | [-p] [file-name] [-m mode]\n",app);
  printf("Options:\n-h (help; this message)\n-P port(default is 69)\n-g (get a file from the server)\n-p (send a file to the server)\n");
  printf("-m binary | octet for octet file transfer (default).\n-m ascii | netascii for netascii file transfer\n");

}

int main(int argc, char * argv[])
{
    int c = 0;
    uint16_t opcode = 0;
    
    if(argc < 2){
        tftp_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    cfg.host = argv[1];

    while ((c = getopt(argc - 1 , argv + 1 , "P:p:g:m:h")) >= 0) {
        switch (c) {
            case 'P':
                cfg.port = optarg;
                break;
            case 'g':
                cfg.file = optarg;
                opcode = TFTP_OPCODE_RRQ;
                break;
            case 'p':
                cfg.file = optarg;
                opcode = TFTP_OPCODE_WRQ;
                break;
            case 'm':
                if(strncmp(optarg,"binary",strlen("binary")) == 0
                            || strncmp(optarg,"octet",strlen("octet")) == 0){
                    cfg.mode = TFTP_MODE_OCTET;
                }else if(strncmp(optarg,"ascii",strlen("ascii")) == 0
                            || strncmp(optarg,"netascii",strlen("netascii")) == 0){
                    cfg.mode = TFTP_MODE_NETASCII;
                }else{
                    fprintf(stderr,"%s invalid mode: %s\n",argv[0],optarg);
                }
                break;
            case 'h':
            default:
                tftp_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if(opcode == 0){
        tftp_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    tftp_init(opcode);
    tftp_start(opcode);

    uv_run(ctx.loop,UV_RUN_DEFAULT);
    uv_loop_close(ctx.loop);
    return 0;

}
