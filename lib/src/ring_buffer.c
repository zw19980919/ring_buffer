#include "ring_buffer.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>
//#include <windows.h>
/********************************
 * 判读环形buffer是否为空
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
 * 初始化环形缓冲区
 * 初始化读写指针，缓冲区大小，为缓冲区申请内存
 * 成功：返回0
 * 失败：返回-1
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
 * 从缓冲区读取数据，读取规则是缓冲区读指针前面四个地址代表的内容是一个长度
 * 代表一帧数据的长度，读取就是从该地址取出长度代表的字节
 * 下图中******代表空，$$代表真实内容，: 代表桢与桢分隔，其中read_pos总是指向实际数据区的第一个字节
 * [代表缓冲区开始，]代表缓冲区结束，write_pos总是指向空数据区的第一个字节
 * [************:read_pos$$$$$$$$$$$$$$$$$$$$:write_pos***************]
 * 类型1：当前代表的数据桢包括两部分，一部分在read_pos和ring_buffer size之间
 * 另外一部分在base和下一个桢之间，如下图
 * [$$$$$$$$$$$$:$$$$$$$$:write_pos*********************:read_pos&&&&&&&&&]
 * 此时要复制两次，
 * 类型2：当前桢位于中间位置，不需要复制两次，只需复制一次，如下图
 * [***************:read_pos$$$$$$$$$$:$$$$$$$$$$$$$:write_pos************]
 ***********************************************************************************/
/***********************************************************************************
 * buffer 是要读出的缓冲区指针，len是返回的实际数据大小，不包括头的大小，完全是数据
 * *********************************************************************************/
int ring_buffer_read(ring_buffer_t *ring_buffer, unsigned char *buffer, int *len)
{
    unsigned int current_frame_size = 0;
    unsigned int falg = ring_buffer_is_empty(ring_buffer);
    /*****判断缓冲区是否为空*******************/
    if (falg == 1)
    {
        return 0;
    }
    else
    {
        current_frame_size = *(unsigned int *)(ring_buffer->base + ring_buffer->read_pos);
        /************************复制两次**********************************************/
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
 * 类型1：要写的长度不足以到达边界,只要复制一次，类型1也有两种情况
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
 * [$$$$$$$$$$$$$:write_pos**********************：read_pos$$$$$$$$$:$$$$$$$$$$$]
 * 类型2：要写的横跨头和尾，必须复制两次
 * [**********:read_pos$$$$$$$:$$$$$$$$$$$$$$$$:write_pos***********************]
******************************************************************************************/
/*******************************************************************************
 * parar:buffer要写入环形buffer的缓冲区，len是写入缓冲区的长度
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
    /***********环形buffer空间不足***********/
    if (current_remaining_space < len)
    {   
        printf("current_remaining_space len:%d\n",current_remaining_space);
        printf("current_remaining_space is low\n");
        return -1;
    }

    /************环形buffer已经满了**********/
    if(((ring_buffer->write_pos + 1) % ring_buffer->size) == ring_buffer->read_pos)
    {   
        printf("current_remaining_space is full\n");
        return -1;
    }
    /************写入包括头部和数据区域的大小，总的长度**/
    *(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos) =  len ;
    ring_buffer->write_pos = ring_buffer->write_pos + 4;
    //printf("the len is:%d\n",*(unsigned int*)((unsigned int*)(ring_buffer->base) + ring_buffer->write_pos));
    
   
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
    /*********************满了之后，把write_pos - 1，和空相区别*********/
    if (((ring_buffer->write_pos + len) % ring_buffer->size) == ring_buffer->read_pos)
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + len - 1) % ring_buffer->size;
    }
    else
    {
        ring_buffer->write_pos = (ring_buffer->write_pos + len) % ring_buffer->size;
    }
}


