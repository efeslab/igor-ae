/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include "bcast_lib.h"
#include "vote_lib.h"
#include "afdx_api.h"
#include "afdx_lib_vldefs.h"

/*
** Function Declarations
*/

/**
** Generate an EIG tree used to store messages.
**
** \param[in] instance Specifies instance whose tree to initialize.
** \param[in] p_path   Array with path to node to add to tree.
** \param[in] depth    Specifies the length of the path.
**
** \returns None
*/
void BCAST_GenTree(size_t instance, uint8* p_path, size_t depth);

/**
** Return true if p_child is a child of p_parent, else false.
**
** \param[in] p_parent Node to test as parent.
** \param[in] p_child  Node to test as child.
**
** \returns true if child, else false
*/
bool BCAST_IsChild(BCAST_EigNode_t* p_parent,
                   BCAST_EigNode_t* p_child);
                   
/**
** Return the last replica index in the provided node's path.
**
** \param[in] p_node Node whose path to parse.
** \returns   Last replica index in path.
*/
size_t BCAST_PathEnd(BCAST_EigNode_t* p_node);
                                        
/**
** Initialize all the child and helper pointers in the tree.
**
** \param[in] instance Specifies instance whose tree to thread.
** \returns None
*/
void BCAST_ThreadTree(size_t instance);

/**
** Check whether the local replica is in the given path.
**
** \param[in] p_path   Array with path to check against.
** \param[in] depth    Specifies length of the path.
**
** \returns True if local replica is not in path.
** \returns False if replica is in path.
*/
bool BCAST_NotInPath(uint8* p_path, size_t depth);

/** 
** Calculate difference between two timestamps in milliseconds.
**
** \param[in] start Starting timestamp.
** \param[in] stop  Ending timestamp.
**
** \return Difference between timestamps.
*/
double BCAST_TimeDiffMs(struct timespec start,
                        struct timespec stop);

/** 
** Calculate a timestamp offset (offset_ms) from the given timestamp.
**
** \param[in] start     Starting timestamp to add to.
** \param[in] offset_ms Time in milliseconds to add to timestamp.
**
** \return The new timestamp.
*/
struct timespec BCAST_TimeOffset(struct timespec start,
                                 size_t offset_ms);
                                 
/**
** Execute a portion of one round of a broadcast instance.
**
** \param[in] instance Specifies instance of broadcast protocol.
** \param[in] round    Specifies round to continue executing.
**
** \returns True once level is populated (or timeout), else False.
*/
bool BCAST_ReadDataRound(size_t instance,
                         size_t round);
                         
/**
** Execute a portion of reading a broadcasted value from the source.
**
** \param[in] instance Specifies instance of broadcast protocol.
** \returns   True once tree is resolved, else False.
*/
bool BCAST_ReadData(size_t instance);                         
                                                        
/*
** Constants
*/

#define LIB_NAME "BCAST_LIB"

/* Get the number of elements in an array. */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/* Internal data for each broadcast instance. */
BCAST_Data_t BCAST_Data[BCAST_MAX_INSTANCES];

/* Special values to compare to (arbitrary). */
static uint8 BCAST_DEF[BCAST_MAX_DATA_SIZE];
static uint8 BCAST_ERR[BCAST_MAX_DATA_SIZE];
static uint8 BCAST_REP[BCAST_MAX_DATA_SIZE];

/*
** Internal Function Definitions
*/

/* Generate an EIG tree used to store messages. */
void BCAST_GenTree(size_t instance, uint8* p_path, size_t depth)
{
    BCAST_Data_t* p_data = NULL;             /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL;          /* pointer to node in tree */
    size_t num_replicas = 0;                 /* number of replicas */
    size_t max_faults = 0;                   /* number of faults to tolerate */
    uint8 child_path[BCAST_MAX_TREE_HEIGHT]; /* path of child */
    bool in_path = false;                    /* true if replica in path */
    int replica_idx = 0;                     /* replica iterator */
    int i = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(p_path != NULL);
    assert(depth <= BCAST_MAX_TREE_HEIGHT);
    
    p_data = &(BCAST_Data[instance]);
    p_node = &(p_data->tree[p_data->node_count]);
    num_replicas = p_data->num_replicas;
    max_faults = p_data->max_faults;

    /* Add node to the tree. */  
    memcpy(p_node->path, p_path, sizeof(child_path));    
    p_node->depth = depth;
    p_data->node_count++;    
    
    /* If depth limit reached, stop. */
    if (depth == (max_faults + 1)) {
        return;
    }
    
    /* If path ends with me, stop. */
    /* Used to prune branches of the tree. */
    if (p_path[depth - 1] == REPLICA_INDEX) {
        return;
    }    

    /* Generate children that are not in path. */
    for (replica_idx = 0; replica_idx < num_replicas; replica_idx++) {
        in_path = false;
        for (i = 0; i < depth; i++) {
            if (replica_idx == p_path[i]) {
                in_path = true; /* already in path */
                break;
            }
        }
        if (in_path == false) { /* not in path */
            memcpy(child_path, p_path, sizeof(child_path));
            child_path[depth] = replica_idx;
            BCAST_GenTree(instance, child_path, depth + 1);
        }
    }

    return;
}

