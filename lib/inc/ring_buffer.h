#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

struct ring_buffer
{
    volatile int read_pos;
    volatile int write_pos;
    volatile int size;
    unsigned char *base;
    void *rw_lock;
};
typedef  struct ring_buffer ring_buffer_t;

int ring_buffer_init(ring_buffer_t *ring_buffer);

int ring_buffer_read( ring_buffer_t *ring_buffer, unsigned char* buffer, int *len);

int ring_buffer_write(ring_buffer_t *ring_buffer, unsigned char *buffer, int len);

int ring_buffer_is_empty(ring_buffer_t *ring_buffer);


#ifdef __cplusplus
}
#endif

#endif