#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_tool.h"

void idle(int id) {
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
    // Step 1: 初始化執行緒，並獲取參數
    thread_setup(id, args);

    int d_p = args[0];  // Desire for pj_class
    int d_s = args[1];  // Desire for sw_class
    int s = args[2];    // Sleep time
    int b = args[3];    // Best friend's ID

    // Step 1: 模擬 oversleeping
    printf("thread %d: sleep %d\n", id, s);
    thread_sleep(s);

    // Step 2: 喚醒好友並讀取課程剩餘名額
    thread_awake(b);
    read_lock();
    printf("thread %d: acquire read lock\n", id);

    int remaining_pj = q_p;  // 剩餘的 pj_class 名額
    int remaining_sw = q_s;  // 剩餘的 sw_class 名額

    sleep(1);
    thread_yield();

    // Step 3: 釋放讀鎖並計算優先值
    read_unlock();

    int p_p = d_p * remaining_pj;  // Priority for pj_class
    int p_s = d_s * remaining_sw;  // Priority for sw_class
    printf("thread %d: release read lock, p_p = %d, p_s = %d\n", id, p_p, p_s);

    sleep(1);
    thread_yield();

    // Step 4: 獲取寫鎖並嘗試報名
    write_lock();

    const char *enroll_class = NULL;  // 要報名的課程名稱
    if (p_p > p_s || (p_p == p_s && d_p > d_s)) {
        // 優先報名 pj_class
        if (q_p > 0) {
            q_p--;  // 減少剩餘名額
            enroll_class = "pj_class";
        } else if (q_s > 0) {
            q_s--;
            enroll_class = "sw_class";
        }
    } else {
        // 優先報名 sw_class
        if (q_s > 0) {
            q_s--;  // 減少剩餘名額
            enroll_class = "sw_class";
        } else if (q_p > 0) {
            q_p--;
            enroll_class = "pj_class";
        }
    }

    printf("thread %d: acquire write lock, enroll in %s\n", id, enroll_class);

    sleep(1);
    thread_yield();

    // Step 5: 釋放寫鎖並退出
    write_unlock();
    printf("thread %d: release write lock\n", id);

    sleep(1);
    thread_exit();
}