/* Return true if p_child is a child of p_parent, else false. */
bool BCAST_IsChild(BCAST_EigNode_t* p_parent,
                   BCAST_EigNode_t* p_child)
{
    int i = 0;
    
    assert(p_parent != NULL);
    assert(p_child != NULL);

    if (p_child->depth != (p_parent->depth + 1)) {
        return false;
    }

    for (i = 0; i < p_parent->depth; i++) {
        if (p_parent->path[i] != p_child->path[i]) {
            return false;
        }
    }

    return true;
}

/* Return the last replica index in the provided node's path. */
size_t BCAST_PathEnd(BCAST_EigNode_t* p_node)
{
    assert(p_node != NULL);
    return (p_node->path[p_node->depth - 1]);
}

/* Initialize all the child and helper pointers in the tree. */
void BCAST_ThreadTree(size_t instance)
{
    BCAST_Data_t* p_data = NULL;      /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL;   /* pointer to node in tree */
    BCAST_EigNode_t* p_child = NULL;  /* pointer to child node */
    BCAST_EigLevel_t* p_level = NULL; /* pointer to level in tree */   
    int node_count = 0;
    int i = 0;
    int j = 0;   

    assert(instance < BCAST_MAX_INSTANCES);
        
    p_data = &(BCAST_Data[instance]);
    node_count = p_data->node_count;

    /* Set all child pointers to NULL. */        
    for (i = 0; i < node_count; i++) {
        p_node = &(p_data->tree[i]);    
        for (j = 0; j < NELEMS(p_node->children); j++) {
            p_node->children[j] = NULL;
        }
    }

    /* Initialize all child pointers in tree. */
    for (i = 0; i < node_count; i++) {
        p_node = &(p_data->tree[i]);
        for (j = i; j < node_count; j++) {
            if (j != i ) {
                p_child = &(p_data->tree[j]);
                if (BCAST_IsChild(p_node, p_child)) {
                    p_node->children[BCAST_PathEnd(p_child)] = p_child;
                }
            }
        }
    }

    /* Set pointers to nodes at each level. */
    for (i = 1; i <= (BCAST_MAX_TREE_HEIGHT + 1); i++) {
        p_level = &(p_data->levels[i]);
        for (j = 0; j < node_count; j++) {
            p_node = &(p_data->tree[j]);
            if (p_node->depth == i) {
                p_level->nodes[p_level->count] = p_node;
                p_level->count++;
            }
        }
    }

    return;
}

/* Check whether the local replica is in the given path. */
bool BCAST_NotInPath(uint8* p_path, size_t depth)
{
    int i = 0;
    
    assert(p_path != NULL);
    assert(depth <= BCAST_MAX_TREE_HEIGHT);
    
    for (i = 0; i < depth; i++) {
        if (p_path[i] == REPLICA_INDEX) {
            return false;
        }
    }
    
    return true;
}

/* Calculate difference between two timestamps in milliseconds. */
double BCAST_TimeDiffMs(struct timespec start,
                        struct timespec stop)
{
    double time;
    time = (stop.tv_sec - start.tv_sec) * 1e9; /* time in seconds as ns */
    time = (time + (stop.tv_nsec - start.tv_nsec)) * 1e-6; /* add in ns, convert to ms */
    return time;
}    

/* Calculate a timestamp offset (offset_ms) from the given timestamp. */
struct timespec BCAST_TimeOffset(struct timespec start,
                                 size_t offset_ms)
{
    struct timespec stop = start;
 
    /* Add seconds. */
    while (offset_ms >= 1000) {
        stop.tv_sec++;   
        offset_ms -= 1000;
    }
 
    /* Add remaining milliseconds. */
    stop.tv_nsec += (offset_ms * 1000000);
    if (stop.tv_nsec >= 1000000000) {
        stop.tv_nsec -= 1000000000;
        stop.tv_sec++;
    }

    return stop;
}
             
