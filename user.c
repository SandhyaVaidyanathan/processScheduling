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

	int mypid = atoi(argv[0]);
	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
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
	init_time.sec = shinfo->sec;
    init_time.nsec = shinfo->nsec;
return 0;
}