#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define valid(r, c, array) (r >= 0 && \
                    r < row_num && \
                    c >= 0 && \
                    c < col_num && \
                    array[r * (col_num + 1) + c] == 'O'\
                    )

int in_fd, out_fd;
FILE *in_fp, *out_fp;
int thread_num;
int row_num, col_num, board_num, epoch_num;
char **pre, **nxt;
int fd_len, base;
int *seg;
pthread_barrier_t *lk;

void *spawn_eval(void *arg) {
    const int idx = (int)(long long)arg;
    int seg_idx = seg[idx];
    int row = seg[idx] / (col_num + 1), col = seg[idx] % (col_num + 1), count = 0;
    for (int i = 0; i < epoch_num; i++) {
        seg_idx = seg[idx];
        if (i % 2) {
            while (seg_idx < seg[idx + 1]) {
                row = seg_idx / (col_num + 1), col = seg_idx % (col_num + 1), count = 0;
                if (valid((row + 1), (col), (*nxt))) count += 1;
                if (valid((row - 1), (col), (*nxt))) count += 1;
                if (valid((row), (col + 1), (*nxt))) count += 1;
                if (valid((row), (col - 1), (*nxt))) count += 1;
                if (valid((row + 1), (col + 1), (*nxt))) count += 1;
                if (valid((row + 1), (col - 1), (*nxt))) count += 1;
                if (valid((row - 1), (col + 1), (*nxt))) count += 1;
                if (valid((row - 1), (col - 1), (*nxt))) count += 1;
                if ((*nxt)[seg_idx] == 'O') (*pre)[seg_idx] = (count == 2 || count == 3) ? 'O' : '.';
                else if ((*nxt)[seg_idx] == '.') (*pre)[seg_idx] = (count == 3) ? 'O' : '.';
                else (*pre)[seg_idx] = (*nxt)[seg_idx];
                seg_idx += 1;
            }
        }
        else {
            while (seg_idx < seg[idx + 1]) {
                row = seg_idx / (col_num + 1), col = seg_idx % (col_num + 1), count = 0;
                if (valid((row + 1), (col), (*pre))) count += 1;
                if (valid((row - 1), (col), (*pre))) count += 1;
                if (valid((row), (col + 1), (*pre))) count += 1;
                if (valid((row), (col - 1), (*pre))) count += 1;
                if (valid((row + 1), (col + 1), (*pre))) count += 1;
                if (valid((row + 1), (col - 1), (*pre))) count += 1;
                if (valid((row - 1), (col + 1), (*pre))) count += 1;
                if (valid((row - 1), (col - 1), (*pre))) count += 1;
                if ((*pre)[seg_idx] == 'O') (*nxt)[seg_idx] = (count == 2 || count == 3) ? 'O' : '.';
                else if ((*pre)[seg_idx] == '.') (*nxt)[seg_idx] = (count == 3) ? 'O' : '.';
                else (*nxt)[seg_idx] = (*pre)[seg_idx];
                seg_idx += 1;
            }
        }
        pthread_barrier_wait(lk);
    }
}

