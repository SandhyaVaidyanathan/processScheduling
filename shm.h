typedef struct pcb{
    pid_t pcbId;
    unsigned long parrivalsec; //time when process is forked
    unsigned long parrivalnsec;
    unsigned long cpuTime; //cpu time used
    unsigned long systemTime; // total time in the system
    unsigned long lastBurstTime; //time used during last burst
    int priority; // process priority
    int qid;
    int ready;
    unsigned long q;
}shmPcb;

typedef struct osClock {
    unsigned long sec;
    unsigned long nsec;
    int scheduledpid;
}shmClock;

