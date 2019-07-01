#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include<pthread.h>

#define NLOOP 10000

int nc; // total number of served consumers

/**************STRUCTURES**********************/
typedef struct semaphore_t {
	pthread_mutex_t lock;
	pthread_cond_t iszero;
	int count;
}semaphore_t;

semaphore_t *semP, *semC;

typedef struct CommonBuffer{
	int * buffer;
	int count;
} CommonBuffer;

CommonBuffer *commonBuffer;
/**********************************************/
void sema_init();
void sema_wait(semaphore_t *sem);
void sema_post(semaphore_t *sem);

/*****************************SEMAPHORE FUNCTIONS**************************************************/
/**************************************************************************************************/
void sema_init(){
	semP = (semaphore_t *)malloc(sizeof(semaphore_t));
	pthread_mutex_init(&semP->lock, NULL);
	pthread_cond_init(&semP->iszero, NULL);
	semP->count = 0;
	
	semC = (semaphore_t *)malloc(sizeof(semaphore_t));
	pthread_mutex_init(&semC->lock, NULL);
	pthread_cond_init(&semC->iszero, NULL);
	semC->count = 0;
}

void sema_wait(semaphore_t *sem){
	pthread_mutex_lock(&sem->lock);
	while(sem->count==0)
		pthread_cond_wait(&sem->iszero, &sem->lock);
	sem->count--;
	pthread_mutex_unlock(&sem->lock);

}

void sema_post(semaphore_t *sem){
	pthread_mutex_lock(&sem->lock);
		if(sem->count==0)
			pthread_cond_signal(&sem->iszero);
		sem->count++;	
	pthread_mutex_unlock(&sem->lock);
}
/****************************************************************************************************/

void buff_init(){
	commonBuffer = (CommonBuffer *)malloc(sizeof(CommonBuffer));
	commonBuffer->buffer = (int *)malloc(NLOOP * sizeof(int));	
	commonBuffer->count = 0;
}


/***********************************PROD-CONS THREADS*************************************************/
static void * producerThread(void * argp){
	struct timespec ts;
	int i;
	for(i=0; i < NLOOP; i++){
		printf("PRODUCER: putting %d into shared buffer\n",i);
		commonBuffer->buffer[commonBuffer->count] = i;
		commonBuffer->count++;
		nc++;
		sema_post(semC);
		sema_wait(semP);
	}
}

static void * consumerThread(void * argc){
	while(nc < NLOOP){
		sema_wait(semC);
		int val;
		val = commonBuffer->buffer[commonBuffer->count-1];
		printf("CONSUMER: retrieved number: %d\n", val);
		sema_post(semP);	
	}
}
/****************************************************************************************************/


int main(int argc, char *argv[]) {

	sema_init();
	buff_init();
	nc = 0;	//used as loop variable for consumer thread
	printf("buffer and semaphore initialized\n");
	
	pthread_t th_p,th_c;
	pthread_create(&th_p, NULL, producerThread, (void *)NULL);
	pthread_create(&th_c, NULL, consumerThread, (void *)NULL);
	
	pthread_join(th_p, NULL);
	pthread_join(th_c, NULL);
	printf("Total served: %d\n",nc);
	
}