/* Execute a portion of one round of a broadcast instance. */
bool BCAST_ReadDataRound(size_t instance,
                         size_t round)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;         /* number of replicas */
    int vl_id = 0;               /* VL to read or send through */
    size_t timeout_ms = 0;       /* timeout for function */
    size_t expect_count = 0;     /* expected values to store at level (round) */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */

    BCAST_EigLevel_t* p_level = NULL;        /* pointer to level in tree */
    BCAST_EigNode_t* p_node = NULL;          /* pointer to node at level */
    size_t serial_size = 0;                  /* size of value in send message */
    uint8 msg_send[AFDX_MAX_MSG_SIZE] = {0}; /* message to send */
    size_t send = 0;                         /* index into message to send */

    struct timespec curr; /* current timestamp */
        
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
   
    p_data       = &(BCAST_Data[instance]);
    num_replicas = p_data->num_replicas;
    timeout_ms   = p_data->timeouts_ms[round];
    expect_count = p_data->levels[round].count;

    /* Check whether I already have all values */
    /* I expect to receive for level. */
    if (p_data->recv_count[round] >= expect_count) {
        return true; /* I am done this round */
    }

    /* Add values to level (round), as well as later levels. */
    for (i = 0; i < num_replicas; i++) {
        if (i != REPLICA_INDEX) { /* not me */
            vl_id = BCAST_Data[instance].vl_ids[i];        
            ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
            if (ret == AFDX_SUCCESS) {
                if (BCAST_DeserialNodeMsg(instance, msg_recv, msg_len, round, i)) {
                    return true; /* level (round) of tree is filled */
                }
            }
        }
    }

    /* Broadcast values from level (round - 1). */
    if (round > 1) { /* receivers don't relay in round 1 */

        p_level     = &(p_data->levels[round - 1]);
        serial_size = BCAST_SerialNodeDataSize(instance, round - 1);        
        vl_id       = p_data->vl_ids[REPLICA_INDEX];
        msg_send[0] = round; /* new depth */

        if (p_data->node_index < p_level->count) { /* still data to send */
            for (i = 0; i < BCAST_SEND_MAX_QUEUE; i++) {            
                send = 1;                    
                while (p_data->node_index < p_level->count) { /* populate the message */
                
                    p_node = p_level->nodes[p_data->node_index];
                    p_data->node_index++;
 
                    if (BCAST_NotInPath(p_node->path, p_node->depth)) { /* I'm not in the path, send it */             
            
                        BCAST_SerialNodeData(instance, p_node->data, p_node->path,
                                             p_node->depth, &(msg_send[send]));
                        assert((round - 1) == p_node->depth);
                        send += serial_size;  

                        /* If no more room for next value send the message. */
                        if ((sizeof(msg_send) - send) < serial_size) {
                            AFDX_SendMsg(msg_send, send, vl_id);
                            if (BCAST_DeserialNodeMsg(instance, msg_send, send, round, REPLICA_INDEX)) {
                                return true; /* level (round) of tree is filled */
                            }
                            send = 1; /* set to prevent double sending */
                            break;    /* move to next message */
                        }
                    }    
                }
                
                /* If there are still values in the message */
                /* that need to be sent, send it. */
                if (send > 1) {
                    AFDX_SendMsg(msg_send, send, vl_id);
                    if (BCAST_DeserialNodeMsg(instance, msg_send, send, round, REPLICA_INDEX)) {
                        return true; /* level (round) of tree is filled */
                    }
                }
            }
        }
    }

    /* Check for timeout. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    if ((timeout_ms != 0) &&
         ((curr.tv_sec >= p_data->stop[round].tv_sec) && 
            (curr.tv_nsec >= p_data->stop[round].tv_nsec))) {
        printf("%s: Instance %zu timeout occured\n", LIB_NAME, instance);             
        return true;
    }

    return false;
}

/* Execute a portion of reading a broadcasted value from the source. */
bool BCAST_ReadData(size_t instance)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */
    size_t len = 0;              /* length of data to agree on */
    size_t max_faults = 0;       /* number of faults to tolerate */ 

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
        
    p_data     = &(BCAST_Data[instance]);
    len        = p_data->len;
    max_faults = p_data->max_faults;

    /* Execute a portion of one round. */
    if (BCAST_ReadDataRound(instance, p_data->round) == true) { /* round is over */
        p_data->round++;
        p_data->node_index = 0;
        if (p_data->round == (max_faults + 2)) { /* resolve the tree */
            BCAST_ResolveTree(instance);    
            memcpy(p_data->p_output, p_data->tree[0].data, len);
            BCAST_ClearTree(instance);
            return true;
        }
    }   
        
    return false;
}             
                                                 
/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 BCAST_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
void BCAST_LibInit(void)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */
    int index_base = 0;          /* index into AFDX table */
    int index = 0;               /* index into AFDX table */    
    int vl_id = 0;               /* virtual link ID */     
    int i = 0;
    int j = 0;

    memset(BCAST_Data, 0x0, sizeof(BCAST_Data));

    for (i = 0; i < NELEMS(BCAST_Data); i++) {
        p_data = &(BCAST_Data[i]);
        index_base = BCAST_VL_OFFSET + (MAX_REPLICA_COUNT * i);
        for (j = 0; j < MAX_REPLICA_COUNT; j++) {
            index = index_base + j;
            vl_id = AFDX_LIB_VlTable[index].vl_id;
            p_data->vl_ids[j] = vl_id;
        }
    }   

    /* Initialize special values. */
    memset(BCAST_DEF, BCAST_DEF_VALUE, sizeof(BCAST_DEF));
    memset(BCAST_ERR, BCAST_ERR_VALUE, sizeof(BCAST_ERR));
    memset(BCAST_REP, BCAST_REP_VALUE, sizeof(BCAST_REP));
    
    printf("%s: Lib Initialized\n", LIB_NAME);
    
    return;
}

/* Cleanup the library. */
void BCAST_LibCleanup(void) {
    printf("%s: Cleaned up library\n", LIB_NAME);
    return;
}

