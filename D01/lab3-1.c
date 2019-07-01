#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include<pthread.h>

#define NLOOP 10000
#define NORMAL_BUFF_MAX 8000
#define URGENT_BUFF_MAX 2000

int nc;
int nnormal, nurgent;
bool arr[NLOOP]={0};

typedef struct Buffer{
	long long * buffer;
	sem_t *full, *empty;
	int in, out, size;
} Buffer;

Buffer *normal, *urgent;

long long current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
	return milliseconds;
}
void send(Buffer *b, long long ms, int is_normal){
	sem_wait(b->empty);
	b->buffer[b->in] = ms;
	if(is_normal == 0)
		b->in %= URGENT_BUFF_MAX;
	b->in %= NORMAL_BUFF_MAX;
	b->size++;
	sem_post(b->full);
}
long long receive(Buffer *b1, Buffer *b2){
	long long ms;
	if(sem_trywait(b1->full) == 0){
		ms = b1->buffer[b1->out];
		b1->out %= URGENT_BUFF_MAX;
		b1->size--; nurgent++;
		printf("CONSUMER: retrieved ms: %lld from URGENT buffer\n", ms);
		//sem_post(b1->empty);
	} else{
		if(sem_trywait(b2->full) == 0){
			ms = b2->buffer[b2->out];
			b2->out %= NORMAL_BUFF_MAX;
			b2->size--; nnormal++;
			printf("CONSUMER: retrieved ms: %lld from NORMAL buffer\n", ms);
			//sem_post(b2->empty);
		}
		
		
	}
}
static void * producerThread(void * argp){

	struct timespec ts;
	int i, t, r;
	
	for(i=0; i < NLOOP; i++){
		/*t = ((rand() % 10 - 1 + 1) + 1);
		ts.tv_nsec = t * 1000000;
		nanosleep(&ts, NULL);*/
		long long ms = current_timestamp();
		sleep(1);
		
		/* Intializes random number generator */
   		srand(time(NULL));
		r = (rand() % 10000);
		//check if r was generated before
		if(!arr[r]) {
			printf("r = %d\n",r);
			if(r<=8000){
				//normal buffer			
				printf("PRODUCER: putting %lld in buffer NORMAL\n",ms);
				send(normal, ms, 1);
			} else {
				printf("PRODUCER: putting %lld in buffer URGENT\n",ms);
				send(normal, ms, 0);
			}
		} else{
				 i--;
		}
		//printf("r = %d\n",r);
		arr[r]=1;
	}

}
static void * consumerThread(void * argc){
	int i;
	for(i=0;i < NLOOP; i++){
		/*struct timespec ts;
		ts.tv_nsec = 10 * 1000000;
		nanosleep(&ts, NULL);*/
		sleep(2);
		long long val;
		val = receive(urgent, normal);
	}
}

void buff_init(){
	normal = (Buffer *)malloc(sizeof(Buffer));
	normal->buffer = (long long *)malloc(NORMAL_BUFF_MAX * sizeof(long long));
	normal->in = normal->out = normal->size = 0;
	
	normal->full = (sem_t *)malloc(sizeof(sem_t));
	sem_init(normal->full, 0, 0);
	normal->empty = (sem_t *)malloc(sizeof(sem_t));
	sem_init(normal->empty, 0, NORMAL_BUFF_MAX);
	
	urgent = (Buffer *)malloc(sizeof(Buffer));
	urgent->buffer = (long long *)malloc(URGENT_BUFF_MAX * sizeof(long long));
	urgent->in = urgent->out = urgent->size = 0;
	
	urgent->full = (sem_t *)malloc(sizeof(sem_t));
	sem_init(urgent->full, 0, 0);
	urgent->empty = (sem_t *)malloc(sizeof(sem_t));
	sem_init(urgent->empty, 0, URGENT_BUFF_MAX);
	
	nnormal = nurgent = 0;
}

int main(int argc, char *argv[]) {

	buff_init();
	pthread_t th_p,th_c;
	pthread_create(&th_p, NULL, producerThread, (void *)NULL);
	pthread_create(&th_c, NULL, consumerThread, (void *)NULL);
	
	
	
	pthread_join(th_p, NULL);
	pthread_join(th_c, NULL);
	
	printf("Normal served: %d Urgent served: %d\n", nnormal, nurgent);
}
