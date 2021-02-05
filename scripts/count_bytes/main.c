/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Count the number of bytes of broadcast traffic sent
** from a standard Ethernet switch.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ether.h>

/*
** Function Declarations
*/

/*
** Constants
*/

#define LOG_FILE_PATH ("./log.txt")

/*
** Global Variables
*/

/*
** Function Definitions
*/

int main(void)
{
    FILE* p_log = NULL; /* file to write to */
    int fd = 0;         /* socket descriptor */
    ssize_t ret;        /* number of bytes read */
    char buffer[65535]; /* temporary buffer */
    
    size_t total_bytes = 0;
 
    /* Open file to write to. */       
    if ((p_log = fopen(LOG_FILE_PATH, "w")) == NULL) {
        printf("Error opening file %s\n", LOG_FILE_PATH);
        return 0; 
    }  

    /* Create the socket */ 
    if ((fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) { 
        printf("Error creating receive socket\n");      
        return 0;
    }   

    /* Set the socket to non-blocking. */
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        printf("Error setting receive socket to non-blocking\n");    
        close(fd);
        return 0;
    }
    
    while (1) {       
        ret = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT, NULL, NULL); 
        if (ret > 0) {
            total_bytes += ret;
            fprintf(p_log, "%zu\n", total_bytes);
            fflush(p_log);
            printf("total bytes: %zu\n", total_bytes);
        }
    }    

    return 0;
}

