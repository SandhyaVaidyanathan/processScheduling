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
int mypid = -1;

const unsigned long int NANOSECOND = 1000000000; 
const int TOTALPROCESS = 100; //default

int main(int argc, char const *argv[])
{
	char* logfile;
	logfile = "log.txt";
	int ztime = 20; //default
	int noOfSlaves = 5; // default
	key_t clock_key, pcb_key;
	unsigned long startTime = 0;
	arg1 = (char*)malloc(40);


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
		shpcbinfo->processStartTime = 0;
		shpcbinfo->cpuTime = 0;
		shpcbinfo->lastBurstTime = 0;
		shpcbinfo->priority = -1;
		shpcbinfo->systemTime = 0;
	}


// Open log file 
FILE *fp = fopen(logfile, "w");
//initialize all pcb elements
int i;
for (i = 0; i < noOfSlaves; ++i)
{
	shpcbinfo[i].pcbId = -1;
	shpcbinfo[i].processStartTime = 0; //time when process is forked
	shpcbinfo[i].priority = -1; // process priority
	shpcbinfo[i].cpuTime = 0;	//cpu time used
	shpcbinfo[i].systemTime = 0; // total time in the system
	shpcbinfo[i].lastBurstTime = 0; //time used during last burst
}
//logical clock
fprintf(stderr, "Starting the clock..\n" );
startTime = time(NULL);
srand(time(NULL));

while(shinfo->sec <= ztime)
{
int xx = rand()%1000;
long unsigned currentTime = (shinfo->sec*NANOSECOND)+shinfo->nsec;
long unsigned plus1xxTime = currentTime + NANOSECOND +xx;
int flag = 0;
    while ( currentTime <plus1xxTime)
       	{
    		shinfo->nsec += xx;
        	if (shinfo->nsec > (NANOSECOND - 1))
            {
            shinfo->nsec -= (NANOSECOND - 1);
            	shinfo->sec++;
            }
       	}
     
    //spawn process if process table is not full.

       	    spawnSlaveProcess(1);

}


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
	//Forking processes
	for(i = 0; i < noOfSlaves; i++) 
	{ 
       	if((childpid = fork())<=0)
       		break;
    }

    if (childpid == 0)
	    {
    	//execl user.c    	
			fprintf(stderr,"exec %d\n",i);  
			sprintf(arg1, "%d", i);
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