#ifndef __TFTP_H__
#define __TFTP_H__
#include "uv.h"
/*
 * See RFC 1350 section 5 and the appendix.
 */

#define TFTP_OPCODE_RRQ		1
#define TFTP_OPCODE_WRQ		2
#define TFTP_OPCODE_DATA	3
#define TFTP_OPCODE_ACK		4
#define TFTP_OPCODE_ERROR	5


#define TFTP_ERR_NOT_DEFINED	0
#define TFTP_ERR_NOT_FOUND	1
#define TFTP_ERR_ACCESS_DENIED	2
#define TFTP_ERR_DISK_FULL	3
#define TFTP_ERR_UNKNOWN_TID	4
#define TFTP_ERR_ILLEGAL_OP	5
#define TFTP_ERR_FILE_EXISTS	6
#define TFTP_ERR_NO_SUCH_USER	7

#define TFTP_MODE_OCTET		"octet"
#define TFTP_MODE_NETASCII	"netascii"
#define TFTP_MODE_MAIL		"mail"


#define TFTP_DEF_RETRIES	2
#define TFTP_DEF_TIMEOUT_USEC	5000
#define TFTP_BLOCKSIZE		512
#define TFTP_MAX_MSGSIZE	(4 + TFTP_BLOCKSIZE)

typedef enum { 
    TFTP_STATE_INIT	           , 
    TFTP_STATE_RRQ_SENT	       , 
    TFTP_STATE_WRQ_SENT	       , 
    TFTP_STATE_DATA_SENT       , 
    TFTP_STATE_ACK_SENT	       , 
} tftp_state_e;


typedef enum{
    TFTP_RECV_BUF = 0,
    TFTP_FILE_BUF = 1,
    TFTP_SEND_BUF = 2,
    TFTP_MAX_BUF ,
}tftp_buf_type_e;

typedef struct{
    uv_buf_t    uvbuf;
    char        buf[TFTP_MAX_MSGSIZE];
}tftp_buf_t;

typedef struct tftp_ctx {
    uint16_t     state;
    uint16_t     block_no;	
    int16_t      retries;
    struct sockaddr_in addr;
    uv_loop_t   *loop;

    uv_file             file_handle;

    uv_udp_t            udp_handle;
    uv_fs_t             fs_req;
    uv_udp_send_t       send_req;
    uv_timer_t          timer_req; 

    tftp_buf_t         *buf;
} tftp_ctx_t;

# define TFTP_CHECK(exp)   do { if (!(exp)) abort(); } while (0)

int tftp_build_packet(
        uint16_t opcode, 
        uint16_t block_no,
        uv_buf_t * buf);

int encode_uint16(uint8_t * buf, uint16_t value);
uint16_t decode_uint16(uint8_t * buf) ;


tftp_buf_t * tftp_uvbuf_init();
uv_buf_t * tftp_uvbuf_get(tftp_ctx_t* clt,int type);
void tftp_finish(tftp_ctx_t* clt);
void tftp_timeout(uv_timer_t* handle);
void tftp_start_timer(tftp_ctx_t* clt);

void tftp_recv_alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) ;

void tftp_file_on_write(uv_fs_t *req) ;
void tftp_file_on_read(uv_fs_t *req);


void tftp_recv_pkt(
        uv_udp_t *req, 
        ssize_t nread, 
        const uv_buf_t *buf, 
        const struct sockaddr *addr, 
        unsigned int flags);
#endif
