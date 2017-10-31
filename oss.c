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

int spawnedSlaves = 0;
pid_t childpid;
char* arg1;
int shmid;
shmClock *shinfo;

const unsigned long int NANOSECOND = 1000000000; 

int main(int argc, char const *argv[])
{
	char* logfile;
	logfile = "log.txt";
	key_t clock_key, pcb_key;
	arg1 = (char*)malloc(40);
	//Create Shared Memory
	clock_key = 555;
	pcb_key = 666;
	//Create shared memory segment 
	shmid = shmget(clock_key, 20*sizeof(shinfo), 0766 |IPC_CREAT |IPC_EXCL);
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