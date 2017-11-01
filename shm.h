typedef struct pcb{
    pid_t pcbId;
    unsigned long processStartTime; //time when process is forked
    unsigned long cpuTime; //cpu time used
    unsigned long systemTime; // total time in the system
    unsigned long lastBurstTime; //time used during last burst
    int priority; // process priority
}shmPcb;

typedef struct osClock {
    unsigned long sec;
    unsigned long nsec;
}shmClock;