/* Clear the contents of the tree. */
void BCAST_ClearTree(size_t instance)
{
    BCAST_Data_t* p_data = NULL;    /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL; /* pointer to node in tree */
    size_t node_count = 0;          /* number of nodes in tree */
    size_t len = 0;                 /* length of data in nodes */
    bool use_ef;                    /* true to use error-free */       
    int i = 0;
    
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
        
    p_data = &(BCAST_Data[instance]);
    node_count = p_data->node_count;
    len = p_data->len;
    use_ef = p_data->use_ef;
    
    memset(p_data->recv_count, 0x0, sizeof(p_data->recv_count));

    /* Initial value depends on protocol. */    
    for (i = 0; i < node_count; i++) {
        p_node = &(p_data->tree[i]);
        p_node->is_value = false;
        if (use_ef) {
            memcpy(p_node->data, BCAST_DEF, len);
        } else {
            memcpy(p_node->data, BCAST_ERR, len);
        }
    }

    return;
}

/* Randomize the contents of the tree. Used for testing only. */
void BCAST_RandomTree(size_t instance)
{
    BCAST_Data_t* p_data = NULL;    /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL; /* pointer to node in tree */
    size_t node_count = 0;          /* number of nodes in tree */
    size_t len = 0;                 /* length of data in nodes */
    int i = 0;
    int j = 0;
    
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
        
    p_data = &(BCAST_Data[instance]);
    node_count = p_data->node_count;
    len = p_data->len;

    for (i = 0; i < node_count; i++) {
        p_node = &(p_data->tree[i]);
        p_node->is_value = true;        
        for (j = 0; j < len; j++) {
            p_node->data[j] = rand();
        }
    }

    return;
}

/* Dump the contents on the tree for the given instance. */           
void BCAST_DumpTree(size_t instance)
{
    BCAST_Data_t* p_data = NULL;     /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL;  /* pointer to node in tree */
    char path_str[30];               /* path to node */
    int node_count = 0;
    int i = 0;
    int j = 0;   

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
        
    p_data = &(BCAST_Data[instance]);
    node_count = p_data->node_count;

    for (i = 0; i < node_count; i++) {
        p_node = &(p_data->tree[i]);
        /* Build the path string. */
        for (j = 0; j < p_node->depth; j++) {
            /* Works since max REPLICA_INDEX is 9. */
            path_str[j] = p_node->path[j] + '0';
        }
        path_str[j] = '\0';
    
        printf("path = %s, depth = %zu, is_value = %d, data (1st byte) = 0x%02X\n",
               path_str, p_node->depth, p_node->is_value, p_node->data[0]);
    }
    printf("\n");

    return;
}

/* Set the data at a specific node in the tree. */
bool BCAST_SetNode(size_t instance,
                   uint8* p_path,
                   size_t depth,
                   void* p_val)
{
    BCAST_Data_t* p_data = NULL;    /* pointer to struct */
    BCAST_EigNode_t* p_node = NULL; /* pointer to node in tree */
    BCAST_EigNode_t* p_tmp = NULL;  /* pointer to node in tree */    
    size_t len = 0;                 /* length of data in tree */
    size_t index = 0;               /* replica index in path */
    int i = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);    
    assert(p_path != NULL);
    assert(depth <= BCAST_MAX_TREE_HEIGHT);
    assert(depth > 0);
    assert(p_val != NULL);
    
    p_data = &(BCAST_Data[instance]);   
    len = p_data->len;
    
    /* Start at root of tree. */
    p_node = &(p_data->tree[0]);    

    /* Verfy that start of path matches root. */
    if (p_node->path[0] != p_path[0]) {
        return false;
    }

    /* Follow provided path through tree. */
    for (i = 0; i < depth - 1; i++) {
        assert(p_node->path[i] == p_path[i]);
        index = p_path[i + 1];
        if (index >= MAX_REPLICA_COUNT) return false; /* index out of range */
        p_tmp = p_node->children[index];
        if (p_tmp == NULL) return false; /* path not in tree */
        p_node = p_tmp;
    }
    
    assert(p_node->path[i] == p_path[i]);
    
    memcpy(p_node->data, p_val, len);

    if (p_node->is_value == false) {
        p_node->is_value = true;
        return true;
    }
    
    return false;
}

/* Apply resolve rule to the contents of the tree. */
void BCAST_ResolveTree(size_t instance)
{
    BCAST_Data_t* p_data = NULL;      /* pointer to struct */
    BCAST_EigLevel_t* p_level = NULL; /* pointer to level in tree */       
    BCAST_EigNode_t* p_node = NULL;   /* pointer to node in tree */
    BCAST_EigNode_t* p_child = NULL;  /* pointer to child node in tree */
    size_t max_faults = 0;            /* max faults to tolerate */
    size_t len = 0;                   /* length of data in nodes */
    bool use_ef;                      /* true to use error-free */   
    int i = 0;
    int j = 0;
    int k = 0;
    
    void* pp_vals[MAX_REPLICA_COUNT]; /* array of ptrs to values to vote */ 
    size_t val_count = 0;             /* number of values to vote */
    void* p_out = NULL;               /* pointer to result of vote */
    int ret = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
        
    p_data = &(BCAST_Data[instance]);
    max_faults = p_data->max_faults;
    len = p_data->len;
    use_ef = p_data->use_ef;

    for (i = max_faults; i >= 1; i--) { /* levels f to 1 */
        p_level = &(p_data->levels[i]);
        for (j = 0; j < p_level->count; j++) { /* all nodes at level */
            p_node = p_level->nodes[j];
            
            if (p_node->path[p_node->depth - 1] != REPLICA_INDEX) {

                /* Populate list of children to vote. */
                /* If not error-free don't include children with Error values. */
                val_count = 0;
                for (k = 0; k < NELEMS(p_node->children); k++) { /* all children of node */
                    p_child = p_node->children[k];
                    if (p_child != NULL) { /* child is real */
                        if ((use_ef) || (memcmp(p_child->data, BCAST_ERR, len) != 0)) {                        
                            pp_vals[val_count] = p_child->data;        
                            val_count++;
                        }
                    }
                }

                /* Vote the children's values. */
                ret = VOTE_FindMaj(pp_vals, val_count, len, &p_out);

                /* Set parent to majority; if none, set to Default. */
                /* If not error-free and majority is Report, set parent to Error. */
                if (ret == VOTE_FAILED) { /* no majority */
                    memcpy(p_node->data, BCAST_DEF, len);
                } else { /* yes majority */
                    if ((!use_ef) && (memcmp(p_out, BCAST_REP, len) == 0)) {
                        memcpy(p_node->data, BCAST_ERR, len);
                    } else {
                        memcpy(p_node->data, p_out, len);
                    }
                }            
            }
        }
    }

    return;
}

