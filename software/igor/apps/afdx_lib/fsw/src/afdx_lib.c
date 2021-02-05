/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "msg_ids.h"

#include "afdx_lib.h"
#include "afdx_lib_vldefs.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/* Get the number of elements in an array. */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
** Global Variables
*/

/* Exported socket tables. */
AFDX_SockTable_t AFDX_SendSocks;
AFDX_SockTable_t AFDX_RecvSocks;

/*
** Function Definitions
*/

/* Return port number for a given VL ID. */
int AFDX_LIB_GetPortNum(int vl_id)
{
    return (61000 + vl_id);    
}

/* Entry point for the library. */
int32 AFDX_LIB_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
int AFDX_LIB_Init(void)
{        
    AFDX_VlEntry_t vl_entry;
    int vl_id = 0;
    int sender_id = 0;
    int fd = 0;
    int enable_bcast = 1;
    bool ip_found = false;
    int cpu_id = 0;
    char* ip_addr;
    char* p_token;
    char tmp_array[100];
    int recv_id = 0;  
    bool is_recv = false;
    struct sockaddr_in addr;
    int i = 0;
    int j = 0;
      
    /* Initialize send sockets. */
    memset(&AFDX_SendSocks, 0x0, sizeof(AFDX_SendSocks));
    for (i = 0; i < NELEMS(AFDX_SendSocks.socks); i++) {
        AFDX_SendSocks.socks[i] = -1;
        clock_gettime(CLOCK_MONOTONIC, &(AFDX_SendSocks.last[i]));        
    }
    
    /* Initialize receive sockets. */
    memset(&AFDX_RecvSocks, 0x0, sizeof(AFDX_RecvSocks));    
    for (i = 0; i < NELEMS(AFDX_RecvSocks.socks); i++) {
        AFDX_RecvSocks.socks[i] = -1;   
    }
    
    /* Configure sockets for VLs. */
    for (i = 0; i < NELEMS(AFDX_LIB_VlTable); i++)
    {
        vl_entry = AFDX_LIB_VlTable[i];
        if (vl_entry.is_used)
        {
            vl_id = vl_entry.vl_id;
            sender_id = atoi(vl_entry.src_id);
            
            if (sender_id == CFE_PLATFORM_CPU_ID) /* you are the sender */
            {
                /* Create the socket */ 
                if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
                    printf("%s: Error creating send socket for VL %d\n", LIB_NAME, vl_id);    
                    goto afdx_cleanup;
                } else {
                    AFDX_SendSocks.socks[vl_id] = fd;
                }
              
                /* Enable broadcast permissions for socket. */     
                if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable_bcast, sizeof(enable_bcast)) < 0) {
                    printf("%s: Error setting send socket permissions for VL %d\n", LIB_NAME, vl_id);    
                    goto afdx_cleanup;
                }

                /* Lookup IP address to bind to. */
                /* This is needed to keep from using the WLAN IP. */
                ip_found = false;
                for (j = 0; j < NELEMS(AFDX_LIB_IpTable); j++) {
                    cpu_id  = AFDX_LIB_IpTable[j].cpu_id;
                    ip_addr = AFDX_LIB_IpTable[j].ip_addr;
                    if (cpu_id == CFE_PLATFORM_CPU_ID) {
                        addr.sin_addr.s_addr = inet_addr(ip_addr);
                        ip_found = true;
                        break;
                    }
                }
                
                if (ip_found == false) {
                    printf("%s: Can't find IP address for CPU %d\n", LIB_NAME, CFE_PLATFORM_CPU_ID);    
                    goto afdx_cleanup;                
                }

                addr.sin_family = AF_INET;                 
                addr.sin_port   = 0; /* random port */                                  
                  
                /* Bind send socket to a secific IP address. */
                if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    printf("%s: Error binding send socket for VL %d to IP %s (errno = %s)\n",
                              LIB_NAME, vl_id, ip_addr, strerror(errno));      
                    goto afdx_cleanup;
                }
            }
            
            /* Determine if you are a receiver. */
            is_recv = false;
            strcpy(tmp_array, vl_entry.dst_ids);
            p_token = strtok(tmp_array, ",");
            while (p_token != NULL) {
                sscanf(p_token, "%d", &recv_id);
                if (recv_id == CFE_PLATFORM_CPU_ID) {
                    is_recv = true;
                    break;
                }
                p_token = strtok(NULL, ",");
            }
                   
            if (is_recv) /* you are a receiver */
            {
                /* Create the socket */ 
                if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
                    printf("%s: Error creating receive socket for VL %d\n", LIB_NAME, vl_id);      
                    goto afdx_cleanup;
                } else {               
                    AFDX_RecvSocks.socks[vl_id] = fd;
                }            
                       
                /* Enable broadcast permissions for socket. */     
                if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable_bcast, sizeof(enable_bcast)) < 0) {
                    printf("%s: Error setting receive socket permissions for VL %d\n", LIB_NAME, vl_id);    
                    goto afdx_cleanup;
                }

                /* Set the socket to non-blocking. */
                if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
                    printf("%s: Error setting receive socket for VL %d to non-blocking\n", LIB_NAME, vl_id);    
                    goto afdx_cleanup;                
                }
                  
                addr.sin_family      = AF_INET; 
                addr.sin_addr.s_addr = INADDR_ANY; 
                addr.sin_port        = htons(AFDX_LIB_GetPortNum(vl_id));                  
                  
                /* Bind receive socket to a specific port. */
                if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    printf("%s: Error binding receive socket for VL %d to port %d (errno = %s)\n",
                              LIB_NAME, vl_id, ntohs(addr.sin_port), strerror(errno));      
                    goto afdx_cleanup;
                }
            }
        }
    }

    printf("%s: Lib Initialized\n", LIB_NAME);

    return 0;
    
afdx_cleanup:

     AFDX_LIB_Cleanup();
     return -1;
}

/* Cleanup library after shutdown. */
void AFDX_LIB_Cleanup(void)
{
    int fd = 0;
    int i = 0;

    /* Cleanup send sockets. */
    for (i = 0; i < NELEMS(AFDX_SendSocks.socks); i++) {
        fd = AFDX_SendSocks.socks[i];
        if (fd != -1) {
            close(fd);
            AFDX_SendSocks.socks[i] = -1;
        }
    }    
    
    /* Cleanup receive sockets. */
    for (i = 0; i < NELEMS(AFDX_RecvSocks.socks); i++) {
        fd = AFDX_RecvSocks.socks[i];
        if (fd != -1) {        
            close(fd);
            AFDX_RecvSocks.socks[i] = -1;
        }           
    }    

    printf("%s: Cleaned up library\n", LIB_NAME);  
    return;
}

