#include "ring_buffer.h"
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
/**************************缓冲区读取规则描述******************************/
/**********************************************************************
 * 从缓冲区读取数据，读取规则是缓冲区读指针前面四个地址代表的内容是一个长度
 * 代表一帧数据的长度，读取就是从该地址取出长度代表的字节
 * 下图中******代表空，$$代表真实内容，: 代表桢与桢分隔，其中read_pos总是指向实际数据区的第一个字节
 * "[" 代表缓冲区开始，"]"代表缓冲区结束，write_pos总是指向空数据区的第一个字节
 * [************:read_pos$$$$$$$$$$$$$$$$$$$$:write_pos*******************]
 *
 * 类型1：当前read_pos代表的数据桢包括两部分，一部分在read_pos和ring_buffer size之间
 * 另外一部分在base和下一个桢之间，如下图,此时要复制两次
 * [$$$$$$$$$$$$:$$$$$$$$:write_pos*********************:read_pos&&&&&&&&&]

 * 类型2：当前桢位于中间位置，不需要复制两次，只需复制一次，如下图
 * [***************:read_pos$$$$$$$$$$:$$$$$$$$$$$$$:write_pos************]
 *************************************************************************/

/************************缓冲区写入规则描述*********************************/
/*******************************************************************************
 * 类型1：要写的长度不足以到达边界,只要复制一次，类型1也有两种情况
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 *
 * [$$$$$$$$$$$$$:write_pos**********************：read_pos$$$$$$$$$:$$$$$$$$$$$]
 * 类型2：要写的横跨头和尾，必须复制两次
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 *******************************************************************************/

/******************************
 * 增加这个字段是为了访问对齐
 * 否则可能由于访问没有对其而产生错误
 * real_len是实际传入的长度，
 * fill_len是填充的长度，
 * 默认设置的对齐长度是4
 * 缓冲区的对齐长度也是4
 * ***************************/
typedef struct
{
    short int align_len;
    short int real_len;
} HEAD_T;

/*****************************
 * size 是对齐的大小，比如4字节对齐
 * len是要对齐的长度，
 * 返回值为以size为对齐的长度
 * **************************/
static int Align(int size, unsigned int len)
{
    int align_len;
    if ((len % size) != 0)
    {
        align_len = (len / size + 1) * size;
        return align_len;
    }
    else
    {
        return len;
    }
}

static void get_lock(void *lock_ptr)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)lock_ptr;
    pthread_mutex_lock(mutex);
}

static void release_lock(void *lock_ptr)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)lock_ptr;
    pthread_mutex_unlock(mutex);
}
/********************************
 * 判读环形buffer是否为空
 * return 1 is empty
 * return 0 is not empty
 * *****************************/

buffer_state ring_buffer_is_empty(ring_buffer_t *ring_buffer)
{
    if (ring_buffer->write_pos == ring_buffer->read_pos)
    {
        printf("ring buffer is empry\n");
        return buffer_empty;
    }
    else
    {
        return success;
    }
}

/*********************************
 * 初始化环形缓冲区
 * 初始化读写指针，缓冲区大小，为缓冲区申请内存
 * 成功：返回0
 * 失败：返回-1
 * ******************************/
buffer_state ring_buffer_init(ring_buffer_t *ring_buffer)
{
    if (ring_buffer == NULL)
    {
        return ring_invalid;
    }
    ring_buffer->read_pos = 0;
    ring_buffer->write_pos = 0;
    ring_buffer->base = (unsigned char *)malloc(1024);
    ring_buffer->flag_init = init_complete;
    ring_buffer->type = 0;
    ring_buffer->size = 1024;
    ring_buffer->rw_lock = (void *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t *)ring_buffer->rw_lock, NULL);
    ring_buffer->GET_LOCK = get_lock;
    ring_buffer->RELEASE_LOCK = release_lock;
    memset(ring_buffer->base, 0, ring_buffer->size);
    return success;
}

/********************************
 * 动态申请ring_buffer和初始化，
 * 输入：要创建的ring_buffer的大小
 * 输出：ring_buffer指针
 *********************************/
ring_buffer_t *ring_buffer_declare_init(int buffer_size)
{
    ring_buffer_t *ring_buffer = (ring_buffer_t *)malloc(sizeof(ring_buffer_t));
    if (ring_buffer == NULL)
    {
        printf("alloc ring_buffer fali\n");
        return (ring_buffer_t *)(-1);
    }
    ring_buffer->read_pos = 0;
    ring_buffer->write_pos = 0;
    ring_buffer->base = (unsigned char *)malloc(Align(4, buffer_size));
    ring_buffer->flag_init = init_complete;
    ring_buffer->type = 2;
    ring_buffer->rw_lock = (void *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t *)ring_buffer->rw_lock, NULL);
    ring_buffer->GET_LOCK = get_lock;
    ring_buffer->RELEASE_LOCK = release_lock;
    ring_buffer->size = Align(4, buffer_size);
    memset(ring_buffer->base, 0, ring_buffer->size);
    return success;
}

/***********************************************************************************
 * buffer 是要读出的缓冲区指针，len是返回的实际数据大小，不包括头的大小，完全是数据
 * *********************************************************************************/