/* Serialize node data into provided buffer. */
void BCAST_SerialNodeData(size_t instance,
                          uint8* p_val,
                          uint8* p_path,
                          size_t depth,
                          uint8* p_buf)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */   
    size_t len = 0;              /* length of data in nodes */
    bool use_ef;                 /* true if protocol is error-free */
    size_t bytes = 0;
    
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(p_val != NULL);
    assert(p_path != NULL);
    assert(depth < BCAST_MAX_TREE_HEIGHT);    
    assert(p_buf != NULL);

    p_data = &(BCAST_Data[instance]);
    len = p_data->len;    
    use_ef = p_data->use_ef;   
    
    memcpy(&(p_buf[0]), p_path, depth); /* old path */
    bytes += depth;
    p_buf[bytes] = REPLICA_INDEX;       /* add self to path */
    bytes++;

    /* If not error-free, forward error values as reports. */
    if ((use_ef) || (memcmp(p_val, BCAST_ERR, len) != 0)) {    
        memcpy(&(p_buf[bytes]), p_val, len);
    } else {
        memcpy(&(p_buf[bytes]), BCAST_REP, len);    
    }
    bytes += len;

    return;
}

/* Return number of bytes BCAST_SerialNodeData() will write to buffer. */
size_t BCAST_SerialNodeDataSize(size_t instance,
                                size_t depth)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */   
    size_t len = 0;              /* length of data in nodes */
    
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(depth < BCAST_MAX_TREE_HEIGHT);

    p_data = &(BCAST_Data[instance]);
    len = p_data->len;

    return (depth + 1 + len);
}

/* Deserialize one node value stored in the given message. */
int BCAST_DeserialNodeData(size_t instance,
                           uint8* p_msg,
                           size_t msg_len,
                           size_t index,
                           size_t depth,
                           uint8* p_path,
                           uint8* p_value)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */    
    size_t len = 0;              /* length of data to agree on */ 
    size_t max_faults = 0;       /* max faults to tolerate */    
    size_t index_new = index;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
    assert(p_msg != NULL);
    assert(p_path != NULL);
    assert(p_value != NULL);        

    p_data = &(BCAST_Data[instance]);
    len = p_data->len;
    max_faults = p_data->max_faults;

    assert((depth > 0) && (depth <= (max_faults + 1))); 
    
    if ((msg_len - index_new) < depth) {
        return -1; /* no room for path */ 
    }

    memcpy(p_path, &(p_msg[index_new]), depth);
    index_new += depth;

    if ((msg_len - index_new) < len) {
        return -1; /* no room for value */
    }

    memcpy(p_value, &(p_msg[index_new]), len);                      
    index_new += len;

    return index_new;
}

/* Parse the provided message and store the values in your tree. */
bool BCAST_DeserialNodeMsg(size_t instance,
                           uint8* p_msg,
                           size_t msg_len,
                           size_t round,
                           size_t src_index)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */    
    size_t expect_count = 0;     /* expected values to store at level (round) */
    
    size_t recv = 0;                        /* index into received message */
    size_t tmp_index = 0;                   /* temp index in received message */
    uint8 depth = 0;                        /* depth from received message */
    uint8 path[BCAST_MAX_TREE_HEIGHT];      /* path from received message */
    uint8 value[BCAST_MAX_DATA_SIZE] = {0}; /* value from message */

    //int k = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
    assert(p_msg != NULL);
    assert(msg_len <= AFDX_MAX_MSG_SIZE);
    assert(src_index < MAX_REPLICA_COUNT);

    p_data = &(BCAST_Data[instance]);
    expect_count = p_data->levels[round].count;

    if (msg_len < 1) {
        return false; /* no room for depth */
    }    

    depth = p_msg[0];
    recv = 1; /* point to first path */   
    
    if (depth < round) {
        return false; /* depth corresponds to past round */
    } 

    /* Parse all values in message. */
    while (1) {
        tmp_index = BCAST_DeserialNodeData(instance, p_msg, msg_len,
                                           recv, depth, path, value);
        if (tmp_index == -1){
             break; /* message malformed, or done parsing */
        }
             
        if (path[depth - 1] != src_index) {   
            break; /* sender does not match path */
        }
        
        recv = tmp_index;
                                                            
        if ((src_index == REPLICA_INDEX) || (BCAST_NotInPath(path, depth))) {                               
            if (BCAST_SetNode(instance, path, depth, value) == true) { /* value is new */ 
                                                         
                p_data->recv_count[depth]++;
                
                //printf("(instance %zu) Level %zu: Got %zu / %zu\n", instance,
                //       round,  p_data->recv_count[depth], expect_count);
                       
                //printf("path: ");
                //for (k = 0; k < depth; k++) {
                //    printf("%d ", path[k]);
                //}
                //printf("\n");
                                                            
                if (p_data->recv_count[round] >= expect_count) {
                    return true; /* got all values at level (round) */
                }
            }
        }                           
    }

    return false;
}

