#include "ring_buffer.h"
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
/**************************��������ȡ��������******************************/
/**********************************************************************
 * �ӻ�������ȡ���ݣ���ȡ�����ǻ�������ָ��ǰ���ĸ���ַ�����������һ������
 * ����һ֡���ݵĳ��ȣ���ȡ���ǴӸõ�ַȡ�����ȴ�����ֽ�
 * ��ͼ��******����գ�$$������ʵ���ݣ�: ����������ָ�������read_pos����ָ��ʵ���������ĵ�һ���ֽ�
 * "[" ����������ʼ��"]"��������������write_pos����ָ����������ĵ�һ���ֽ�
 * [************:read_pos$$$$$$$$$$$$$$$$$$$$:write_pos*******************]
 *
 * ����1����ǰread_pos�������������������֣�һ������read_pos��ring_buffer size֮��
 * ����һ������base����һ����֮�䣬����ͼ,��ʱҪ��������
 * [$$$$$$$$$$$$:$$$$$$$$:write_pos*********************:read_pos&&&&&&&&&]

 * ����2����ǰ��λ���м�λ�ã�����Ҫ�������Σ�ֻ�踴��һ�Σ�����ͼ
 * [***************:read_pos$$$$$$$$$$:$$$$$$$$$$$$$:write_pos************]
 *************************************************************************/

/************************������д���������*********************************/
/*******************************************************************************
 * ����1��Ҫд�ĳ��Ȳ����Ե���߽�,ֻҪ����һ�Σ�����1Ҳ���������
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 *
 * [$$$$$$$$$$$$$:write_pos**********************��read_pos$$$$$$$$$:$$$$$$$$$$$]
 * ����2��Ҫд�ĺ��ͷ��β�����븴������
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 *******************************************************************************/

/******************************
 * ��������ֶ���Ϊ�˷��ʶ���
 * ����������ڷ���û�ж������������
 * real_len��ʵ�ʴ���ĳ��ȣ�
 * fill_len�����ĳ��ȣ�
 * Ĭ�����õĶ��볤����4
 * �������Ķ��볤��Ҳ��4
 * ***************************/
typedef struct
{
    short int align_len;
    short int real_len;
} HEAD_T;

/*****************************
 * size �Ƕ���Ĵ�С������4�ֽڶ���
 * len��Ҫ����ĳ��ȣ�
 * ����ֵΪ��sizeΪ����ĳ���
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
 * �ж�����buffer�Ƿ�Ϊ��
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
 * ��ʼ�����λ�����
 * ��ʼ����дָ�룬��������С��Ϊ�����������ڴ�
 * �ɹ�������0
 * ʧ�ܣ�����-1
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
 * ��̬����ring_buffer�ͳ�ʼ����
 * ���룺Ҫ������ring_buffer�Ĵ�С
 * �����ring_bufferָ��
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
 * buffer ��Ҫ�����Ļ�����ָ�룬len�Ƿ��ص�ʵ�����ݴ�С��������ͷ�Ĵ�С����ȫ������
 * *********************************************************************************/
buffer_state ring_buffer_read(ring_buffer_t *ring_buffer, unsigned char *buffer, int *len)
{
    unsigned int aligned_len = 0, real_len = 0;
    HEAD_T *head_ptr;
    /*****�жϻ������Ƿ�Ϊ��*******************/
    if (ring_buffer_is_empty(ring_buffer) == buffer_empty)
    {
        printf("ring buffer is empty\n");
        return buffer_empty;
    }
    /************�õ�������****************************/
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
        /************************��������**********************************************/
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
 * parar:bufferҪд�뻷��buffer�Ļ�������len��д�뻺�����ĳ���
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
    /************�õ�������****************************/
    ring_buffer->GET_LOCK(ring_buffer->rw_lock);

    current_remaining_space = (ring_buffer->write_pos >= ring_buffer->read_pos)
                                  ? (ring_buffer->size - ring_buffer->write_pos + ring_buffer->read_pos)
                                  : (ring_buffer->read_pos - ring_buffer->write_pos);
    /***********����buffer�ռ䲻��***********/
    if (current_remaining_space < len)
    {
        printf("current_remaining_space len:%d\n", current_remaining_space);
        printf("current_remaining_space is low\n");
        ring_buffer->RELEASE_LOCK(ring_buffer->rw_lock);
        return buffer_full;
    }

    /************����buffer�Ѿ�����**********/
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

    /************д�����ͷ������������Ĵ�С���ܵĳ���,Ȼ��ָ��ring_buffe����������r****/
    head_ptr = (HEAD_T *)(ring_buffer->base + ring_buffer->write_pos);
    aligned_len = Align(4, len);
    real_len = len;
    head_ptr->align_len = aligned_len;
    head_ptr->real_len = len;
    printf("ring_buffer->write_pos is :%d\n", ring_buffer->write_pos);
    ring_buffer->write_pos = (ring_buffer->write_pos + sizeof(HEAD_T)) % ring_buffer->size;
    printf("ring_buffer->write_pos is:%d\n", ring_buffer->write_pos);

    // printf("the len is:%d\n",*(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos));
    /************��Ӧ����1��***************************/
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
    /*********************����֮�󣬰�write_pos - 1���Ϳ�������*********/
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
