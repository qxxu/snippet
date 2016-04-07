#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "uv.h"
#include "tftpd.h"
#include "tftp.h"
static tftpd_cfg_t cfg = {
    .port = "10240",
    .path = "/home/geesun/tftpboot/",
};

static tftpd_ctx_t ctx ;

void tftp_help(char * app)
{
  printf("Usage:\n%s directory [-P port] \n",app);
  printf("Options:\n-h (help; this message)\n-P port(default is 69)\n");
}


uv_loop_t* create_loop()
{
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    if (loop) {
      uv_loop_init(loop);
    }
    return loop;
}


void tftp_work_process(uv_work_t *req) {
    int flags;
    uv_fs_t      open_req;
    struct sockaddr_in addr;
    tftpd_clt_ctx_t * clt = (tftpd_clt_ctx_t *)req->data;
    uv_buf_t * uvbuf = NULL ;

    clt->ctx.loop = create_loop();
    clt->ctx.buf = tftp_uvbuf_init();
    uv_udp_init(clt->ctx.loop, &clt->ctx.udp_handle);
    uv_timer_init(clt->ctx.loop, &clt->ctx.timer_req);

    uv_ip4_addr("0.0.0.0",0, &addr); 
    uv_udp_bind(&clt->ctx.udp_handle, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR);

    if(clt->opcode == TFTP_OPCODE_WRQ){
        flags = O_RDWR|O_CREAT;
    }else{
        flags = O_RDONLY;
    }

    clt->ctx.file_handle = uv_fs_open(clt->ctx.loop, 
            &open_req, 
            clt->filename, flags, 0644, NULL);
    TFTP_CHECK(clt->ctx.file_handle > 0);

    uv_fs_req_cleanup(&open_req);
    
    uvbuf = tftp_uvbuf_get(&clt->ctx,TFTP_FILE_BUF);

    if(clt->opcode == TFTP_OPCODE_WRQ){
        uvbuf->len = 0; 
        clt->ctx.fs_req.data = &clt->ctx;
        uv_fs_write(clt->ctx.loop, 
                &clt->ctx.fs_req, 
                clt->ctx.file_handle, 
                uvbuf, 
                1, -1, tftp_file_on_write);
        uv_timer_stop(&clt->ctx.timer_req);
    }else{
        uvbuf->len = TFTP_BLOCKSIZE;
        clt->ctx.fs_req.data = &clt->ctx;
        uv_fs_read(clt->ctx.loop, &clt->ctx.fs_req, 
                clt->ctx.file_handle,
                uvbuf, 1, -1, tftp_file_on_read);
        uv_timer_stop(&clt->ctx.timer_req);
    }
    
    clt->ctx.udp_handle.data = &clt->ctx;
    uv_udp_recv_start(&clt->ctx.udp_handle, tftp_recv_alloc_cb, tftp_recv_pkt);
    
    uv_run(clt->ctx.loop, UV_RUN_DEFAULT) ;
    uv_loop_close(clt->ctx.loop);
    free(clt->ctx.loop);
}

void tftp_work_done(uv_work_t *req, int status) {

    tftpd_clt_ctx_t * clt = (tftpd_clt_ctx_t *)req->data;
    free(clt);
}


void tftp_new_connection(const struct sockaddr *addr,char * filename,int opcode)
{
    tftpd_clt_ctx_t * clt = malloc(sizeof(tftpd_clt_ctx_t));
    memset(clt,0x00,sizeof(tftpd_clt_ctx_t));

    clt->work.data = clt; 
    snprintf(clt->filename,TFTP_BLOCKSIZE,"%s/%s",cfg.path,filename);
    clt->opcode = opcode; 
    clt->ctx.state = TFTP_STATE_INIT;
    clt->ctx.block_no = 0;
    clt->ctx.retries = TFTP_DEF_RETRIES;
    memcpy(&clt->ctx.addr,addr,sizeof(struct sockaddr_in));
    uv_queue_work(ctx.loop, &clt->work,tftp_work_process , tftp_work_done);
}

int tftp_build_packet(
        uint16_t opcode, 
        uint16_t block_no,
        uv_buf_t * buf)
{
    int len = 0; 
    int total_len  = 0; 

    memset(buf->base,0x00,buf->len);

    len = encode_uint16((uint8_t *) buf->base,opcode);
    total_len = len;

    switch(opcode){
        case TFTP_OPCODE_ACK:
        case TFTP_OPCODE_DATA:
            total_len += encode_uint16(((uint8_t *) buf->base) + total_len, block_no);
            break;
    }

    buf->len = total_len;
    return total_len;
}


void tftp_svr_recv_pkt(
        uv_udp_t *req, 
        ssize_t nread, 
        const uv_buf_t *buf, 
        const struct sockaddr *addr, 
        unsigned int flags) 
{
    uint16_t opcode ; 
    char * filename;

    if(nread < 0){
        return ;
    }
    if(nread == 0){
        return ;
    }

    opcode = decode_uint16((uint8_t *) buf->base);
    filename = buf->base + 2;
    switch(opcode){
        case TFTP_OPCODE_WRQ:
        case TFTP_OPCODE_RRQ:
            tftp_new_connection(addr,filename,opcode);
            break;
        default:
            fprintf(stderr,"Unexpect opcode %d \n",opcode);
            break;
    }
}

void tftp_svr_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) 
{
    buf->base = ctx.buf;
    buf->len  = TFTP_MAX_MSGSIZE;
}

int tftpd_init()
{
    struct sockaddr_in addr;
    ctx.loop = uv_default_loop();

    uv_udp_init(ctx.loop, &ctx.udp_handle);
    uv_ip4_addr("0.0.0.0",atoi(cfg.port), &addr); 
    uv_udp_bind(&ctx.udp_handle, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&ctx.udp_handle, tftp_svr_alloc_cb, tftp_svr_recv_pkt);
    return 0;
}

int main(int argc, char * argv[])
{
    int c = 0;

    if(argc >= 2){
        cfg.path = argv[1];

        while ((c = getopt(argc - 1 , argv + 1 , "p:h")) >= 0) {
            switch(c) {
                case 'p':
                    cfg.port = optarg;
                    break;
                case 'h':
                default:
                    tftp_help(argv[0]);
                    exit(EXIT_FAILURE);
            }
        }
    }
    
    //daemon(1,1);

    tftpd_init();

    uv_run(ctx.loop,UV_RUN_DEFAULT);
    uv_loop_close(ctx.loop);

    return 0;
}
