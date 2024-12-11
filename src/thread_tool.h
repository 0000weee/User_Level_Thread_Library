#ifndef THREAD_TOOL_H
#define THREAD_TOOL_H

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

// The maximum number of threads.
#define THREAD_MAX 100

#define JUMP_FROM_THREAD_YIELD 1
#define JUMP_FROM_SIGNAL_HANDLER 2
#define JUMP_FROM_SCHEDULER 3
#define JUMP_FROM_LOCK 4
#define JUMP_FROM_SLEEP 5
#define JUMP_FROM_EXIT 6
void sighandler(int signum);
void scheduler();

// The thread control block structure.
struct tcb {
    int id;
    int *args;
    // Reveals what resource the thread is waiting for. The values are:
    //  - 0: no resource.
    //  - 1: read lock.
    //  - 2: write lock.
    int waiting_for;
    int sleeping_time;
    jmp_buf env;  // Where the scheduler should jump to.
    int n, i, f_cur, f_prev; // TODO: Add some variables you wish to keep between switches.
};

// The only one thread in the RUNNING state.
extern struct tcb *current_thread;
extern struct tcb *idle_thread;

struct tcb_queue {
    struct tcb *arr[THREAD_MAX];  // The circular array.
    int head;                     // The index of the head of the queue
    int size;
};

extern struct tcb_queue ready_queue, waiting_queue;


// The rwlock structure.
//
// When a thread acquires a type of lock, it should increment the corresponding count.
struct rwlock {
    int read_count;
    int write_count;
};

extern struct rwlock rwlock;

// The remaining spots in classes.
extern int q_p, q_s;

// The maximum running time for each thread.
extern int time_slice;

// The long jump buffer for the scheduler.
extern jmp_buf sched_buf;

void enqueue(struct tcb_queue queue, struct tcb *thread){
    int idx = (queue->head + queue->size) % THREAD_MAX;
    queue->arr[idx] = thread;
    queue->size++;
}

struct tcb* enqueue(struct tcb_queue queue, struct tcb *thread){
    if(queue->size > 0){
        queue->head++;
        queue->size--;
        return queue->arr[head-1];
    }else{
        return NULL;
    }
    
}

// TODO::
// You should setup your own sleeping set as well as finish the marcos below
#define thread_create(func, t_id, t_args)                                              \
    ({                                                                                 \
        func(t_id, t_args);                                                            \
    })

#define thread_setup(t_id, t_args)                                          \
    ({                                                                      \
        struct tcb* new_tcb = (struct tcb*) calloc(1, sizeof(struct tcb));  \
        new_tcb->id = t_id;                                                 \
        new_tcb->args = t_args;                                             \
                                                                            \
        int jmpVal = setjmp(new_tcb->env);                                  \
        if (jmpVal = 0){                                                    \
            if (t_id == 0) {                                                \
                idle_thread = new_tcb;                                      \
                return;                                                     \
            } else {                                                        \
                enqueue(ready_queue, new_tcb);                            \
                return;                                                     \
            }                                                               \
        }                                                                   \
    })

#define thread_yield()                                              \
    ({                                                              \
        int jmpVal = sigsetjmp(current_thread->env, 1);             \
        sigset_t sigset;                                            \
        sigset_t oldset;                                            \
                                                                    \
        /* Initialize signal set */                                 \
        sigemptyset(&sigset);                                       \
        sigaddset(&sigset, SIGTSTP);                                \
        sigaddset(&sigset, SIGALRM);                                \
                                                                    \
        /* Unblock SIGTSTP */                                       \
        sigprocmask(SIG_UNBLOCK, &sigset, &oldset);                 \
                                                                    \
        /* Block SIGTSTP  */                                        \
        sigprocmask(SIG_BLOCK, &sigset, NULL);                      \
                                                                    \
        /* Unblock SIGALRM */                                       \
        sigdelset(&sigset, SIGTSTP);                                \
        sigprocmask(SIG_UNBLOCK, &sigset, NULL);                    \
                                                                    \
        /* Block SIGALRM */                                         \
        sigaddset(&sigset, SIGALRM);                                \
        sigprocmask(SIG_BLOCK, &sigset, NULL);                      \
                                                                    \
        /*Relinquishes control to the scheduler*/                   \
        if (jmpVal == 0){                                           \
            siglongjmp(sched_buf, JUMP_FROM_THREAD_YIELD);          \
        }                                                           \        
    })