buffer_state ring_buffer_read(ring_buffer_t *ring_buffer, unsigned char *buffer, int *len)
{
    unsigned int aligned_len = 0, real_len = 0;
    HEAD_T *head_ptr;
    /*****判断缓冲区是否为空*******************/
    if (ring_buffer_is_empty(ring_buffer) == buffer_empty)
    {
        printf("ring buffer is empty\n");
        return buffer_empty;
    }
    /************得到互斥锁****************************/
    ring_buffer->GET_LOCK(ring_buffer->rw_lock);

    if (ring_buffer->flag_init == init_complete)
    {
        head_ptr = (HEAD_T *)(ring_buffer->base + ring_buffer->read_pos);
        real_len = head_ptr->real_len;
        aligned_len = head_ptr->align_len;
        printf("ring_buffer->read_pos is:%d\n", ring_buffer->read_pos);
        ring_buffer->read_pos = ((ring_buffer->read_pos + sizeof(HEAD_T)) % ring_buffer->size);
        printf("ring_buffer read len is:%d\n", real_len);
        printf("ring_buffer read context is:%s\n", (unsigned char *)(ring_buffer->read_pos + ring_buffer->base));
        /************************复制两次**********************************************/
        if (real_len > (ring_buffer->size - ring_buffer->read_pos))
        {
            memcpy(buffer, ring_buffer->base + ring_buffer->read_pos, ring_buffer->size - ring_buffer->read_pos + 1);
            memcpy(buffer + ring_buffer->size - ring_buffer->read_pos + 1, (unsigned int *)ring_buffer->base,
                   real_len - ring_buffer->size + ring_buffer->read_pos - 1);
            ring_buffer->read_pos = ((ring_buffer->read_pos + aligned_len) % ring_buffer->size);
            *len = real_len;
            ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
            return success;
        }
        else
        {
            memcpy(buffer, ring_buffer->base + ring_buffer->read_pos, real_len);
            ring_buffer->read_pos = ((ring_buffer->read_pos + aligned_len) % ring_buffer->size);
            *len = real_len;
            ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
            return success;
        }
    }
    else
    {
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return buffer_ninit;
    }
}

/*******************************************************************************
 * parar:buffer要写入环形buffer的缓冲区，len是写入缓冲区的长度
 * *****************************************************************************/
buffer_state ring_buffer_write(ring_buffer_t *ring_buffer, unsigned char *buffer, int len)
{
    unsigned int current_remaining_space;
    unsigned int aligned_len, real_len, fill_len;
    HEAD_T *head_ptr;

    if (buffer == NULL || ring_buffer == NULL || len == 0)
    {
        printf("ERROR::invail pararm\n");
        return parameter_invalid;
    }
    /************得到互斥锁****************************/
    ring_buffer->GET_LOCK(ring_buffer->rw_lock);

    current_remaining_space = (ring_buffer->write_pos >= ring_buffer->read_pos)
                                  ? (ring_buffer->size - ring_buffer->write_pos + ring_buffer->read_pos)
                                  : (ring_buffer->read_pos - ring_buffer->write_pos);
    /***********环形buffer空间不足***********/
    if (current_remaining_space < len)
    {
        printf("current_remaining_space len:%d\n", current_remaining_space);
        printf("current_remaining_space is low\n");
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return buffer_full;
    }

    /************环形buffer已经满了**********/
    if (((ring_buffer->write_pos + 1) % ring_buffer->size) == ring_buffer->read_pos)
    {
        printf("current_remaining_space is full\n");
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return buffer_full;
    }

    if (ring_buffer->flag_init == no_init)
    {
        printf("ring buffer is no init\n");
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return buffer_ninit;
    }

    /************写入包括头部和数据区域的大小，总的长度,然后指向ring_buffe的数据区域r****/
    head_ptr = (HEAD_T *)(ring_buffer->base + ring_buffer->write_pos);
    aligned_len = Align(4, len);
    real_len = len;
    head_ptr->align_len = aligned_len;
    head_ptr->real_len = len;
    printf("ring_buffer->write_pos is :%d\n", ring_buffer->write_pos);
    ring_buffer->write_pos = (ring_buffer->write_pos + sizeof(HEAD_T)) % ring_buffer->size;
    printf("ring_buffer->write_pos is:%d\n", ring_buffer->write_pos);

    // printf("the len is:%d\n",*(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos));
    /************对应类型1的***************************/
    if (ring_buffer->write_pos > ring_buffer->read_pos)
    {
        if (len > (ring_buffer->size - ring_buffer->write_pos + 1))
        {
            memcpy(ring_buffer->base + ring_buffer->write_pos, buffer, ring_buffer->size - ring_buffer->write_pos + 1);
            memcpy(ring_buffer->base, buffer + ring_buffer->size - ring_buffer->write_pos + 1,
                   len - ring_buffer->size + ring_buffer->write_pos - 1);
        }
        else
        {
            printf("buffers is:%s\n", buffer);
            printf("len is:%d\n", len);
            memcpy((void *)((unsigned char *)(ring_buffer->base) + ring_buffer->write_pos), buffer, len);
        }
    }
    else
    {
        memcpy(ring_buffer->base + ring_buffer->write_pos, buffer, len);
    }
    printf("ring buffer context:%s\n", ring_buffer->base + ring_buffer->write_pos);
    /*********************满了之后，把write_pos - 1，和空相区别*********/
    if (((ring_buffer->write_pos + aligned_len) % ring_buffer->size) == ring_buffer->read_pos)
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + aligned_len - 1) % ring_buffer->size;
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return success;
    }
    else
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + aligned_len) % ring_buffer->size;
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return success;
    }
}

buffer_state ring_buffer_destory(ring_buffer_t *ring_buffer)
{
    if (ring_buffer->flag_init == no_init)
    {
        printf("ring_buffer also destory\n");
        return ring_invalid;
    }

    if (ring_buffer->type == 2)
    {
        free(ring_buffer->base);
        memset(ring_buffer, 0, sizeof(ring_buffer));
        free(ring_buffer);
        return success;
    }
    else
    {
        free(ring_buffer->base);
        memset(ring_buffer, 0, sizeof(ring_buffer));
        return success;
    }
}
