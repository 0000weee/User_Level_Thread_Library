#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "routine.h"
#include "thread_tool.h"

// TODO::
// Prints out the signal you received.
// This function should not return. Instead, jumps to the scheduler.
void sighandler(int signum) {
    if (signum == SIGTSTP){
        // 處理 SIGTSTP 信號
        printf("caught SIGTSTP\n");

    }else if(signum == SIGALRM){
        // 處理 SIGALRM 信號
        printf("caught SIGALRM\n");
    }else{
        // 處理 default signal
    }
}

// TODO::
// Perfectly setting up your scheduler.
void scheduler() {
    // Your code here
    if (setjmp(sched_buf) == 0){ // Call setjmp() to save the scheduler's context
        //Create the idle thread with ID 0 and the routine idle().
        idle(0, NULL);
    }
}
