#ifndef THREAD_TOOL_H
#define THREAD_TOOL_H

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

// The maximum number of threads.
#define THREAD_MAX 100


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

// TODO::
// You should setup your own sleeping set as well as finish the marcos below
#define thread_create(func, t_id, t_args)                                              \
    ({                                                                                 \
    })

#define thread_setup(t_id, t_args)                                                     \
    ({                                                                                 \
    })

#define thread_yield()                                  \
    ({                                                  \
    })

#define read_lock()                                                      \
    ({                                                                   \
    })

#define write_lock()                                                     \
    ({                                                                   \
    })

#define read_unlock()                                                                 \
    ({                                                                                \
    })

#define write_unlock()                                                                \
    ({                                                                                \
    })

#define thread_sleep(sec)                                            \
    ({                                                               \
    })

#define thread_awake(t_id)                                                        \
    ({                                                                            \
    })

#define thread_exit()                                    \
    ({                                                   \
    })

#endif  // THREAD_TOOL_H
