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
#define JUMP_FROM_SIGNAL_HANDLER 2
void sighandler(int signum) {
    if (signum == SIGTSTP){
        // 處理 SIGTSTP 信號
        printf("caught SIGTSTP\n");

    }else if (signum == SIGALRM){
        // 處理 SIGALRM 信號
        printf("caught SIGALRM\n");
    }else{
        // 處理 default signal
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
// TODO::
// Perfectly setting up your scheduler.
// 1. Reset the Alarm
// 2. Clearing the Pending Signals
// 3. Managing Sleeping Threads
void scheduler() {
    // Your code here
    int jmpVal = setjmp(sched_buf);
    if (jmpVal == 0){ // Call setjmp() to save the scheduler's context
        //Create the idle thread with ID 0 and the routine idle().
        idle(0, NULL);
    }
    else if (jmpVal = JUMP_FROM_SIGNAL_HANDLER){
        while (1) {
            clear_pending_signals(); // 清除掛起的信號
            reset_alarm();           // 重設鬧鐘
            
            // 從就緒隊列中選擇下一個執行緒
            struct tcb *next_thread = pick_next_thread();
            if (next_thread) {
                switch_thread(next_thread);
            } else {
                idle(0, NULL);       // 無可運行執行緒時進入閒置
            }
        }
    }
}
