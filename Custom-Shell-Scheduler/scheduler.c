// scheduler.c
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

static SharedState *g_S = NULL;
static int g_NCPU;
static int g_TSLICE;            // milliseconds
static int g_running_jobs[MAX_JOBS];
static int g_run_count = 0;
static int g_current_slice = 0;
static volatile int g_exit_flag = 0;

static void cleanup_child_processes(void);

static void sigterm_handler(int signum) {
    (void)signum;
    g_exit_flag = 1;
}

void enqueue(SharedState *S, int idx) {
    S->ready_q[S->rq_tail] = idx;
    S->rq_tail = (S->rq_tail + 1) % MAX_JOBS;
    S->rq_size++;
}

int dequeue(SharedState *S) {
    if (S->rq_size == 0) return -1;
    int idx = S->ready_q[S->rq_head];
    S->rq_head = (S->rq_head + 1) % MAX_JOBS;
    S->rq_size--;
    return idx;
}

static void check_for_new_jobs(void) {
    while (g_S->njq_size > 0) {
        if (g_S->job_count >= MAX_JOBS) {
            g_S->njq_head = (g_S->njq_head + 1) % MAX_JOBS;
            g_S->njq_size--;
            continue;
        }

        char *path = g_S->new_job_q[g_S->njq_head];

        pid_t pid = fork();
        if (pid < 0) {
            perror("scheduler fork failed");
            g_S->njq_head = (g_S->njq_head + 1) % MAX_JOBS;
            g_S->njq_size--;
            continue;
        }

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);

            char local_path[256];
            strncpy(local_path, path, sizeof(local_path) - 1);
            local_path[sizeof(local_path) - 1] = '\0';

            char *argv[] = { local_path, NULL };
            execvp(argv[0], argv);
            perror("execvp failed");
            _exit(127);
        }

        g_S->njq_head = (g_S->njq_head + 1) % MAX_JOBS;
        g_S->njq_size--;

        kill(pid, SIGSTOP);

        int idx = g_S->job_count;
        g_S->jobs[idx].pid = pid;
        strncpy(g_S->jobs[idx].name, path, sizeof(g_S->jobs[idx].name) - 1);
        g_S->jobs[idx].name[sizeof(g_S->jobs[idx].name) - 1] = '\0';
        g_S->jobs[idx].state = READY;
        g_S->jobs[idx].started = 0;
        g_S->jobs[idx].completion_slice = 0;
        g_S->jobs[idx].slices_ran = 0;
        g_S->jobs[idx].slices_waited = 0;
        g_S->jobs[idx].submission_slice = g_current_slice;
        enqueue(g_S, idx);
        g_S->job_count++;
    }
}

static void handle_time_slice(void) {
    g_current_slice++;

    int currently_running_count = g_run_count;
    g_run_count = 0;

    for (int i = 0; i < currently_running_count; i++) {
        int job_idx = g_running_jobs[i];
        Job *j = &g_S->jobs[job_idx];

        j->slices_ran++;
        kill(j->pid, SIGSTOP);

        int status;
        pid_t r = waitpid(j->pid, &status, WNOHANG);

        if (r == j->pid || (kill(j->pid, 0) == -1 && errno == ESRCH)) {
            j->state = DONE;
            j->completion_slice = g_current_slice;
        } else {
            j->state = READY;
            enqueue(g_S, job_idx);
        }
    }

    while (g_run_count < g_NCPU && g_S->rq_size > 0) {
        int idx_to_run = dequeue(g_S);
        if (idx_to_run == -1) break;

        Job *j = &g_S->jobs[idx_to_run];
        if (j->state == DONE) continue;

        kill(j->pid, SIGCONT);
        j->state = RUNNING;
        j->started = 1;
        g_running_jobs[g_run_count++] = idx_to_run;
    }

    for (int i = 0; i < g_S->rq_size; i++) {
        int idx_in_queue = g_S->ready_q[(g_S->rq_head + i) % MAX_JOBS];
        g_S->jobs[idx_in_queue].slices_waited++;
    }
    
}

void run_scheduler(SharedState *S, int NCPU, int TSLICE) {
    g_S = S;
    g_NCPU = NCPU;
    g_TSLICE = TSLICE;
    struct sigaction sa = {0};
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);

    int idle_cycles = 0;

    while (!g_exit_flag) {
        check_for_new_jobs();

        // If nothing to do, sleep without incrementing time
        if (g_run_count == 0 && g_S->rq_size == 0 && g_S->njq_size == 0) {
            usleep((useconds_t)g_TSLICE * 1000);
            idle_cycles++;
            continue;
        } else {
            idle_cycles = 0;
        }

        handle_time_slice();

        // Small sleep for pacing
        usleep((useconds_t)g_TSLICE * 1000);
    }

    cleanup_child_processes();
}


static void cleanup_child_processes(void) {
    if (!g_S) return;

    for (int i = 0; i < g_S->job_count; i++) {
        if (g_S->jobs[i].state != DONE) {
            kill(g_S->jobs[i].pid, SIG KILL);
            waitpid(g_S->jobs[i].pid, NULL, 0);
            g_S->jobs[i].state = DONE;
            g_S->jobs[i].completion_slice = g_current_slice;
        }
    }
}

void print_report(SharedState *S, int TSLICE) {
    printf("\nExecution Report:\n");
    printf("%-20s\t%-10s\t%-15s\t\t%-15s\n", "Name", "PID", "Turnaround Time", "Wait Time");

    for (int i = 0; i < S->job_count; i++) {
        Job j = S->jobs[i];
        int wait_time_ms = j.slices_waited ;
        int turnaround_time_ms;

        if (j.state == DONE) {
            turnaround_time_ms = (j.completion_slice - j.submission_slice);
            if (turnaround_time_ms < 0 || turnaround_time_ms > 60000)
                turnaround_time_ms = j.slices_ran ;
        } else {
            turnaround_time_ms = j.slices_ran;
        }

        printf("%-20s\t%-10d\t%-5d TSLICES\t\t%-5d TSLICES\n",
               j.name, j.pid, turnaround_time_ms, wait_time_ms);
    }
    fflush(stdout);
}

