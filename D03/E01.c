/*a C program using Pthreads to sort the content of a binary file including a sequence of integer
numbers. a threaded quicksort program where the recursive calls to quicksort are
replaced by threads activations, i.e. sorting is done, in parallel, in different regions of the file.
If the difference between the right and left indexes is less than a value size, given as an
argument of the command line, sorting is performed by the standard quicksort algorithm.
*/

#include<stdio.h>
#include<unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

typedef struct{
	int *vec;
	int left;
	int right;
	int tid;
	pthread_mutex_t mutex;
	pthread_mutex_t mutex2;
	
}Param;

/*GLOBAL VARIABLES*/
Param *p;
int v_size;

int nthreads;
int nrunningthreads = 0;
char *paddr;         
sem_t *barrier1, *barrier2, *barrier3;
pthread_mutex_t mutex;

/*FUNCTION PROTOTYPES*/
void printArray(int *v, int start, int end);
void swap(int i, int j);
void quicksort (int *v, int left, int right);
void merge(int *v, int l, int m, int r);
/*******************FUNCTIONS************************/
void printArray(int *v, int start, int end)
{
    int i;
    for (i=start; i < end; i++)
        printf("%d ", v[i+1]);
    printf("\n");
}

void quicksort (int *v, int left, int right) {
	printf("==========================STANDARD QUICKSORT==========================================\n");
	int i, j, x, tmp;
	if (left >= right)
		return;
	x = v[left];
	i = left -1;
	j = right + 1;
	while (i < j) {
		while (v[--j] > x);
		while (v[++i] < x);
		if (i < j){
				int tmp; tmp = v[i];				
				v[i] = v[j];
				v[j] = tmp;
		}
	}
	quicksort (v, left, j);
	quicksort (v, j + 1, right);
}

void merge(int *v, int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;
    int L[n1], R[n2]; /*  temp arrays */
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = v[l + i];
    for (j = 0; j < n2; j++)
        R[j] = v[m + 1+ j];
    /* Merge the temp arrays back into v[l..r]*/
    i = 0; /* Initial index of first subarray */
    j = 0; /* Initial index of second subarray */
    k = l; /* Initial index of merged subarray*/
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            v[k] = L[i];
            i++;
        }
        else {
            v[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        v[k] = L[i];
        i++;
        k++;
    } 
    while (j < n2) {
        v[k] = R[j];
        j++;
        k++;
    }
}

