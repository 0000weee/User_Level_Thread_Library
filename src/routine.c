#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_tool.h"

void idle(int id, int *args) {
    // TODO:: IDLE ^-^
    while (1) {
        printf("thread [%d]: idle\n", id);
        sleep(1);
        thread_yield();
    }
}

void fibonacci(int id, int *args) {
    thread_setup(id, args);

    current_thread->n = current_thread->args[0];
    for (current_thread->i = 1;; current_thread->i++) {
        if (current_thread->i <= 2) {
            current_thread->f_cur = 1;
            current_thread->f_prev = 1;
        } else {
            int f_next = current_thread->f_cur + current_thread->f_prev;
            current_thread->f_prev = current_thread->f_cur;
            current_thread->f_cur = f_next;
        }
        printf("thread %d: F_%d = %d\n", current_thread->id, current_thread->i, current_thread->f_cur);

        sleep(1);

        if (current_thread->i == current_thread->n) {
            thread_exit();
        } else {
            thread_yield();
        }
    }
}

void pm(int id, int *args) {
    thread_setup(id, args);

    // 從 args[0] 初始化 n 值
    current_thread->n = args[0];

    // 如果是第一次執行，初始化其他變數
    if (current_thread->i == 0) {
        current_thread->i = 1;  // 起始迭代次數
        current_thread->sum = 0; // 初始化累加結果
    }

    // 循環計算 pm(n)
    while (1) {
        // 計算 (-1)^(i-1) * i
        int term = ((current_thread->i % 2 == 1) ? 1 : -1) * current_thread->i;

        // 累加當前項
        current_thread->sum += term;

        // 輸出當前結果
        printf("thread %d: pm(%d) = %d\n", id, current_thread->i, current_thread->sum);

        sleep(1);

        // 決定是否退出或繼續
        if (current_thread->i == current_thread->n) {
            thread_exit();  // 結束執行緒
        } else {
            current_thread->i++;  // 更新迭代次數
            thread_yield();  // 暫時讓出執行緒
        }
    }
}


void enroll(int id, int *args) {
    // TODO:: enroll !! -^-
    thread_setup(id, args);

    sleep(1);

    if () {
        thread_exit();
    } else {
        thread_yield();
    }
}

