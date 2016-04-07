#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "tftp.h"
int encode_uint16(uint8_t * buf, uint16_t value)
{
    uint8_t * p = buf; 
    *p = ((value >> 8) & 0xff); p++;
    *p = (value & 0xff); p++;

    return sizeof(value);
}

uint16_t decode_uint16(uint8_t * buf) 
{
    uint16_t v; 
    v = (buf[0] << 8) + buf[1];
    return v;
}


tftp_buf_t * tftp_uvbuf_init()
{
    tftp_buf_t * buf = malloc(sizeof(tftp_buf_t) * TFTP_MAX_BUF );
    int i = 0;
    
    for(i = 0; i < TFTP_MAX_BUF; i++){
        buf[i].uvbuf.base = buf[i].buf;
        buf[i].uvbuf.len  = TFTP_MAX_MSGSIZE;
        memset(buf[i].uvbuf.base,0x00,buf[i].uvbuf.len);
    }

    return buf;
}


uv_buf_t * tftp_uvbuf_get(tftp_ctx_t* clt,int type)
{
    TFTP_CHECK(type < TFTP_MAX_BUF); 

    return &clt->buf[type].uvbuf;
}


void tftp_finish(tftp_ctx_t* clt)
{
    uv_close((uv_handle_t *)&clt->udp_handle,NULL);
    uv_close((uv_handle_t *)&clt->timer_req,NULL);
    free(clt->buf);
}


void tftp_timeout(uv_timer_t* handle)
{    
    tftp_ctx_t * ctx = (tftp_ctx_t*)handle->data;
    ctx->retries --; 
    if(ctx->retries > 0){
        uv_buf_t     *uvbuf = tftp_uvbuf_get(ctx,TFTP_SEND_BUF);
        uv_udp_send(
                &ctx->send_req, 
                &ctx->udp_handle,
                uvbuf, 
                1, 
                (const struct sockaddr*) &ctx->addr, 
                NULL);

    }else{
        fprintf(stderr,"Connection timeout \n");
        tftp_finish(ctx);
    }
}

void tftp_start_timer(tftp_ctx_t* clt)
{
    clt->timer_req.data = clt;
    uv_timer_start(&clt->timer_req, tftp_timeout, TFTP_DEF_TIMEOUT_USEC, TFTP_DEF_TIMEOUT_USEC);
    clt->retries = TFTP_DEF_RETRIES;
}

void tftp_recv_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) 
{
    tftp_ctx_t* ctx = (tftp_ctx_t*)handle->data;
    uv_buf_t * recv = tftp_uvbuf_get(ctx,TFTP_RECV_BUF);
    buf->base = recv->base;
    buf->len  = TFTP_MAX_MSGSIZE;
}

void tftp_file_on_write(uv_fs_t *req) 
{
    tftp_ctx_t* ctx = (tftp_ctx_t*)req->data;
    uv_buf_t     *uvbuf = tftp_uvbuf_get(ctx,TFTP_SEND_BUF);
    uv_buf_t     *write_buf = tftp_uvbuf_get(ctx,TFTP_FILE_BUF);

    tftp_build_packet(TFTP_OPCODE_ACK,ctx->block_no,uvbuf);
    uv_udp_send(
            &ctx->send_req, 
            &ctx->udp_handle,
            uvbuf,
            1 , 
            (const struct sockaddr *)&ctx->addr,
            NULL);

    uv_fs_req_cleanup(req);
    /* last block write done */
    if(write_buf->len < TFTP_BLOCKSIZE && write_buf->len != 0){
        tftp_finish(ctx);
        return;
    }
    tftp_start_timer(ctx);
    ctx->state = TFTP_STATE_ACK_SENT;
    ctx->block_no ++;
}

void tftp_file_on_read(uv_fs_t *req) {
    tftp_ctx_t * ctx = (tftp_ctx_t *)req->data;
    uv_buf_t     *uvbuf = tftp_uvbuf_get(ctx,TFTP_SEND_BUF);
    uv_buf_t     *file_uvbuf = tftp_uvbuf_get(ctx,TFTP_FILE_BUF);

    if(req->result <= 0) {
        tftp_finish(ctx);
        return;
    }

    ctx->block_no ++;
    tftp_build_packet(TFTP_OPCODE_DATA,ctx->block_no,uvbuf);
    memcpy(uvbuf->base + 4,file_uvbuf->base, req->result);
    uvbuf->len = 4 + req->result; 

    uv_udp_send(
            &ctx->send_req, 
            &ctx->udp_handle,
            uvbuf,
            1 , 
            (const struct sockaddr *)&ctx->addr,
            NULL);

    tftp_start_timer(ctx);
    
    ctx->state = TFTP_STATE_DATA_SENT;

    uv_fs_req_cleanup(req);
}




void tftp_recv_pkt(
        uv_udp_t *req, 
        ssize_t nread, 
        const uv_buf_t *buf, 
        const struct sockaddr *addr, 
        unsigned int flags) 
{
    uint16_t opcode ; 
    uint16_t blockno;
    tftp_ctx_t* ctx = (tftp_ctx_t*)req->data;
    uv_buf_t * uvbuf = tftp_uvbuf_get(ctx,TFTP_FILE_BUF);

    if(nread < 0){
        tftp_finish(ctx);
        return ;
    }
    if(nread == 0){
        return ;
    }

    memcpy(&ctx->addr,addr,sizeof(struct sockaddr_in));
    opcode = decode_uint16((uint8_t *) buf->base);
    blockno = decode_uint16((uint8_t *) (buf->base + 2));
    switch(opcode){
        case TFTP_OPCODE_DATA:
            switch(ctx->state){
                case TFTP_STATE_RRQ_SENT:
                case TFTP_STATE_ACK_SENT:
                    /*expect block no */
                    if(ctx->block_no == blockno){
                        uvbuf->len = nread - 4;
                        memcpy(uvbuf->base,buf->base + 4,uvbuf->len);
                        ctx->fs_req.data = ctx;
                        uv_fs_write(ctx->loop, 
                                &ctx->fs_req, 
                                ctx->file_handle, 
                                uvbuf, 
                                1, -1, tftp_file_on_write);

                        /* recv right data and stop the retry timer */
                        uv_timer_stop(&ctx->timer_req);
                    }else if(blockno > ctx->block_no){
                        //TODO:send error with unknow TID
                    }
            }
            break;
        case TFTP_OPCODE_ACK:
            switch(ctx->state){
                case TFTP_STATE_WRQ_SENT:
                case TFTP_STATE_DATA_SENT:
                    if(ctx->block_no == blockno){
                        uvbuf->len = TFTP_BLOCKSIZE;
                        ctx->fs_req.data = ctx;
                        uv_fs_read(ctx->loop, &ctx->fs_req, 
                                ctx->file_handle,
                                uvbuf, 1, -1, tftp_file_on_read);

                        uv_timer_stop(&ctx->timer_req);
                    }
                    break;
            }
            break;
        case TFTP_OPCODE_ERROR:
            fprintf(stderr,"Error: %s \n",buf->base + 4);
            tftp_finish(ctx);
            break;
        default:
            fprintf(stderr,"Unknow opcode %d \n",opcode);
            tftp_finish(ctx);
    }
}