void thread_mode() {
    long int delta;
    pthread_t tidp[thread_num];
    char *tmp;
    int ptr = 0;
    fscanf(in_fp, "%d%d%d\n", &row_num, &col_num, &epoch_num);
    pre = mmap(NULL, sizeof(char *),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    nxt = mmap(NULL, sizeof(char *),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *pre = mmap(NULL, (row_num * (col_num + 1) + 1) * sizeof(char),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *nxt = mmap(NULL, (row_num * (col_num + 1) + 1) * sizeof(char),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    seg = mmap(NULL, (thread_num + 1) * sizeof(int),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    lk = mmap(NULL, sizeof(pthread_barrier_t),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    while (((*pre)[ptr] = fgetc(in_fp)) != EOF) ++ptr;
    strcpy(*nxt, *pre);
    (*pre)[ptr] = 0;
    (*nxt)[ptr] = 0;
    seg[0] = 0;
    delta = ptr / thread_num;
    for (int i = 1; i < thread_num; i++) seg[i] = seg[i - 1] + delta;
    seg[thread_num] = ptr;
    pthread_barrier_init(lk, NULL, thread_num);
    for (int j = 0; j < thread_num; j++)
        pthread_create(&(tidp[j]), NULL, spawn_eval, (void *)(long long)j);
    for (int j = 0; j < thread_num; j++)
        pthread_join(tidp[j], NULL);
    if (epoch_num % 2) fputs(*nxt, out_fp);
    else fputs(*pre, out_fp);
}

void child_proc(const int idx) {
    int seg_idx = seg[idx];
    int row = seg[idx] / (col_num + 1), col = seg[idx] % (col_num + 1), count = 0;
    for (int i = 0; i < epoch_num; i++) {
        seg_idx = seg[idx];
        if (i % 2) {
            while (seg_idx < seg[idx + 1]) {
                row = seg_idx / (col_num + 1), col = seg_idx % (col_num + 1), count = 0;
                if (valid((row + 1), (col), (*nxt))) count += 1;
                if (valid((row - 1), (col), (*nxt))) count += 1;
                if (valid((row), (col + 1), (*nxt))) count += 1;
                if (valid((row), (col - 1), (*nxt))) count += 1;
                if (valid((row + 1), (col + 1), (*nxt))) count += 1;
                if (valid((row + 1), (col - 1), (*nxt))) count += 1;
                if (valid((row - 1), (col + 1), (*nxt))) count += 1;
                if (valid((row - 1), (col - 1), (*nxt))) count += 1;
                if ((*nxt)[seg_idx] == 'O') (*pre)[seg_idx] = (count == 2 || count == 3) ? 'O' : '.';
                else if ((*nxt)[seg_idx] == '.') (*pre)[seg_idx] = (count == 3) ? 'O' : '.';
                else (*pre)[seg_idx] = (*nxt)[seg_idx];
                seg_idx += 1;
            }
        }
        else {
            while (seg_idx < seg[idx + 1]) {
                row = seg_idx / (col_num + 1), col = seg_idx % (col_num + 1), count = 0;
                if (valid((row + 1), (col), (*pre))) count += 1;
                if (valid((row - 1), (col), (*pre))) count += 1;
                if (valid((row), (col + 1), (*pre))) count += 1;
                if (valid((row), (col - 1), (*pre))) count += 1;
                if (valid((row + 1), (col + 1), (*pre))) count += 1;
                if (valid((row + 1), (col - 1), (*pre))) count += 1;
                if (valid((row - 1), (col + 1), (*pre))) count += 1;
                if (valid((row - 1), (col - 1), (*pre))) count += 1;
                if ((*pre)[seg_idx] == 'O') (*nxt)[seg_idx] = (count == 2 || count == 3) ? 'O' : '.';
                else if ((*pre)[seg_idx] == '.') (*nxt)[seg_idx] = (count == 3) ? 'O' : '.';
                else (*nxt)[seg_idx] = (*pre)[seg_idx];
                seg_idx += 1;
            }
        }
        pthread_barrier_wait(lk);
    }
    exit(0);
}

void proc_mode() {
    long int delta;
    pthread_t tidp[thread_num];
    char *tmp;
    int ptr = 0;
    pthread_barrierattr_t lk_attr;
    fscanf(in_fp, "%d%d%d\n", &row_num, &col_num, &epoch_num);
    pre = mmap(NULL, sizeof(char *),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    nxt = mmap(NULL, sizeof(char *),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *pre = mmap(NULL, (row_num * (col_num + 1) + 1) * sizeof(char),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *nxt = mmap(NULL, (row_num * (col_num + 1) + 1) * sizeof(char),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    seg = mmap(NULL, (thread_num + 1) * sizeof(int),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    lk = mmap(NULL, sizeof(pthread_barrier_t),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    while (((*pre)[ptr] = fgetc(in_fp)) != EOF) ++ptr;
    strcpy(*nxt, *pre);
    (*pre)[ptr] = 0;
    (*nxt)[ptr] = 0;
    seg[0] = 0;
    delta = ptr / thread_num;
    for (int i = 1; i < thread_num; i++) seg[i] = seg[i - 1] + delta;
    seg[thread_num] = ptr;
    pthread_barrierattr_init(&lk_attr);
    pthread_barrierattr_setpshared(&lk_attr, 1);
    pthread_barrier_init(lk, &lk_attr, thread_num);
    for (int j = 0; j < thread_num; j++)
        pthread_create(&(tidp[j]), NULL, spawn_eval, (void *)(long long)j);
    for (int j = 0; j < thread_num; j++)
        pthread_join(tidp[j], NULL);
    if (epoch_num % 2) fputs(*nxt, out_fp);
    else fputs(*pre, out_fp);
}

int main(int argc, char *argv[]) {
    thread_num = atoi(argv[2]);
    in_fp = fopen(argv[3], "r");
    out_fp = fopen(argv[4], "w");
    if (!strcmp("-t", argv[1])) thread_mode();
    else proc_mode();
    return 0;
}
