#include <stdio.h>
#include "ring_buffer.h"
#include <pthread.h>
#include<string.h>
ring_buffer_t my_rbuffer;
void threadFunc() {
	
	int len =0;
	unsigned char buffer[20]={0};
	while(1)
	{
		Sleep(2000);
		memset(buffer, 0, sizeof(buffer));
		//ring_buffer_read(&my_rbuffer, buffer, &len);
		printf("receive len: %d\n",len);
		printf("recrive context:%s\n",buffer);
	}
	return ;
}
 
int main()
{
	pthread_t tid;
	unsigned char buffer[]="hello world\n";
	memset(&tid, 0, sizeof(tid));
	//pthread_create(&tid, NULL, threadFunc, NULL);
	ring_buffer_init(&my_rbuffer);
	while(1)
	{   
		Sleep(1000);
		ring_buffer_write(&my_rbuffer, buffer, sizeof(buffer));
	}
	return 0;
}
