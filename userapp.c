#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#define true 1 
#define false 0

typedef int bool; 

/**
 * Helper function that calculates the difference between two timeval structs.
 * Obtained from the Internet.
 */
int timeval_subtract (struct timeval *result, struct timeval *x,struct timeval  *y)  
{  
    /* Perform the carry for the later subtraction by updating y. */  
    if (x->tv_usec < y->tv_usec) {  
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;  
        y->tv_usec -= 1000000 * nsec;  
        y->tv_sec += nsec;  
    }  
    if (x->tv_usec - y->tv_usec > 1000000) {  
        int nsec = (y->tv_usec - x->tv_usec) / 1000000;  
        y->tv_usec += 1000000 * nsec;  
        y->tv_sec -= nsec;  
    }  

    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */  
    result->tv_sec = x->tv_sec - y->tv_sec;  
    result->tv_usec = x->tv_usec - y->tv_usec;  

    /* Return 1 if result is negative. */  
    return x->tv_sec < y->tv_sec;  
}

/**
 * Helper function that registers the current process.
 */
void Register(int comptime, int period, int pid) {
    FILE *f = fopen("/proc/edf/status", "w");
    fprintf(f, "R %d %d %d", pid, period, comptime);
    fclose(f);
}

/**
 * Helper function that calls yield for the current process.
 */
void Yield(int pid){
    FILE *f = fopen("/proc/edf/status", "w");
    fprintf(f, "Y %d", pid);
    fclose(f);
}

/**
 * Helper function that de-registers the current process.
 */
void Deregister(int pid){
    FILE *f = fopen("/proc/edf/status", "w");
    fprintf(f, "D %d", pid);
    fclose(f);
}

/**
 * Helper function that checks whether the current process has been
 * successfully registered.
 */
int read_status(pid_t pid){

    // data read from file
    int read_pid;
    
    // variables for readlines
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    // return variable
    bool val = false;
    
    FILE *fp = fopen("/proc/edf/status", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    // read log line by line
    while ((read = getline(&line, &len, fp)) != -1) {
        sscanf(line, "%d\n", &read_pid);
        if (read_pid == pid){
            val = true;
        }
    }

    // close file pointer
    fclose(fp);
    
    // free line
    if (line)
        free(line);
        
    return val;
}

/**
 * Helper function that does one job in one iteration.
 */
void do_job(int n){
    int res = 1;
    while(n != 0){
        res = res + 1;
        n --;
    }
}


int main(int argc, char* argv[]) {

    // Add a deadline = period
    int period = atoi(argv[1]);  // period time
    int deadline = period;
    int comptime = atoi(argv[2]);  // computation time of one job
    int num_periods = atoi(argv[3]);   // number of periods/jobs
    int n = 1000 * atol(argv[4]);  // length of each job -- number of interations in each 
    int count = 0;
 
    struct timeval wake_tv;
    struct timeval finish_tv;
    struct timeval diff;

    pid_t pid = getpid();
    
    Register(comptime, period, pid);
    
    /* Check if Register successfully */
    bool exist;
    exist = read_status(pid); //Proc filesystem: Verify the process was admitted
    
    if (!exist){
        printf("PID not registered\n");
        exit(1);    // exit if failed
    }
    
    Yield(pid);

    //this is the real-time loop
    while(num_periods >= count)
    {
        printf ("Iteration %d:\n", count);

        gettimeofday(&wake_tv, NULL);
        printf ("  Woke up at %ld.%06ld\n", wake_tv.tv_sec, wake_tv.tv_usec);

        count++;
        do_job(n);
        
        gettimeofday(&finish_tv, NULL);
        timeval_subtract(&diff, &finish_tv, &wake_tv);
        printf ("  Did work for  %ld.%06ld seconds\n", diff.tv_sec, diff.tv_usec);

        Yield(pid);
    }

    Deregister(pid);
}
