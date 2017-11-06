#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <getopt.h>
#include <stdbool.h>

#include "shm.h"

void spawnSlaveProcess(int);
void interruptHandler(int);
void clearSharedMem1();
void clearSharedMem2();

pid_t childpid;
char* arg1;
int shmid, shmpcbid;
shmClock *shinfo;
shmPcb *shpcbinfo;
int status = 0;
int spawnedSlaves = 0;
int mypid = 0;
long processWaitTime;

//queues
//Begin queue stuff//

struct queue {
  pid_t id;
  struct queue *next;

} *front0, *front1, *front2,
  *rear0, *rear1, *rear2, 
  *temp0, *temp1, *temp2, 
  *frontA0, *frontA1, *frontA2;

int q0size;
int q1size;
int q2size;
int q3size;

bool isEmpty(int);
void Enqueue(int, int);
int Dequeue(int);
void clearQ(void);
int scheduleProcess(void);
void AvgTAT(int);

const unsigned long int NANOSECOND = 1000000000; 
const int TOTALPROCESS = 100; //default
const int ALPHA = 20000000;
const int BETA = 15000000;

FILE *fp;

int main(int argc, char const *argv[])
{
	char* logfile;
	logfile = "log.txt";
	int ztime = 30; //default
	int noOfSlaves = 5; // default
	key_t clock_key, pcb_key;
	unsigned long startTime = 0;
	int option = 0;
	arg1 = (char*)malloc(40);

  	while ((option = getopt(argc, argv,"hs:l:t:")) != -1) 
  	{		
  	switch (option) 
  	{		
  		case 'h' :		
           printf("Usage executable -s {no. of slave processes to be spawned} -t {time in seconds when master will terminate with all children} \n");		
           return 1;
           break;	
		  case 's':
    	   noOfSlaves = atoi(optarg);
    	   if(noOfSlaves < 1 || noOfSlaves >19)
    	   {
    	   		fprintf(stderr, "Slave processes count out of range, continuing with default value 5" );
    	   		noOfSlaves = 5; // default value
    	   }

           break;

      case 't':
        	ztime = atoi(optarg);
        	if (ztime <= 0)
        	{
        		fprintf(stderr, "Time should be greater than 0, continuing with default value 20" );
        		ztime = 20; //default value
        	}
        	break;
        default:
        	 printf("Usage executable -s {no. of slave processes to be spawned} -t {time in seconds when master will terminate with all children} \n");		
        }
    }

//signal handling 
	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
	alarm(ztime);	

	clock_key = 555;
	pcb_key = 666;
	//Create shared memory segment for clock
	shmid = shmget(clock_key, 40*sizeof(shinfo), 0766 |IPC_CREAT |IPC_EXCL);
	if ((shmid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to create shared memory");
		return -1;
	}
	if (shmid == -1)
	{
		printf("Shared Memory Already created");
		return -1;
	}
	else
	{
		shinfo = (shmClock*)shmat(shmid,NULL,0);
		if (shinfo == (void*)-1)
			return -1;
		// clock initially set to 0
		shinfo->nsec = 0;
		shinfo->sec = 0;
		shinfo->scheduledpid = -1;
	}

	//create shared memory for pcb
	shmpcbid = shmget(pcb_key, 40*sizeof(shpcbinfo), 0766 |IPC_CREAT |IPC_EXCL);
	if ((shmpcbid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to create shared memory");
		return -1;
	}
	if (shmpcbid == -1)
	{
		printf("Shared Memory Already created");
		return -1;
	}
	else
	{
		shpcbinfo = (shmPcb*)shmat(shmpcbid,NULL,0);
		if (shpcbinfo == (void*)-1)
			return -1;
		// clock initially set to 0
		shpcbinfo->pcbId = -1;
		shpcbinfo->parrivalsec = 0;
		shpcbinfo->parrivalnsec = 0;
		shpcbinfo->cpuTime = 0;
		shpcbinfo->lastBurstTime = 0;
		shpcbinfo->priority = -1;
		shpcbinfo->systemTime = 0;
		shpcbinfo->qid = 0;
	}


// Open log file 
fp = fopen(logfile, "w");
//initialize all pcb elements

int i;
for (i = 0; i < noOfSlaves; ++i)
{
	shpcbinfo[i].pcbId = -1;
	shpcbinfo[i].parrivalsec = 0; //time when process is forked
	shpcbinfo[i].parrivalnsec=0;
	shpcbinfo[i].priority = -1; // process priority
	shpcbinfo[i].cpuTime = 0;	//cpu time used
	shpcbinfo[i].systemTime = 0; // total time in the system
	shpcbinfo[i].lastBurstTime = 0; //time used during last burst
	shpcbinfo[i].qid = 0;
	shpcbinfo[i].ready = 0; //ready to execute if 1
}
//logical clock
fprintf(fp, "Starting the clock..\n" );
//for queues
  front0 = front1 = front2 =  NULL;
  rear0 = rear1 = rear2 =  NULL;
  q0size = q1size = q2size =  0;
startTime = time(NULL);
srand(time(NULL));

while(shinfo->sec < ztime && spawnedSlaves < noOfSlaves)
{
int xx = rand()%1000;
long unsigned currentTime = (shinfo->sec*NANOSECOND)+shinfo->nsec;
long unsigned plus1xxTime = currentTime + NANOSECOND +xx;
int flag = 0;

//Don't generate if the process table is full -- ---add
    while ( currentTime <plus1xxTime)
       	{

    		//shinfo->nsec += xx;
    		shinfo->nsec += xx;
        	if (shinfo->nsec > (NANOSECOND - 1))
            {
            shinfo->nsec -= (NANOSECOND - 1);
            	shinfo->sec++;
            }
           currentTime = (shinfo->sec*NANOSECOND)+shinfo->nsec; 
       	}
     
    //spawn process if process table is not full.
       	    fprintf(fp, "Generating process with PID %d and putting it in queue 0 at time %d:%lu\n",mypid,shinfo->sec,shinfo->nsec);
       	    Enqueue(mypid,0);
       	    spawnSlaveProcess(1);

			xx = rand()%1000;
			plus1xxTime = currentTime + NANOSECOND +xx;

}

		while(shinfo->scheduledpid != -1);
		{
			shinfo->scheduledpid = scheduleProcess();
			shinfo->scheduledpid == -1;
		}


//Average wait time
	void AvgTAT(int pcb) {

  long long startToFinish = (shinfo->sec*NANOSECOND +shinfo->nsec) - (shpcbinfo[pcb].parrivalsec*NANOSECOND + shpcbinfo[pcb].parrivalnsec);
 // long systemTime += startToFinish;
  processWaitTime += startToFinish - shpcbinfo[pcb].q;
}
	printf("Average wait time : %lu \n" ,processWaitTime%noOfSlaves);

	clearQ();

//clearing memory
	free(arg1);
	wait(&status);
	clearSharedMem1();
	clearSharedMem2();
	fclose(fp);
	kill(-getpgrp(),SIGQUIT);
	return 0;
}

void spawnSlaveProcess(int noOfSlaves)
{
	int i;
	mypid++;
	//Forking processes
	for(i = 0; i < noOfSlaves; i++) 
	{ 
       	if((childpid = fork())<=0)
       		break;
    }

    if (childpid == 0)
	    {
    	//execl user.c    	
	    	//shinfo->scheduledpid = 3;
			//fprintf(stderr,"exec %d\n",mypid);  
			sprintf(arg1, "%d", mypid);
			//Calling user.c program

			execl("user", arg1, NULL); 
    	}
    	spawnedSlaves++;
}

void interruptHandler(int SIG){
  signal(SIGQUIT, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  if(SIG == SIGINT)
   {
    fprintf(stderr, "\nCTRL-C encoutered, killing processes\n");
  	}

  if(SIG == SIGALRM) 
  {
    fprintf(stderr, "Master has timed out. killing processes\n");
  }

    kill(-getpgrp(), 9);
  	clearSharedMem1();
  	clearSharedMem2();
}
void clearSharedMem1()
{
	int error = 0;
	if(shmdt(shinfo) == -1) {
		error = errno;
	}
	if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
		error = errno;
	}
	if(!error) {
		return ;
	}
}

void clearSharedMem2()
{
	int error = 0;
	if(shmdt(shpcbinfo) == -1) {
		error = errno;
	}
	if((shmctl(shmpcbid, IPC_RMID, NULL) == -1) && !error) {
		error = errno;
	}
	if(!error) {
		return ;
	}
}


bool isEmpty(int choice) {
  switch(choice) {
    case 0:
      if((front0 == NULL) && (rear0 == NULL))
        return true;
      break;
    case 1:
      if((front1 == NULL) && (rear1 == NULL))
        return true;
      break;
    case 2:
      if((front2 == NULL) && (rear2 == NULL))
        return true;
      break;
    default:
      printf("Not a valid queue choice\n");
  }
  return false;
}

void Enqueue(int processId, int choice) {
	shpcbinfo[mypid].qid = choice;
  printf("Putting pid %d in queue %d \n", processId, choice);
  switch(choice) {
    case 0:     
      fprintf(fp,"Putting pid %d in queue %d \n",processId,choice);
      if(rear0 == NULL) {
        rear0 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear0->next = NULL;
        rear0->id = processId;
        front0 = rear0;
      }
      else {
        temp0 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear0->next = temp0;
        temp0->id = processId;
        temp0->next = NULL;

        rear0 = temp0;
      }
      q0size++;
      break;
    case 1:
      printf("Putting pid %d in queue %d\n", processId, choice);
      if(rear1 == NULL) {
        rear1 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear1->next = NULL;
        rear1->id = processId;
        front1 = rear1;
      }
      else {
        temp1 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear1->next = temp1;
        temp1->id = processId;
        temp1->next = NULL;
        rear1 = temp1;
      }
      q1size++;
      break;
    case 2:
      printf("oss : Putting pid %d in queue %d\n",  processId,  choice);
      if(rear2 == NULL) {
        rear2 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear2->next = NULL;
        rear2->id = processId;
        front2 = rear2;
      }
      else {
        temp2 = (struct queue*)malloc(1 * sizeof(struct queue));
        rear2->next = temp2;
        temp2->id = processId;
        temp2->next = NULL;
        rear2 = temp2;
      }
      q2size++;
      break;
    default:
      printf("Not a valid queue choice\n");
  }
}

int Dequeue(int choice) {
  int poppedID;
  long dispatch;
  switch(choice) {
    case 0:
      frontA0 = front0;
      if(frontA0 == NULL) {
        printf("Error: dequeuing an empty queue\n");
      }
      else {
        if(frontA0->next != NULL) {
          frontA0 = frontA0->next;
          poppedID = front0->id;
          free(front0);
          front0 = frontA0;
        }
        else {
          poppedID = front0->id;
          free(front0);
          front0 = NULL;
          rear0 = NULL;
        }
        printf("oss : Dispatching pid %d from queue %d at %lu:%lu \n", poppedID,choice,shinfo->sec,shinfo->nsec);
        dispatch = (shinfo->sec*NANOSECOND + shinfo->nsec) - shpcbinfo[mypid].systemTime;
        printf("oss :Total time this dispatch was %lu nanoseconds\n",dispatch );
        q0size--;
        	shinfo->scheduledpid = -1;	
      }
      break;
    case 1:
      frontA1 = front1;
      if(frontA1 == NULL) {
        printf("Error: dequeuing an empty queue\n");
      }
      else {
        if(frontA1->next != NULL) {
          frontA1 = frontA1->next;
          poppedID = front1->id;
          free(front1);
          front1 = frontA1;
        }
        else {
          poppedID = front1->id;
          free(front1);
          front1 = NULL;
          rear1 = NULL;
        }
        printf("Dispatching pid %d from queue %d\n", poppedID, choice);
        q1size--;
        dispatch = (shinfo->sec*NANOSECOND + shinfo->nsec) - shpcbinfo[mypid].systemTime;
        printf("Total time this dispatch was %lu nanoseconds\n",dispatch );
        shinfo->scheduledpid = -1;	
      }
      break;
    case 2:
      frontA2 = front2;
      if(frontA2 == NULL) {
        printf("Error: dequeuing an empty queue\n");
      }
      else {
        if(frontA2->next != NULL) {
          frontA2 = frontA2->next;
          poppedID = front2->id;
          free(front2);
          front2 = frontA2;
        }
        else {
          poppedID = front2->id;
          free(front2);
          front2 = NULL;
          rear2 = NULL;
        }
        printf("Dispatching pid %d from queue %d%s\n", poppedID, choice);
        q2size--;
        dispatch = (shinfo->sec*NANOSECOND + shinfo->nsec) - shpcbinfo[mypid].systemTime;
        printf("Total time this dispatch was %lu nanoseconds\n",dispatch );
        shinfo->scheduledpid = -1;	
      }
      break;
    default:
      printf("Not a valid queue choice\n");
  }
  fprintf(fp, "Dispatching pid %d from queue %d\n", poppedID, choice);
  return poppedID;
}

void clearQ(void) {
  while(!isEmpty(0)) {
    Dequeue(0);
  }
  while(!isEmpty(1)) {
    Dequeue(1);
  }
  while(!isEmpty(2)) {
    Dequeue(2);
  }
  return ;
}
int scheduleProcess(void) {
  if(!isEmpty(0)) {
    return Dequeue(0);
  }
  else if(!isEmpty(1)) {
    return Dequeue(1);
  }
  else if(!isEmpty(2)) {
    return Dequeue(2);
  }

  else return -1;

}

