/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _bcast_lib_
#define _bcast_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>

/*
** Public Defines
*/

/* Maximum size of data to broadcast in bytes. */
/* Must be <= AFDX_MAX_MSG_SIZE */
#define BCAST_MAX_DATA_SIZE (1400)

/* Max instances of broadcast. */
/* Must agree with the AFDX config table. */
#define BCAST_MAX_INSTANCES (50)

/* Index in VL table where broadcast VLs start. */
/* See afdx_lib_vls.c in igor_defs/tables/ */
#define BCAST_VL_OFFSET (0)

/* Maximum number of nodes in an EIG tree. */
/* Must be large enough to handle the n = 10, f = 4 case. */
#define BCAST_MAX_TREE_NODES (3700)

/* Maximum depth of the EIG tree. */
/* Used as the maximum length of the path in bytes. */
#define BCAST_MAX_TREE_HEIGHT (5)

/* Define messages with special meaning in protocol(s). */
/* See Section 3.1: https://leepike.github.io/pubs/pike-nfm17.pdf */
#define BCAST_DEF_VALUE (0x01) /* default value */
#define BCAST_ERR_VALUE (0xBC) /* received a manifest-faulty message */
#define BCAST_REP_VALUE (0xCD) /* report a manifest-faulty message */

/*
** Public Structure Declarations
*/

/* A single node in an EIG tree. */
/* This is only public to facilitate testing. */
typedef struct BCAST_EigNode {
    uint8 path[BCAST_MAX_TREE_HEIGHT];                 /* Path to node from root of tree. */
                                                       /* Only the first (depth) bytes are used. */
    size_t depth;                                      /* Depth of node from 1 to (f + 1). */
    struct BCAST_EigNode* children[MAX_REPLICA_COUNT]; /* Pointers to children, NULL if unused */
    uint8  data[BCAST_MAX_DATA_SIZE];                  /* buffer to store data */
    bool   is_value;                                   /* true if value in node */
} BCAST_EigNode_t;

/* Pointer to all nodes at given level in tree. */
/* This is only public to facilitate testing. */
typedef struct {
    BCAST_EigNode_t* nodes[BCAST_MAX_TREE_NODES]; /* Pointers to nodes at level */
    size_t count;                                 /* Number of nodes at level */
} BCAST_EigLevel_t;

/* Data for one instance of broadcast. */
/* This is only public to facilitate testing. */
typedef struct {
    bool   is_config;              /* True if instance of broadcast is configured. */
    size_t num_replicas;           /* Number of replicas <= MAX_REPLICA_COUNT. */
    size_t max_faults;             /* Number of faults to tolerate. */
    size_t src_index;              /* Index of source replica < MAX_REPLICA_COUNT. */
    
    void* p_input;                 /* Pointer to input data to broadcast. */
                                   /* Only used by replica (src_indeX). */
    void* p_output;                /* Pointer where output data will be stored. */
                                   /* Only used by non-source replicas. */
    size_t len;                    /* Size of data to be broadcasted in bytes. */
    bool use_ef;                   /* True to use error-free version of broadcast. */
                                   /* Otherwise use non-equivocation version. */              
                                     
    int vl_ids[MAX_REPLICA_COUNT]; /* VLs used for sending and receiving. */
                                   /* Index REPLICA_INDEX is used for sending. */
                                   /* Else, index k is used to receive from Replica k. */
                                   
    /* Number of values stored in each level of tree. */
    /* The receive count for level k is stored at recv_count[k]. */                 
    size_t recv_count[BCAST_MAX_TREE_HEIGHT + 1];                              

    /* Timeouts in ms for each round of protocol. */
    /* The timeout for round k is stored at timeouts_ms[k]. */
    /* A timeout of 0 means no timeout is used. */                           
    size_t timeouts_ms[BCAST_MAX_TREE_HEIGHT + 1];

    /* Pointers to all nodes at each level of tree. */
    /* Nodes at level k are at levels[k] (levels[0] is unused. */                    
    BCAST_EigLevel_t levels[BCAST_MAX_TREE_HEIGHT + 1];
    
    /* EIG tree used to organize exchanged messages. */
    BCAST_EigNode_t tree[BCAST_MAX_TREE_NODES];
    size_t node_count; /* number of nodes in the tree */

    /*
    ** Data added to accomodate BCAST_ExecuteMulti().
    */

    /* Timestamp when each round times out. */
    struct timespec stop[BCAST_MAX_TREE_HEIGHT + 1];
    size_t round;      /* keeps track of the current round */
    size_t node_index; /* index of next node at level to send */
    
} BCAST_Data_t;