/* Execute round of protocol by exchanging values between replicas. */
void BCAST_ReadMsgRound(size_t instance,
                        size_t round)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;         /* number of replicas */
    int vl_id = 0;               /* VL to read or send through */
    size_t timeout_ms = 0;       /* timeout for function */
    size_t expect_count = 0;     /* expected values to store at level (round) */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */

    BCAST_EigLevel_t* p_level = NULL;        /* pointer to level in tree */
    BCAST_EigNode_t* p_node = NULL;          /* pointer to node at level */
    size_t serial_size = 0;                  /* size of value in send message */
    size_t node_index = 0;                   /* index of node at level */
    uint8 msg_send[AFDX_MAX_MSG_SIZE] = {0}; /* message to send */
    size_t send = 0;                         /* index into message to send */

    struct timespec curr;                    /* current timestamp */
    struct timespec stop;                    /* time to return from function */
        
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
    
    p_data = &(BCAST_Data[instance]);
    num_replicas = p_data->num_replicas;
    timeout_ms = p_data->timeouts_ms[round];
    expect_count = p_data->levels[round].count;

    /* Calculate timeout (only used if timeout is nonzero). */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    stop = BCAST_TimeOffset(curr, timeout_ms);

    /* Check whether I already have all values */
    /* I expect to receive for level. */
    if (p_data->recv_count[round] >= expect_count) {
        return;
    }

    while (1) {

        /* Add values to level (round), as well as later levels. */
        for (i = 0; i < num_replicas; i++) {
            if (i != REPLICA_INDEX) { /* not me */
                vl_id = BCAST_Data[instance].vl_ids[i];        
                ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
                if (ret == AFDX_SUCCESS) {
                    if (BCAST_DeserialNodeMsg(instance, msg_recv, msg_len, round, i)) {
                        return; /* level (round) of tree is filled */
                    }
                }
            }
        }

        /* Broadcast values from level (round - 1). */
        if (round > 1) { /* receivers don't relay in round 1 */

            p_level     = &(p_data->levels[round - 1]);
            serial_size = BCAST_SerialNodeDataSize(instance, round - 1);        
            vl_id       = p_data->vl_ids[REPLICA_INDEX];
            msg_send[0] = round; /* new depth */
           
            if (node_index < p_level->count) {
                for (i = 0; i < BCAST_SEND_MAX_QUEUE; i++) {            
                    send = 1;                    
                    while (node_index < p_level->count) { /* populate the message */
                    
                        p_node = p_level->nodes[node_index];
                        node_index++;
     
                        if (BCAST_NotInPath(p_node->path, p_node->depth)) { /* I'm not in the path, send it */             
                
                            BCAST_SerialNodeData(instance, p_node->data, p_node->path,
                                                 p_node->depth, &(msg_send[send]));
                            assert((round - 1) == p_node->depth);
                            send += serial_size;  

                            /* If no more room for next value send the message. */
                            if ((sizeof(msg_send) - send) < serial_size) {
                                AFDX_SendMsg(msg_send, send, vl_id);
                                if (BCAST_DeserialNodeMsg(instance, msg_send, send, round, REPLICA_INDEX)) {
                                    return; /* level (round) of tree is filled */
                                }
                                send = 1; /* set to prevent double sending */
                                break;    /* move to next message */
                            }
                        }    
                    }
                    
                    /* If there are still values in the message */
                    /* that need to be sent, send it. */
                    if (send > 1) {
                        AFDX_SendMsg(msg_send, send, vl_id);
                        if (BCAST_DeserialNodeMsg(instance, msg_send, send, round, REPLICA_INDEX)) {
                            return; /* level (round) of tree is filled */
                        }
                    }
                }
            }
        }
        
        /* Check for timeout. */
        clock_gettime(CLOCK_MONOTONIC, &curr);
        if ((timeout_ms != 0) &&
             ((curr.tv_sec >= stop.tv_sec) && (curr.tv_nsec > stop.tv_nsec))) {
            printf("%s: Instance %zu timeout occured\n", LIB_NAME, instance);             
            return;   
        }

        usleep(BCAST_RECV_POLL_US);
    }

    return;
}

