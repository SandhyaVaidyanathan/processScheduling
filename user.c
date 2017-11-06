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
#include <getopt.h>

#include "shm.h"

int shmid, shmpcbid;
shmClock *shinfo;
shmPcb *shpcbinfo;
shmClock init_time; // starting time for the process

const unsigned long int NANOSECOND = 1000000000; 
const int quantum = 800000;
int q;

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

	kill(-getpgrp(), SIGQUIT);
}


int main(int argc, char const *argv[])
{

	key_t clock_key, pcb_key;
	clock_key = 555;
	pcb_key = 666;
	long duration;
	char* logfile;
	logfile = "log.txt";

	int mypid = atoi(argv[0]);
	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
	alarm(200);
//	FILE *fp = fopen(logfile, "w");
	//Read shared memory segments
	shmid = shmget(clock_key, 20*sizeof(shinfo), 0744|IPC_EXCL);
	if ((shmid == -1) && (errno != EEXIST)) 
	{
		perror("Unable to read shared memory");
		return -1;
	} 
	else
	{
		shinfo = (shmClock*)shmat(shmid,NULL,0);
		if (shinfo == (void*)-1)
		{
			printf("Cannot attach shared memory\n");
			return -1;
		}
	}


	shmpcbid = shmget(pcb_key, 20*sizeof(shpcbinfo), 0744|IPC_EXCL);
	if ((shmpcbid == -1) && (errno != EEXIST)) 
	{
		perror("Unable to read shared memory");
		return -1;
	} 
	else
	{
		shpcbinfo = (shmPcb*)shmat(shmpcbid,NULL,0);
		if (shpcbinfo == (void*)-1)
		{
			printf("Cannot attach shared memory\n");
			return -1;
		}
	}


printf("This has arrived in child %d id \n", mypid);
	shpcbinfo[mypid].parrivalsec = shinfo->sec;
    shpcbinfo[mypid].parrivalnsec = shinfo->nsec;


    // Random number in the range 0 - 3 to decide the course of action for the process

    srand(shinfo->sec);
    int processAction = rand()%(3+ 1 -0)+ 0;
   // printf("processAction : %d \n",processAction );
    int r,s,p,finished = 0;

    switch(processAction)
    {
    	case 0: 
    		{
    			//remove from queue, terminate
    			//flag=0;
    			break;
    		}
    	case 1:
    		{
    			//remove from queue after its time quantum
    			break;
    		}
    	case 2:
    		{
    			//wait for IO, move to next queue(not ready queue)
    			r = rand()%5 + 1;
    			s = rand()%1000 + 1;
    			if(shpcbinfo[mypid].qid < 2)
    				shpcbinfo[mypid].qid++;
    			break;
    		}
    	case 3:
    		{	
    			p = rand()%99 + 1;
    			sleep(p);
    			//fprintf(fp, "Not using its entire time quantum");
    			if(shpcbinfo[mypid].qid < 2)
    				shpcbinfo[mypid].qid++;
    			break;
    		}
    	default:
    		break;
    }

    do{

    while(mypid != shinfo->scheduledpid);
if (shpcbinfo[mypid].qid == 0)
	shpcbinfo[mypid].q = quantum;
else if(shpcbinfo[mypid].qid == 1)
	shpcbinfo[mypid].q = quantum/2;
else
	shpcbinfo[mypid].q = quantum/4;

printf("%lu is the quantum  \n",q );
if (processAction == 3)
   	duration = rand()%q;
else
   	duration = q;

printf("%lu is the duration \n",duration );
shpcbinfo[mypid].cpuTime +=duration;
shinfo->nsec +=duration;
    if (shinfo->nsec > (NANOSECOND - 1))
        {
        shinfo->nsec -= (NANOSECOND - 1);
        shinfo->sec++;
        }


if(shpcbinfo[mypid].cpuTime >= q)
	{
	shpcbinfo[mypid].lastBurstTime = duration;
    shpcbinfo[mypid].systemTime = (shinfo->sec * NANOSECOND + shinfo->nsec) - (shpcbinfo[mypid].parrivalsec*NANOSECOND + shpcbinfo[mypid].parrivalnsec);
	finished = 1;
	shinfo->scheduledpid = -1;		
	}
shinfo->scheduledpid = -1;	
printf( "Exiting this process");


}while(!finished);

  if(shmdt(shinfo) == -1) {
    perror("    Slave could not detach shared memory struct");
  }

  if(shmdt(shpcbinfo) == -1) {
    perror("    Slave could not detach from shared memory array");
  }
kill(mypid,9);
return 0;
}
