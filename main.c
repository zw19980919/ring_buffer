#include <stdio.h>
#include "ring_buffer.h"
#include <pthread.h>
#include<string.h>
ring_buffer_t my_rbuffer;
void threadFunc() {
	
	int len =0;
	unsigned char buffer[100]={0};
	while(1)
	{
		usleep(5000);
		memset(buffer, 0, sizeof(buffer));
		ring_buffer_read(&my_rbuffer, buffer, &len);
	}
	return ;
}
 
int main()
{
	pthread_t tid;
	unsigned char buffer[] ="hellogkdasfkcbksjdfcg ,smbflsvg, bl";
	memset(&tid, 0, sizeof(tid));
	pthread_create(&tid, NULL, threadFunc, NULL);
	ring_buffer_init(&my_rbuffer);
	while(1)
	{   
		usleep(5000);
		ring_buffer_write(&my_rbuffer, buffer, sizeof(buffer));
	}
	return 0;
}
