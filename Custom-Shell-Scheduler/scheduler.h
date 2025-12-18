#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/types.h>

#define MAX_JOBS 64

// Job Status States
typedef enum {
    READY,
    RUNNING,
    DONE
} JobState;

// Structure to represent a single Job
typedef struct {
    pid_t pid;
    char name[256];
    JobState state;
    int started;
    int submission_slice;
    int completion_slice;
    int slices_ran;
    int slices_waited;
} Job;

// Shared Memory Structure exchanged between Shell and Scheduler
typedef struct {
    // List of all jobs
    Job jobs[MAX_JOBS];
    int job_count;

    // Ready Queue (Circular Buffer of indices)
    int ready_q[MAX_JOBS];
    int rq_head;
    int rq_tail;
    int rq_size;

    // New Job Queue (Circular Buffer of file paths)
    char new_job_q[MAX_JOBS][256];
    int njq_head;
    int njq_tail;
    int njq_size;
} SharedState;

// Function Prototypes
void run_scheduler(SharedState *S, int NCPU, int TSLICE);
void print_report(SharedState *S, int TSLICE);

#endif