/*******************THREAD FUNCTION************************/
static void *QS(void *pa){
	//pthread_detach (pthread_self ());
	int r = random() % 5;
	sleep(r);
	
	Param *p = (Param *)pa;
	printf("\tthread %d : left = %d right = %d\n",p->tid,p->left,p->right);
	int i, j, x;	
	//int len  = p->right - p->left;
	printf("==========================THREADED QUICKSORT==========================================\n");
	if (p->left >= p->right) {
		printf("thread %d exiting\n",p->tid);
		pthread_exit((void *)pthread_self());
		if(nrunningthreads == 1)
			printf("NO MORE RUNNING THREADS************************************++\n");
	}
	i = p->left-1;
	j = p->right+1;
	x = p->vec[p->left];
	pthread_mutex_lock(&p->mutex);
		nrunningthreads++;
	pthread_mutex_unlock(&p->mutex);
	/*sort*/
	while (i < j) {
		while (p->vec[--j] > x);
		while (p->vec[++i] < x);
			if (i < j){
				printf("%d swapping %d and %d \n",p->tid,p->vec[i],p->vec[j]);
				pthread_mutex_lock(&p->mutex);
					int tmp,k;
					tmp = p->vec[i];
					p->vec[i] = p->vec[j];
					p->vec[j] = tmp;
					for (k = 0; k < nrunningthreads; k++)
						sem_post(barrier1);
				pthread_mutex_unlock(&p->mutex);
			}	
	}
	printf("THREAD %d waiting on barrier1\n",p->tid);
	sem_wait(barrier1);

	/*CREATE TWO THREADS(INSTEAD OF RECURSIVE CALL)*/
	pthread_mutex_lock(&p->mutex);	
		pthread_t th1,th2;
		Param *newP = (Param *)malloc(sizeof(Param));
		newP->vec = (int *)malloc(sizeof(int));
		newP->vec = p->vec;
		newP->left = p->left;
		newP->right = j;
		newP->tid = (p->tid)+1;
		pthread_create(&th1,NULL,QS,newP);

		Param *newP1 = (Param *)malloc(sizeof(Param));
		newP1->vec = (int *)malloc(sizeof(int));
		newP1->vec = p->vec;
		newP1->left = j+1;
		newP1->right = p->right;
		newP1->tid = p->tid+2;
		pthread_create(&th2,NULL,QS,newP1);
		
		pthread_join(th1,NULL);
		pthread_join(th2,NULL);
		
		nrunningthreads--;
		printf("running threads = %d\n",nrunningthreads);
		//printf("after decrement :runningthreads = %d\n ",nrunningthreads);
		int l;
		for(int l=0;l<nrunningthreads;l++)
			sem_post(barrier2);	
	pthread_mutex_unlock(&p->mutex);
	
	printf("THREAD %d waiting on barrier2\n",p->tid);
	sem_wait(barrier2);
	if(nrunningthreads == 0){
		/*merge the sorted sub-vectors*/
		printf("I am the last thread. I need to do merging all by myself :( \n");
		merge(p->vec, 1, (v_size/nthreads), v_size+1);
		
	}
	//printArray(p->vec,p->left, p->right);	
	
	
	//printf("====================================================================\n");
}
/*******************MAIN FUNCTION************************/
int main(int argc,char* argv[]){
	/*CHECK ARGUMENT*/
	if(argc!=2){
		printf("usage: %s <size>\n",argv[0]);
		exit(1);
	}
	pthread_mutex_init(&mutex,NULL);
	FILE *fp;
	int a[]={2,4,1,6,5,3,700,17,20,23,11,18,13,29,122,27,24,39,35,50,30,21,38,99,43,76,67,100},i; 
	v_size = sizeof(a)/sizeof(a[0]); 	/*SIZE OF VECTOR*/
	/*OPEN FILE FOR WRITING*/
	fp = fopen("save.bin","wb");
	if(!fp){ 
		printf("error");
		exit(1); 
	}
	/*WRITE TO FILE*/
	for(i=0;i<v_size;i++)  // or could use fwrite(a,sizeof(int),n,f);
    	//fwrite(&a[i],sizeof(int),1,fp);
		fprintf(fp, "%d\n", a[i]);  
	fclose(fp);
	printf("done writing\n");
	//fclose(fp);
	
	//*******OPEN FILE FOR READING(USING 'OPEN' SYSTEM CALL)***********
	int fd;
	if ((fd = open ("save.bin", O_RDWR)) == -1)
    	perror ("open");
   struct stat stat_buf;
	printf("reading file \n");
	if(fstat(fd,&stat_buf) == -1)
		perror("mmap");
	int len; len = stat_buf.st_size;	/*FILE SIZE*/
	
	/*MAP FILE TO MEMORY AS A VECTOR*/
	paddr = mmap((caddr_t)0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);
	if(paddr == (caddr_t) -1)
		perror("mmap error");
	int n;
	n = len / 2;
	//int *v;	
	close(fd);
	int v[] = {};
	printf("done mapping file to memory\n");
	printf("===============================================================================\n");
	int size = atoi(argv[1]);
	if(v_size < size) {
		/*SORT WITH STANDARD QUICKSORT ALGORITHM*/
		quicksort(v,1,v_size+1);
	   printArray(v,0,v_size);
	   printf("sorting done with standard algorithm\n");
	} else {
		/*SORT WITH MULTI-THREADED QUICKSORT*/	
		nthreads = ceil(log10(v_size));

		pthread_t *threads;
		threads = (pthread_t *)malloc(nthreads*sizeof(pthread_t));
		p = (Param *) malloc (nthreads * sizeof(Param));
	
		/*INIT MUTEX AND SEMAPHORE*/
		//pthread_mutex_init(&mutex,NULL);
		barrier1 =(sem_t *) malloc(sizeof(sem_t));
		sem_init(barrier1,0,0);
		barrier2 =(sem_t *) malloc(sizeof(sem_t));
		sem_init(barrier2,0,0);
		barrier3 =(sem_t *) malloc(sizeof(sem_t));
		sem_init(barrier3,0,0);
		
		int region_s = v_size/nthreads;	/*REGION SIZE*/
		//create threads
		for(i=0;i<nthreads;i++) {
			int *pi = (int *)malloc(sizeof(int));
			*pi = i;
			p[i].vec = v;
			p[i].left = (region_s * i)+1;
			p[i].tid = *pi;					/*THREAD ID*/
		    p[i].right = region_s * (i + 1)  ;	/*LAST THREAD TAKES RIGHTMOST REGION*/
		  	pthread_mutex_init(&p[i].mutex,NULL);
		  	pthread_mutex_init(&p[i].mutex2,NULL);
			pthread_create(&threads[i],NULL,QS,&p[i]);
		}
		printArray(p->vec,0,v_size);
		printf("===============================================================================\n");
		
		for(i=0;i<nthreads;i++)
			pthread_join(threads[i],NULL);
		
		//pthread_exit ((void *) pthread_self());
		//sem_wait(&sem1);
	printArray(p->vec,0,v_size);	
		//return 0;
	}
	
	return 0;
}



