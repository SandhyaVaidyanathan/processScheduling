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

	int mypid = atoi(argv[0]);
	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
	alarm(20);

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


printf("This is in child %d id \n", mypid);
	shpcbinfo[mypid].parrivalsec = shinfo->sec;
    shpcbinfo[mypid].parrivalnsec = shinfo->nsec;


    // Random number in the range 0 - 3 to decide the course of action for the process
/*    do{
    	while(shinfo->scheduledpid != mypid && shpcbinfo[mypid].ready!=1);

    }while(flag) */
    srand(shinfo->sec);
    int processAction = rand()%(3+ 1 -0)+ 0;
    printf("processAction : %d \n",processAction );
    int r,s,p,flag = 1;
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
    			printf("Hey in case 2 ----\n");
    			//if(shpcbinfo[mypid].qid < 2)
    			//	shpcbinfo[mypid].qid++;
    			break;
    		}
    	case 3:
    		{	
    			p = rand()%99 + 1;
    			sleep(p);
    			printf("Hey in case 3 ----\n");
    			//fprintf(fp, "Not using its entire time quantum");
    			//if(shpcbinfo[mypid].qid < 2)
    			//	shpcbinfo[mypid].qid++;
    			break;
    		}
    	default:
    		break;
    }

if (shpcbinfo[mypid].qid == 0)
	q = quantum;
else if(shpcbinfo[mypid].qid == 1)
	q = quantum/2;
else
	q = quantum/4;

if (processAction == 3)
   	duration = rand()%q;
else
   	duration = q;

shpcbinfo[mypid].cpuTime +=duration;
return 0;
}

