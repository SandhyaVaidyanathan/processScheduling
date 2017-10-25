typedef struct pcb{
    unsigned long cpuTime;
    unsigned long systemTime;
    unsigned long lastBurstTime;
    int priority;
}pcb;

typedef struct osClock {
    unsigned long sec;
    unsigned long nsec;
}shmClock;

