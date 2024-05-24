#include <stdio.h>
#include "ring_buffer.h"
#include <pthread.h>
#include<string.h>
ring_buffer_t my_rbuffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void threadFunc() {
	
	int len =0;
	unsigned char buffer[20]={0};
	while(1)
	{
		Sleep(2000);

		pthread_mutex_lock(&mutex);

		memset(buffer, 0, sizeof(buffer));
		//ring_buffer_read(&my_rbuffer, buffer, &len);
		//printf("receive len: %d\n",len);
		//printf("recrive context:%s\n",buffer);
		fflush(stdout);
		pthread_mutex_unlock(&mutex);
	}
	return ;
}
 
int main()
{
	pthread_t tid;
	unsigned char buffer[] ="hello_world\n";
	char *p = malloc(100);
	memset(&tid, 0, sizeof(tid));
	pthread_create(&tid, NULL, threadFunc, NULL);
	ring_buffer_init(&my_rbuffer);
	while(1)
	{   
		Sleep(1000);
		//printf("while\n");
		pthread_mutex_lock(&mutex);
		memcpy(p, buffer, sizeof(buffer));
		printf("buffer 1 :%s\r\n",buffer);
		
		//printf("p:%s\n",p);
		ring_buffer_write(&my_rbuffer, buffer, sizeof(buffer));
		pthread_mutex_unlock(&mutex);
		fflush(stdout);
	}
	return 0;
}