#define read_lock()                                                 \
    ({                                                              \
        setjmp(current_thread->env);                                \
        while(1){                                                   \
            if (rwlock.write_count > 0){                            \
                enqueue(waiting_queue, current_thread);           \
                thread_yield();                                     \
            }                                                       \
            else{                                                   \
                rwlock.read_count += 1;                             \
                break;                                              \
            }                                                       \
        }                                                           \
    })                                                              

#define write_lock()                                                \
    ({                                                              \
        setjmp(current_thread->env);                                \
        while(1){                                                   \
            if (rwlock.write_count > 0 || rwlock.read_count > 0){   \
                enqueue(waiting_queue, current_thread);           \
                thread_yield();                                     \
            }                                                       \
            else{                                                   \
                rwlock.write_count += 1;                            \
                break;                                              \
            }                                                       \
        }                                                           \                                                        
    })

#define read_unlock()                                               \
    ({                                                              \
        rwlock.read_count -= 1;                                     \
    })

#define write_unlock()                                              \
    ({                                                              \
        rwlock.write_count -= 1;                                    \
    })

struct sleeping_set {
    struct tcb* threads[THREAD_MAX];
    int size;
};

extern struct sleeping_set sleeping_set = {.size = 0};

/* Add a thread to the sleeping_set */
void add_to_sleeping_set(struct tcb* thread) {
    if (sleeping_set.size < THREAD_MAX) {
        sleeping_set.threads[sleeping_set.size++] = thread;
    }
}

/* Remove a thread from the sleeping_set */
void remove_from_sleeping_set(struct tcb* thread) {
    for (int i = 0; i < sleeping_set.size; i++) {
        if (sleeping_set.threads[i] == thread) {
            for (int j = i; j < sleeping_set.size - 1; j++) {
                sleeping_set.threads[j] = sleeping_set.threads[j + 1];
            }
            sleeping_set.threads[--sleeping_set.size] = NULL;
            return;
        }
    }
}


#define thread_sleep(sec)                                           \
    ({                                                                  \
        if (sec < 1 || sec > 10) {                                      \
            printf("Invalid sleep time: %d\n", sec);                    \
            return;                                                     \
        }                                                               \
                                                                        \
        /* Convert sleep seconds to simulated time slices */            \
        current_thread->sleeping_time = sec;                            \
                                                                        \
        /* Add to sleeping_set */                                       \
        add_to_sleeping_set(current_thread);                            \
                                                                        \
        /* Yield control to scheduler */                                \
        thread_yield();                                                 \
    })



#define thread_awake(t_id)                                          \
    ({                                                              \
        struct tcb* thread = NULL;                                  \
                                                                    \
        /* Find thread in sleeping_set */                           \
        for (int i = 0; i < sleeping_set.size; i++) {               \
            if (sleeping_set.threads[i]->id == t_id) {              \
                thread = sleeping_set.threads[i];                   \
                break;                                              \
            }                                                       \
        }                                                           \
                                                                    \
        /* If thread is found, wake it up */                        \
        if (thread) {                                               \
            thread->sleeping_time = 0;                              \
            remove_from_sleeping_set(thread);                       \
            enqueue(ready_queue, thread);                         \
        }                                                           \
    })


#define thread_exit()                                               \
    ({                                                              \
        printf("thread [%d]: exit\n", current_thread->id);          \
                                                                    \
        /* Jump to scheduler using longjmp */                       \
        longjmp(scheduler_env, 1);                                  \
    })


#endif  // THREAD_TOOL_H
