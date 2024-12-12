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
    }else if (signum == SIGALRM){
        printf("caught SIGALRM\n");
    }else{
        // 處理 default signal, 忽略它?
    }
    siglongjmp(sched_buf, JUMP_FROM_SIGNAL_HANDLER);
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

struct sleeping_set sleeping_set;

void managing_sleeping_threads(){
    for (int i = 0; i < sleeping_set.size; i++) {
        //sleeping_set.threads[i]->sleeping_time -= time_slice;
        if (sleeping_set.threads[i]->sleeping_time <= 0) {
            //printf("id %d sleep to ready\n",sleeping_set.threads[i]->id);
            enqueue(&ready_queue, sleeping_set.threads[i]);
            remove_sleeping_set_by_index(sleeping_set, i);// modify
            i--;
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
        if (head_of_the_waiting_queue == NULL){
            return;
        }
        //print_queue();
        switch (head_of_the_waiting_queue->waiting_for){
            case 0: // no resource
                enqueue(&ready_queue, head_of_the_waiting_queue);
                waiting_queue.head++;
                waiting_queue.size--;
                break;
            case 1: // read lock
                if (rwlock.write_count == 0){
                    enqueue(&ready_queue, head_of_the_waiting_queue);
                    waiting_queue.head++;
                    waiting_queue.size--;
                }
                break;
            case 2: // write lock
                if (rwlock.write_count == 0 && rwlock.read_count == 0){
                    enqueue(&ready_queue, head_of_the_waiting_queue);
                    waiting_queue.head++;
                    waiting_queue.size--;
                }
                break;
            default:
                break;
        }
    }
}

void handling_previously_running_threads(int prev_thread_status){
    // 根據返回值處理當前執行緒
    switch (prev_thread_status) {
        case JUMP_FROM_SIGNAL_HANDLER:
            for(int i=0; i<sleeping_set.size; i++){
                sleeping_set.threads[i]->sleeping_time -= time_slice;
            }
            if (current_thread->id != 0) // 非 idle
                enqueue(&ready_queue, current_thread);
            break;
        case JUMP_FROM_LOCK:                                         
            enqueue(&waiting_queue, current_thread);
            thread_yield();  
            break;
        case JUMP_FROM_SLEEP:
            // 已處理，不需操作
            break;
        case JUMP_FROM_EXIT:
            free(current_thread->args);
            free(current_thread);
            break;
    }
    //print_queue();
}

void enqueue(struct tcb_queue* queue, struct tcb *thread){
    int idx = (queue->head + queue->size) % THREAD_MAX;
    queue->arr[idx] = thread;
    queue->size++;
}

struct tcb* dequeue(struct tcb_queue* queue){
    if(queue->size > 0){
        queue->head++;
        queue->size--;
        //printf("q_head-1 %d\n", (queue->head) -1);
        return queue->arr[((queue->head) -1) % THREAD_MAX];
    }else{
        return NULL;
    }
}

void selecting_the_next_thread() {
    //printf("head: %d, size: %d\n", ready_queue.head, ready_queue.size);
    struct tcb *next_thread = dequeue(&ready_queue); // 從 ready_queue 中取出下一個執行緒
    if (next_thread != NULL) { // ready_queue 有執行緒

        current_thread = next_thread;
        
    } else { // ready_queue 為空       

        if (sleeping_set.size > 0) {
            // 如果有執行緒在睡眠中，調度 idle 執行緒
            current_thread = idle_thread;
        } else {

            // 沒有任何執行緒，清理 idle 的資源並返回
            free(idle_thread->args);
            free(idle_thread);
            return; // 返回到 start_threading()，進一步返回到 main()
        }
    }
    // 7. Context Switching
    printf("current_thread->id: %d\n", current_thread->id);
    siglongjmp(current_thread->env, JUMP_FROM_SCHEDULER);
}

// TODO::
// Perfectly setting up your scheduler.
void scheduler() {    
    // Scheduler Initialization
    thread_create(idle, 0, NULL);
    int jmpVal = sigsetjmp(sched_buf, 1);

    // 1.
    reset_alarm();
    // 2.      
    handling_previously_running_threads(jmpVal);
    clear_pending_signals();
    // 3.
    managing_sleeping_threads();
    // 4.
    handling_waiting_threads();
    // 5.
    //
    // 6.
    selecting_the_next_thread();
}


void print_queue(){
    printf("ready queue: ");
    for(int i = 0; i<ready_queue.size; i++){
        printf("%d ,", ready_queue.arr[(ready_queue.head+i)%THREAD_MAX]->id);
    }
    printf("\n");
    printf("waiting queue: ");
    for(int i = 0; i<waiting_queue.size; i++){
        printf("%d ,", waiting_queue.arr[(waiting_queue.head+i)%THREAD_MAX]->id);
    }
    printf("\n");
    printf("sleeping set: ");
    for(int i = 0; i<sleeping_set.size; i++){
        printf("%d ,", sleeping_set.threads[i]->id);
    }
    printf("\n");
}