/* Configure an instance of the Broadcast Protocol. */
void BCAST_Config(size_t instance,
                  size_t num_replicas,
                  size_t src_index,
                  void* p_input,
                  void* p_output,
                  size_t len,
                  bool use_ef,
                  size_t* p_timeouts_ms,
                  size_t num_timeouts)
{
    BCAST_Data_t* p_data = NULL;       /* pointer to struct */
    uint8 path[BCAST_MAX_TREE_HEIGHT]; /* path to root */        
    size_t depth = 1;                  /* depth of root */    
    size_t max_faults;                 /* max faults to tolerate */
    int i = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(num_replicas <= MAX_REPLICA_COUNT);
    assert(src_index < MAX_REPLICA_COUNT);    
    assert(len <= BCAST_MAX_DATA_SIZE);

    if (src_index == REPLICA_INDEX) { /* you are source */
        assert(p_input != NULL);
    }
    
    if (src_index != REPLICA_INDEX) { /* you are not source */
        assert(p_output != NULL);
        assert(p_timeouts_ms != NULL);
    }
    
    /* Value of f depends on protocol. */    
    max_faults = (use_ef) ? floor((num_replicas - 1.0)/3.0) : \
                            floor((num_replicas - 1.0)/2.0);

    if (src_index != REPLICA_INDEX) { /* you are not source */
        assert(num_timeouts = (max_faults + 1));
    }

    p_data = &(BCAST_Data[instance]);

    p_data->is_config    = true;   
    p_data->num_replicas = num_replicas;   
    p_data->max_faults   = max_faults;
    p_data->src_index    = src_index;   
    p_data->p_input      = p_input;
    p_data->p_output     = p_output;    
    p_data->len          = len;
    p_data->use_ef       = use_ef;    
    
    memset(p_data->recv_count, 0x0, sizeof(p_data->recv_count));
    
    for (i = 0; i < num_timeouts; i++) {
        p_data->timeouts_ms[i + 1] = p_timeouts_ms[i];
    }

    /* Initialize the tree. */
    p_data->node_count = 0;    
    path[0] = src_index;
    BCAST_GenTree(instance, path, depth);

    /* Thread pointers for tree. */
    BCAST_ThreadTree(instance);    

    /* Clear the tree. */
    BCAST_ClearTree(instance);

    return;
}

/* Broadcast your input value to all other replicas. */
void BCAST_SendMsg(size_t instance)
{
    BCAST_Data_t* p_data = NULL;       /* pointer to struct */   
    uint8 path[BCAST_MAX_TREE_HEIGHT]; /* path to data (unused) */
    int vl_id;                         /* VL to send through */    

    uint8 buffer[AFDX_MAX_MSG_SIZE] = {0}; /* message to send */
    size_t bytes = 0;                      /* copied to buffer */
               
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index == REPLICA_INDEX);

    p_data = &(BCAST_Data[instance]);
    vl_id  = p_data->vl_ids[REPLICA_INDEX];    

    buffer[0] = 1; /* depth where node will be stored */
    bytes++;
    BCAST_SerialNodeData(instance, p_data->p_input, path, 0, &(buffer[bytes]));
    bytes += BCAST_SerialNodeDataSize(instance, 0);
    
    AFDX_SendMsg(buffer, bytes, vl_id);    

    return;
}

/* Read the value broadcasted by the source. */
void BCAST_ReadMsg(size_t instance,
                   double* p_round_times_ms,
                   double* p_resolve_time_ms)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */
    size_t len = 0;              /* length of data to agree on */
    size_t max_faults = 0;       /* number of faults to tolerate */ 
    struct timespec start;       /* start timestamp */
    struct timespec stop;        /* stop timestamp */
    size_t round = 0;            /* current round */
    int i = 0;

    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);
    assert(p_round_times_ms != NULL);
    assert(p_resolve_time_ms != NULL);
        
    p_data = &(BCAST_Data[instance]);
    len = p_data->len;
    max_faults = p_data->max_faults;

    //printf("Tree before reading:\n");
    //BCAST_DumpTree(instance);     
    
    /* Execute each round of protocol. */
    for (i = 0; i < (max_faults + 1); i++) {
        round = i + 1;
        clock_gettime(CLOCK_MONOTONIC, &start);
        BCAST_ReadMsgRound(instance, round);
        clock_gettime(CLOCK_MONOTONIC, &stop);
        *(&(p_round_times_ms[round])) = BCAST_TimeDiffMs(start, stop);
        //printf("Done round %d -----------------\n", round);
    }
    
    //printf("Tree after message exchange:\n");
    //BCAST_DumpTree(instance);     

    /* Resolve the tree. */
    clock_gettime(CLOCK_MONOTONIC, &start);
    BCAST_ResolveTree(instance);
    clock_gettime(CLOCK_MONOTONIC, &stop);    
    *p_resolve_time_ms = BCAST_TimeDiffMs(start, stop);

    //printf("Tree after resolving:\n");
    //BCAST_DumpTree(instance);     
    
    /* Copy data from root of tree to output. */
    memcpy(p_data->p_output, p_data->tree[0].data, len);

    BCAST_ClearTree(instance);
    
    return;
}