/*
** Public Global Variables
*/

/* Internal data for each broadcast instance. */
/* This is only public to facilitate testing. */
extern BCAST_Data_t BCAST_Data[BCAST_MAX_INSTANCES];

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 BCAST_LibEntry(void);
void  BCAST_LibInit(void);
void  BCAST_LibCleanup(void);

/**
** Clear the contents of the tree.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \returns None
*/
void BCAST_ClearTree(size_t instance);

/**
** Randomize the contents of the tree. Used for testing only.
**
** Random number generator must be seeded by caller.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \returns None
*/
void BCAST_RandomTree(size_t instance);

/**
** Dump the contents on the tree for the given instance.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \return None
*/           
void BCAST_DumpTree(size_t instance); 

/**
** Set the data at a specific node in the tree.
**
** If the provided path is not in the tree, then no action is taken.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \param[in] p_path   Array with path to node in tree.
** \param[in] depth    Specifies the length of the path.
** \param[in] p_val    Pointer to data to add to tree.
**                     The length is determined by BCAST_Config().
**
** \returns True if value was added to an empty node.
** \returns False otherwise.
*/
bool BCAST_SetNode(size_t instance,
                   uint8* p_path,
                   size_t depth,
                   void* p_val);

/**
** Apply resolve rule to the contents of the tree.
**
** Note that the replica must not be the source for the given instance.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \returns None
*/
void BCAST_ResolveTree(size_t instance);

/**
** Serialize node data into provided buffer.
**
** This function appends REPLICA_INDEX to the path and copies the new path
** and provided data to the buffer.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \param[in] p_val    Pointer to value to add to buffer.
** \param[in] p_path   Path of node whose value to add to buffer.
** \param[in] depth    Path length of node whose value to add to buffer.
**                     Set to zero if broadcasting from source.
** \param[out] p_buf   Buffer where new path and data are copied.   
**
** \returns None
*/
void BCAST_SerialNodeData(size_t instance,
                          uint8* p_val,
                          uint8* p_path,
                          size_t depth,
                          uint8* p_buf);
                            
/**
** Return number of bytes BCAST_SerialNodeData() will write to buffer.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \param[in] depth    Path length of node whose value to add to buffer.
**                     Set to zero if broadcasting from source.
**
** \returns Number of bytes to write to buffer.
*/
size_t BCAST_SerialNodeDataSize(size_t instance,
                                size_t depth);
                                         
/**
** Deserialize one node value stored in the given message.
**
** \param[in]  instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                      The instance must have already been configured
**                      with BCAST_Config().
** \param[in]  p_msg    Pointer to start of message to parse.
** \param[in]  msg_len  Length of message being parsed in bytes.
** \param[in]  index    Index of byte in buffer where path is located.
** \param[in]  depth    Length of path in the message.
** \param[out] p_path   Path of node read from message.
** \param[out] p_value  Value of node read from message.
**
** \returns Index in message buffer where next path starts.
** \returns -1 if message is malformed.
*/
int BCAST_DeserialNodeData(size_t instance,
                           uint8* p_msg,
                           size_t msg_len,
                           size_t index,
                           size_t depth,
                           uint8* p_path,
                           uint8* p_value);    

/**
** Parse the provided message and store the values in your tree.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \param[in] p_msg    Pointer to start of message containing values.
** \param[in] msg_len  Length of message being parsed in bytes.
** \param[in] round    Round in which message is being parsed.
** \param[in] index    Index of replica that sent the message.
**
** \returns True if level (round) of the tree becomes completely filled.
** \returns Otherwise False.
*/
bool BCAST_DeserialNodeMsg(size_t instance,
                           uint8* p_msg,
                           size_t msg_len,
                           size_t round,
                           size_t src_index);

