#include "ring_buffer.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>
//#include <windows.h>
/********************************
 * �ж�����buffer�Ƿ�Ϊ��
 * return 1 is empty
 * return 0 is not empty
 * *****************************/


int ring_buffer_is_empty(ring_buffer_t *ring_buffer)
{
    if (ring_buffer->read_pos == ring_buffer->read_pos)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*********************************
 * ��ʼ�����λ�����
 * ��ʼ����дָ�룬��������С��Ϊ�����������ڴ�
 * �ɹ�������0
 * ʧ�ܣ�����-1
 * ******************************/
int ring_buffer_init(ring_buffer_t *ring_buffer)
{
    if (ring_buffer == NULL)
    {
        return -1;
    }
    ring_buffer->read_pos = 0;
    ring_buffer->write_pos = 0;
    ring_buffer->base = (unsigned char *)malloc(1024);
    printf("the base address is:%x\n",ring_buffer->base);
    ring_buffer->size = 4096;
    memset(ring_buffer->base, 0, ring_buffer->size);
    return 0;
}
/**********************************************************************************
 * �ӻ�������ȡ���ݣ���ȡ�����ǻ�������ָ��ǰ���ĸ���ַ�����������һ������
 * ����һ֡���ݵĳ��ȣ���ȡ���ǴӸõ�ַȡ�����ȴ�����ֽ�
 * ��ͼ��******����գ�$$������ʵ���ݣ�: ����������ָ�������read_pos����ָ��ʵ���������ĵ�һ���ֽ�
 * [����������ʼ��]��������������write_pos����ָ����������ĵ�һ���ֽ�
 * [************:read_pos$$$$$$$$$$$$$$$$$$$$:write_pos***************]
 * ����1����ǰ�������������������֣�һ������read_pos��ring_buffer size֮��
 * ����һ������base����һ����֮�䣬����ͼ
 * [$$$$$$$$$$$$:$$$$$$$$:write_pos*********************:read_pos&&&&&&&&&]
 * ��ʱҪ�������Σ�
 * ����2����ǰ��λ���м�λ�ã�����Ҫ�������Σ�ֻ�踴��һ�Σ�����ͼ
 * [***************:read_pos$$$$$$$$$$:$$$$$$$$$$$$$:write_pos************]
 ***********************************************************************************/
/***********************************************************************************
 * buffer ��Ҫ�����Ļ�����ָ�룬len�Ƿ��ص�ʵ�����ݴ�С��������ͷ�Ĵ�С����ȫ������
 * *********************************************************************************/
int ring_buffer_read(ring_buffer_t *ring_buffer, unsigned char *buffer, int *len)
{
    unsigned int current_frame_size = 0;
    unsigned int falg = ring_buffer_is_empty(ring_buffer);
    /*****�жϻ������Ƿ�Ϊ��*******************/
    if (falg == 1)
    {
        return 0;
    }
    else
    {
        current_frame_size = *(unsigned int *)(ring_buffer->base + ring_buffer->read_pos);
        /************************��������**********************************************/
        if (current_frame_size > (ring_buffer->size - ring_buffer->read_pos))
        {
            memcpy(buffer, (unsigned int *)ring_buffer->base + ring_buffer->read_pos,
                   ring_buffer->size - ring_buffer->read_pos + 1);
            memcpy(buffer + ring_buffer->size - ring_buffer->read_pos + 1, (unsigned int *)ring_buffer->base,
                   current_frame_size - ring_buffer->size + ring_buffer->read_pos -1);
            ring_buffer->read_pos = (ring_buffer->read_pos + current_frame_size) % ring_buffer->size;
            *len = current_frame_size;
        }
        else
        {
            memcpy(buffer, (unsigned int *)ring_buffer->base + ring_buffer->read_pos, current_frame_size);
            ring_buffer->read_pos = (ring_buffer->read_pos + current_frame_size) % ring_buffer->size;
            *len = current_frame_size;
        }
    }
}

/****************************************************************************************
 * ����1��Ҫд�ĳ��Ȳ����Ե���߽�,ֻҪ����һ�Σ�����1Ҳ���������
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 * [$$$$$$$$$$$$$:write_pos**********************��read_pos$$$$$$$$$:$$$$$$$$$$$]
 * ����2��Ҫд�ĺ��ͷ��β�����븴������
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
******************************************************************************************/
/*******************************************************************************
 * parar:bufferҪд�뻷��buffer�Ļ�������len��д�뻺�����ĳ���
 * *****************************************************************************/
int ring_buffer_write(ring_buffer_t *ring_buffer, unsigned char *buffer, int len)
{
    unsigned int current_remaining_space;
    if (buffer == NULL || ring_buffer == NULL)
    {
        return -1;
    }
    printf("enter\n");

    current_remaining_space = (ring_buffer->write_pos >= ring_buffer->read_pos)
                                  ? (ring_buffer->size - ring_buffer->write_pos + ring_buffer->read_pos)
                                  : (ring_buffer->read_pos - ring_buffer->write_pos);
    /***********����buffer�ռ䲻��***********/
    if (current_remaining_space < len)
    {   
        printf("current_remaining_space len:%d\n",current_remaining_space);
        printf("current_remaining_space is low\n");
        return -1;
    }

    /************����buffer�Ѿ�����**********/
    if(((ring_buffer->write_pos + 1) % ring_buffer->size) == ring_buffer->read_pos)
    {   
        printf("current_remaining_space is full\n");
        return -1;
    }
    /************д�����ͷ������������Ĵ�С���ܵĳ���**/
    *(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos) =  len ;
    ring_buffer->write_pos = ring_buffer->write_pos + 4;
    //printf("the len is:%d\n",*(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos));
    
   
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
            printf("ring_buffer->write_pos is:%d\n", ring_buffer->write_pos);               
            printf("buffers is:%s\n", buffer);
            printf("len is:%d\n", len);
            memcpy((void *)((unsigned char*)(ring_buffer->base) + ring_buffer->write_pos), buffer, len);
        }
    }
    else
    {
        memcpy(ring_buffer->base + ring_buffer->write_pos, buffer, len);
    }
     printf("ring buffer context:%s\n",ring_buffer->base + ring_buffer->write_pos);
    /*********************����֮�󣬰�write_pos - 1���Ϳ�������*********/
    if (((ring_buffer->write_pos + len) % ring_buffer->size) == ring_buffer->read_pos)
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + len - 1) % ring_buffer->size;
    }
    else
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + len) % ring_buffer->size;
    }
}


