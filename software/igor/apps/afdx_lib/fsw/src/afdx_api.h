/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _afdx_api_
#define _afdx_api_

/*
** Public Defines
*/

/* Max number of bytes that can be sent in one message. */
/* Chosen as the maximum payload size for a single AFDX frame with UDP/IP COM ports. */
/* Thus, using the BAG to separate subsequent messages sent on the same VL is equivalent */
/* to seperating subsequent frames on the same VL (what a BAG is for). */
#define AFDX_MAX_MSG_SIZE (1472)

/* Return codes for library. */
#define AFDX_SUCCESS (0)  /* operation was successful */
#define AFDX_FAILED  (-1) /* error completing operation */
#define AFDX_NOMSG   (-2) /* no message to read */
#define AFDX_FULLBUF (-3) /* no room in send buffer */

/*
** Public Structure Declarations
*/

/*
** Exported Global Variables
*/

/*
** Public Function Prototypes
*/

/**
** Send a message through a given virtual link.
**
** \param[in] p_data Pointer to buffer with message to send.
** \param[in] len    Length of message to send in bytes.
** \param[in] vl_id  Virtual link on which to send.
**
** \return AFDX_SUCCESS if message was written to network stack
** \return AFDX_FAILED  if error occurred
** \return AFDX_FULLBUF if no room to store message
*/
int AFDX_SendMsg(const void* p_data,
                 uint16 len,
                 size_t vl_id);
                 
/**
** Read a message from the given virtual link.
**
** \param[out] p_buffer Address where message read is copied.
**                      Must be >= AFDX_MAX_MSG_SIZE bytes!
** \param[out] p_len    Address set to length of message read.
** \param[in] vl_id     Virtual link on which to read.
**
** \return AFDX_SUCCESS if message was read from network stack
** \return AFDX_NOMSG   if no message to read
** \return AFDX_FAILED  if error occurred
*/                 
int AFDX_ReadMsg(void* p_buffer,
                 uint16* p_len,
                 size_t vl_id);
           
/**
** Execute some functions to test the library.
** \return None
*/              
void AFDX_Test(void);

#endif

