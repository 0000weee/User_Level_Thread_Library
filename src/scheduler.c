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
//SIGTSTP: Used for manual context switching, triggered by pressing Ctrl+Z on terminal or the judge.
//SIGALRM: Used for automatic context switching, triggered by the alarm() system call to enforce time slices.

void sighandler(int signum) {
    if (signum == SIGTSTP){
        printf("caught SIGTSTP\n");
        // 接著要處理 SIGTSTP 信號

    }else if (signum == SIGALRM){
        printf("caught SIGALRM\n");
        // 接著要處理 SIGALRM 信號
    }else{
        // 處理 default signal, 忽略它?
    }
    longjmp(sched_buf, JUMP_FROM_SIGNAL_HANDLER);
}

void reset_alarm() {
    alarm(0);                    // 取消之前的鬧鐘
    alarm(time_slice);           // 設置新的鬧鐘（時間片單位）
}

void clear_pending_signals() {
    struct sigaction sa_ignore, sa_default;

    // 暫時設置忽略信號
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;

    sigaction(SIGTSTP, &sa_ignore, NULL);
    sigaction(SIGALRM, &sa_ignore, NULL);

    // 恢復預設處理程序
    sa_default.sa_handler = sighandler;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;

    sigaction(SIGTSTP, &sa_default, NULL);
    sigaction(SIGALRM, &sa_default, NULL);
}


void switch_thread(struct tcb *next_thread) {
    if (setjmp(current_thread->env) == 0) {
        current_thread = next_thread;
        siglongjmp(next_thread->env, 1);  // 跳到下一個執行緒上下文
    }
}

struct sleeping_set sleeping_set;

void managing_sleeping_threads(){
    for (int i = 0; i < sleeping_set.size; i++) {
        if (sleeping_set.threads[i].sleeping_time > 0) {
            sleeping_set.threads[i].sleeping_time -= time_slice;
            if (sleeping_set.threads[i].sleeping_time <= 0) {
                queue_add(ready_queue, sleeping_set.threads[i]);
            }
        }
    }
}

void handling_waiting_threads(){
    /*  If the head of the waiting_queue can acquire the resource, 
        it is moved to the ready_queue. 
        Repeat this process until no more threads can be moved.
    */
    while (waiting_queue.size > 0){
        struct tcb* head_of_the_waiting_queue = waiting_queue.arr[waiting_queue.head % THREAD_MAX];

        switch (head_of_the_waiting_queue->waiting_for){
            case 0: // no resource
                queue_add(handling_waiting_threads, ready_queue);
                waiting_queue.head++;
                waiting_queue.size--;
                break;
            case 1: // read lock
                if(rwlock.write_count == 0){
                    queue_add(handling_waiting_threads, ready_queue);
                    waiting_queue.head++;
                    waiting_queue.size--;
                }
                break;
            case 2: // write lock
                if(rwlock.write_count == 0 && rwlock.read_count == 0){
                    queue_add(handling_waiting_threads, ready_queue);
                    waiting_queue.head++;
                    waiting_queue.size--;
                }
                break;
            default:
                break;
        }
    }
}
// TODO::
// Perfectly setting up your scheduler.
void scheduler() {
    // Your code here
    int jmpVal = setjmp(sched_buf);
    if (jmpVal == 0){ // Scheduler Initialization
        // Create the idle thread with ID 0 and the routine idle().
        thread_create(0, idle, NULL);
    }
    else {
        while (1) {
            // 1. Reset the Alarm
            reset_alarm();

            // 2. Clearing the Pending Signals       
            clear_pending_signals();

            // 3. Managing Sleeping Threads
            managing_sleeping_threads();
            
            // 4. Handling Waiting Threads
            handling_waiting_threads();
            // 5. Handling Previously Running Threads

            // 6. Selecting the Next Thread

            // 7. Context Switching
        }
    }
}