/**
** Execute round of protocol by exchanging values between replicas.
**
** The function returns after a timeout is reached, or once all values for
** the round have been received.
**
** Note that the replica must not be the source for the given instance.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \param[in] round    Round of the protocol to execute.
**
** \returns None
*/
void BCAST_ReadMsgRound(size_t instance,
                        size_t round);

/**
** Configure an instance of the Broadcast Protocol.
**
** \param[in]  instance      Instance of broadcast protocol to use.
**                           Must be < BCAST_MAX_INSTANCES.
** \param[in]  num_replicas  Number of replicas in system.
**                           Must be <= MAX_REPLICA_COUNT.
** \param[in]  src_index     Index of replica that is source for broadcast.
**                           Must be < MAX_REPLICA_COUNT.
** \param[in]  p_input       Pointer to location of data to broadcast.
**                           This can be NULL if you are not the source. 
** \param[out] p_output      Pointer where output will be stored.
** \param[in]  len           Length of data to broadcast in bytes.
**                           Must be <= BCAST_MAX_DATA_SIZE.
** \param[in]  use_ef        True to use error-free version of protocol.
**                           Otherwise use non-equivocation version.
** \param[in]  p_timeouts_ms Timeouts for each round in milliseconds.
**                           The first entry is the timeout for round 1. 
**                           A timeout of 0 means no timeout is used.
**                           This can be NULL if you are the source. 
** \param[in]  num_timeouts  Number of timeouts given in \ref p_timeouts_ms.
**                           Must be equal to the number of rounds.
** \return None
*/
void BCAST_Config(size_t instance,
                  size_t num_replicas,
                  size_t src_index,
                  void* p_input,
                  void* p_output,
                  size_t len,
                  bool use_ef,
                  size_t* p_timeouts_ms,
                  size_t num_timeouts);
                  
/**
** Broadcast your input value to all other replicas.
**
** Note that the replica must be the source for the given instance.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \returns None
*/
void BCAST_SendMsg(size_t instance);
                        
/**
** Read the value broadcasted by the source.
**
** This function has the replica continuously read messages, store the values
** it receives in its tree, and forward the values to other replicas. After
** each round times out, or each level of the tree has been populated, it
** resolves the tree, sets the output for the given instance, and returns the
** time taken for each round and to resolve the tree.
**
** Note that the replica must not be the source for the given instance.
**
** \param[in]  instance          Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                               The instance must have already been configured
**                               with BCAST_Config().
** \param[out] p_round_times_ms  Pointer to array with >= (BCAST_MAX_TREE_HEIGHT + 1)
**                               elements to populate with time taken by each round in
**                               ms. The time taken by round k is stored in
**                               p_times_ms[k]. Other entries are unused.
** \param[out] p_resolve_time_ms Set to time taken to resolve the tree in ms.
**
** \returns None
*/
void BCAST_ReadMsg(size_t instance,
                   double* p_round_times_ms,
                   double* p_resolve_time_ms);

/**
** Run multiple instances of broadcast simulatenously.
**
** This is a convenience function that runs multiple instances of broadcast that
** have been configured until (num_instances) of them timeout or finish.
**
** \param[in] num_instances Number of instances of broadcast that must terminate
**                          in order for this function to return.
** \param[in] p_run_these   Pointer to array of size BCAST_MAX_INSTANCES
**                          indicating which instances to run.
** \returns None
*/
void BCAST_ExecuteMulti(size_t num_instances,
                        bool* p_run_these);

/**
** Clear the receive VLs for the given broadcast instance.
**
** \param[in] instance Instance of broadcast protocol < BCAST_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with BCAST_Config().
** \returns None
*/
void BCAST_ClearVLs(size_t instance);

/**
** Execute some functions to test the library.
** \return None
*/              
void BCAST_Test(void);
 
#endif