/* Run multiple instances of broadcast simulatenously. */
void BCAST_ExecuteMulti(size_t num_instances,
                        bool* p_run_these)
{
    BCAST_Data_t* p_data = NULL;              /* pointer to struct */ 
    bool is_done[BCAST_MAX_INSTANCES] = {0};  /* true if instance is done */
    size_t num_done = 0;                      /* number of done instances */
    
    int i = 0;
    int k = 0;

    struct timespec curr; /* current timestamp */
    struct timespec prev; /* previous timestamp */
    
    assert(num_instances <= BCAST_MAX_INSTANCES);
    assert(p_run_these != NULL);

    /* Get the current timestamp. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    prev = curr;

    /* Reset global data before running protocol. */
    for (i = 0; i < BCAST_MAX_INSTANCES; i++) {
        p_data = &(BCAST_Data[i]);     
        if ((p_data->is_config) && (p_run_these[i] == true)) {
            p_data->round = 1; /* start in round 1 */
            p_data->node_index = 0;
            /* Set the timeout times. */
            for (k = 1; k < (p_data->max_faults + 2); k++) {
                p_data->stop[k] = BCAST_TimeOffset(prev, p_data->timeouts_ms[k]);
                prev = p_data->stop[k];
            }
        }
    }
    
    /* Loop through instances and, if source, send initial data. */
    for (i = 0; i < BCAST_MAX_INSTANCES; i++) {
        p_data = &(BCAST_Data[i]);
        if ((p_data->is_config) && (p_run_these[i] == true) && (p_data->src_index == REPLICA_INDEX)) {
            BCAST_SendMsg(i);
            num_instances--;
        }
    }
    
    /* Read from each instance until all terminate. */
    while (1) {
        /* Loop through all instances. */
        for (i = 0; i < BCAST_MAX_INSTANCES; i++) {
            p_data = &(BCAST_Data[i]);
            if ((p_data->is_config) && (p_run_these[i] == true) && (p_data->src_index != REPLICA_INDEX) && (is_done[i] == false)) {
                if (BCAST_ReadData(i) == true) { /* instance is done */
                    is_done[i] = true;
                    num_done++;
                    if (num_done >= num_instances) {
                        return; /* all instances done */
                    }
                }
            }
        }
        usleep(BCAST_RECV_POLL_MULTI_US);
    }
    
    return;
}
                        
/* Clear the receive VLs for the given broadcast instance. */
void BCAST_ClearVLs(size_t instance)
{
    BCAST_Data_t* p_data = NULL; /* pointer to struct */
    int vl_id = 0;               /* VL to read from */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */
    
    assert(instance < BCAST_MAX_INSTANCES);
    assert(BCAST_Data[instance].is_config);
    assert(BCAST_Data[instance].src_index != REPLICA_INDEX);

    p_data = &(BCAST_Data[instance]);
    
    for (i = 0; i < NELEMS(p_data->vl_ids); i++) {
        if (i != REPLICA_INDEX) { /* not me */
            vl_id = p_data->vl_ids[i];
            do {
                ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
            } while (ret == AFDX_SUCCESS);
        }
    }

    return;
}

/* Execute some functions to test the library. */              
void BCAST_Test(void)
{
    size_t instance = 0;
    size_t num_replicas = 7;
    size_t src_index = 1;
    size_t len = 100;
    bool use_ef = true;
    size_t timeouts_ms[MAX_REPLICA_COUNT] = {0};
    size_t max_faults = 2;

    uint8 input[BCAST_MAX_DATA_SIZE];
    uint8 output[BCAST_MAX_DATA_SIZE];
  
    BCAST_LibInit();
        
    BCAST_Config(instance, num_replicas, src_index, input, output,
                 len, use_ef, timeouts_ms, max_faults + 1);

    BCAST_ClearTree(instance);
    
    BCAST_DumpTree(instance);    

    BCAST_RandomTree(instance);
    
    BCAST_DumpTree(instance);    

    BCAST_ClearTree(instance);
        
    uint8 newval[BCAST_MAX_DATA_SIZE] = {0xFF};
    uint8 path[3] = {1, 6, 4};
    size_t depth = 3;

    BCAST_SetNode(instance, path, depth, newval);   
    path[2] = 0;
    BCAST_SetNode(instance, path, depth, newval);    
    path[2] = 5;
    BCAST_SetNode(instance, path, depth, newval);   

    path[1] = 2;
    path[2] = 4;
    BCAST_SetNode(instance, path, depth, newval);  
    path[2] = 5;
    BCAST_SetNode(instance, path, depth, newval);  
    path[2] = 6;    
    BCAST_SetNode(instance, path, depth, newval);  

    path[1] = 4;
    path[2] = 2;
    BCAST_SetNode(instance, path, depth, newval);  
    path[2] = 3;
    BCAST_SetNode(instance, path, depth, newval);  
    path[2] = 5;    
    BCAST_SetNode(instance, path, depth, newval);  

    path[1] = 0;
    depth = 2;
    BCAST_SetNode(instance, path, depth, newval);
           
    BCAST_DumpTree(instance);    

    BCAST_ResolveTree(instance);

    BCAST_DumpTree(instance);
                       
    return;
}

