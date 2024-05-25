#include <stdio.h>
#include "ring_buffer.h"
#include <pthread.h>
#include<string.h>
ring_buffer_t my_rbuffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void threadFunc() {
	
	int len =0;
	unsigned char buffer[100]={0};
	while(1)
	{
		usleep(5000);

		pthread_mutex_lock(&mutex);

		memset(buffer, 0, sizeof(buffer));
		ring_buffer_read(&my_rbuffer, buffer, &len);
		//printf("receive len: %d\n",len);
		//printf("recrive context:%s\n",buffer);
		//fflush(stdout);
		pthread_mutex_unlock(&mutex);
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
		pthread_mutex_lock(&mutex);
		ring_buffer_write(&my_rbuffer, buffer, sizeof(buffer));
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}
