#include "threadtools.h"

// Please complete this three functions. You may refer to the macro function defined in "threadtools.h"

// Mountain Climbing
// You are required to solve this function via iterative method, instead of recursion.
void MountainClimbing(int thread_id, int number){
	/* Please fill this code section. */
    ThreadInit(thread_id, number);
    Current->w = 0, Current->x = 0, Current->y = 1, Current->z = 0;
    while (Current->w <= Current->N) {
        sleep(1);
        Current->w += 1;
        printf("Mountain Climbing: %d\n", Current->y);
        Current->z = Current->x + Current->y;
        Current->x = Current->y;
        Current->y = Current->z;
        ThreadYield();
    }
    sleep(1);
    ThreadYield();
    ThreadExit();
}

// Reduce Integer
// You are required to solve this function via iterative method, instead of recursion.
void ReduceInteger(int thread_id, int number){
	/* Please fill this code section. */
    ThreadInit(thread_id, number);
    Current->w = 0, Current->x = Current->N;
    if (Current->x == 1) {
        sleep(1);
        printf("Reduce Integer: %d\n", Current->w);
        ThreadYield();
        ThreadExit();
    }
    while (Current->x > 1) {
        sleep(1);
        if (Current->x % 2 == 0) Current->x /= 2;
        else if (Current->x == 3 || Current->x % 4 == 1) Current->x -= 1;
        else Current->x += 1;
        Current->w += 1;
        printf("Reduce Integer: %d\n", Current->w);
        ThreadYield();
    }
    sleep(1);
    ThreadYield();
    ThreadExit();
}

// Operation Count
// You are required to solve this function via iterative method, instead of recursion.
void OperationCount(int thread_id, int number){
	/* Please fill this code section. */
    ThreadInit(thread_id, number);
    Current->w = 0;
    Current->x = 0;
    Current->y = (Current->N % 2) ? 2 : 1;
    Current->z = Current->N / 2;
    if (Current->N == 1) {
        printf("Operation Count: %d\n", Current->w);
        ThreadYield();
        ThreadExit();
    }
    while(Current->x < Current->z) {
        sleep(1);
        Current->w += Current->y;
        Current->y += 2;
        Current->x += 1;
        printf("Operation Count: %d\n", Current->w);
        ThreadYield();
    }
    sleep(1);
    ThreadYield();
    ThreadExit();
}
