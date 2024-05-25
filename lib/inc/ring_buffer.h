#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H
#include <malloc.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
    success = 0,
    buffer_empty = 1,
    buffer_full = 2,
    buffer_ninit = 3,
    ring_invalid = 4,
    system_no_memory = 5,
    parameter_invalid = 6
}buffer_state;

typedef enum
{   
    no_init = 0,
    init_complete = 1
}ring_init_state;

struct ring_buffer
{   
    volatile int read_pos;
    volatile int write_pos;
    volatile int size;
    ring_init_state flag_init;
    volatile int type ;
    unsigned char *base;
    void *rw_lock;
};
typedef struct ring_buffer ring_buffer_t;

buffer_state ring_buffer_init(ring_buffer_t *ring_buffer);

ring_buffer_t* ring_buffer_declare_init(int buffer_size);

buffer_state ring_buffer_read( ring_buffer_t *ring_buffer, unsigned char* buffer, int *len);

buffer_state ring_buffer_write(ring_buffer_t *ring_buffer, unsigned char *buffer, int len);

buffer_state ring_buffer_destory(ring_buffer_t *ring_buffer);

buffer_state ring_buffer_is_empty(ring_buffer_t *ring_buffer);



#ifdef __cplusplus
}
#endif

#endif