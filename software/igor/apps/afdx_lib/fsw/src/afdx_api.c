/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "msg_ids.h"

#include "afdx_lib.h"
#include "afdx_api.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/*
** Global Variables
*/

/*
** Function Definitions
*/

/**
** Send a message through a given virtual link.
*/
int AFDX_SendMsg(const void* p_data,
                 uint16 len,
                 size_t vl_id)
 {
    int fd;                      /* socket descriptor */
    struct sockaddr_in recvaddr; /* address of receiver */
    struct timespec last;        /* time last frame was sent */
    struct timespec next;        /* time next frame can be sent */   
    ssize_t ret;                 /* number of bytes sent */
    int retc = 0;
     
    assert(p_data != NULL);
    assert(len > 0);
    assert(vl_id < AFDX_LIB_VL_COUNT);
 
    /* Get socket to send through and timestamp from last frame. */   
    fd   = AFDX_SendSocks.socks[vl_id];
    last = AFDX_SendSocks.last[vl_id];
    
    assert(fd != -1);

    memset(&recvaddr, 0x0, sizeof(recvaddr)); 
    
    /* Fill in receiver address. */
    /* Note that we use the broadcast IP to avoid */
    /* needing to do repeating unicast messages. */
    recvaddr.sin_family      = AF_INET;
    recvaddr.sin_port        = htons(AFDX_LIB_GetPortNum(vl_id));
    recvaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    /* Determine when next frame can be sent. */
    next.tv_sec  = last.tv_sec;
    next.tv_nsec = last.tv_nsec + (AFDX_BAG_MS * 1000000); 
    while (next.tv_nsec >= 1000000000) {
        next.tv_nsec -= 1000000000;
        next.tv_sec++;
    }

    /* Wait to send frame until end of BAG. */
    /* If BAG is past, return immediately without suspending thread. */
    do {
        retc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    } while (retc != 0); /* unclear if returns EINTR or sets errno */
    
    /* Update the last frame timestamp. */
    clock_gettime(CLOCK_MONOTONIC, &(AFDX_SendSocks.last[vl_id]));

    /* Send the message. */
    /* If interrupted by a signal, repeat right away. */
    /* If no room to store message, return to avoid blocking. */
    do {   
        ret = sendto(fd, p_data, len, MSG_DONTWAIT, 
                     (const struct sockaddr*)&recvaddr, sizeof(recvaddr));
    } while ((ret == -1) && (errno == EINTR));                 
                  
    if (ret != len) {
        if (ret >= 0) {
            printf("%s: Sent %zu bytes on VL %zu (expected %"PRIu16")\n",
                      LIB_NAME, ret, vl_id, len);        
            return AFDX_FAILED;
        } else if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                   (errno == ENOBUFS)) {
            printf("%s: No room to store message for VL %zu (errno = %s)\n",
                      LIB_NAME, vl_id, strerror(errno));
            return AFDX_FULLBUF;
        } else {
            printf("%s: Error sending on VL %zu (errno = %s)\n",
                      LIB_NAME, vl_id, strerror(errno));        
            return AFDX_FAILED;
        }
    }

    return AFDX_SUCCESS;
}
 
/**
** Read a message from the given virtual link.
*/                 
int AFDX_ReadMsg(void* p_buffer,
                 uint16* p_len,
                 size_t vl_id)
{
    int fd;                              /* socket descriptor */
    ssize_t ret;                         /* number of bytes read */
    uint8 tmp_buffer[AFDX_MAX_MSG_SIZE]; /* temporary buffer */
 
    assert(p_buffer != NULL);
    assert(p_len != NULL); 
    assert(vl_id < AFDX_LIB_VL_COUNT);
     
    /* Get socket to read from. */   
    fd = AFDX_RecvSocks.socks[vl_id];
    
    assert(fd != -1);

    /* Read the latest message. */
    /* If interrupted by a signal, repeat right away. */
    do {
        ret = recvfrom(fd, tmp_buffer, sizeof(tmp_buffer), MSG_DONTWAIT, NULL, NULL); 
    } while ((ret == -1) && (errno == EINTR));

    if (ret < 1) {
        if (ret == 0) {
            return AFDX_NOMSG; /* no message to read */
        } else { /* errno is set */
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                return AFDX_NOMSG; /* no message to read */
            } else {
                printf("%s: Error reading on VL %zu (errno = %s)\n",
                          LIB_NAME, vl_id, strerror(errno));              
                return AFDX_FAILED;
            }
        }
    }

    /* copy data to user buffer. */    
    memcpy(p_buffer, tmp_buffer, ret);
    *p_len = ret;

    return AFDX_SUCCESS;
}

/* Execute some functions to test the library. */              
void AFDX_Test(void)
{
    uint8 data[AFDX_MAX_MSG_SIZE] = {0};
    uint16 len = AFDX_MAX_MSG_SIZE;
    size_t vl_id = 0;
    int ret = 0;
    int i = 0;

    AFDX_LIB_Init();
    
    for (i = 0; i < 50; i++) {
        ret = AFDX_SendMsg(data, len, vl_id);
        if (ret == AFDX_SUCCESS) {
            printf("Sent a %d byte message on VL %zu (message %d)\n", len, vl_id, i + 1);
        } else {
            printf("Error sending message on VL %zu\n", vl_id);
        }        
    }
        
    AFDX_LIB_Cleanup();
    
    return;
}

