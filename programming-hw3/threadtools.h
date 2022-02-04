#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice, switchmode;

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE{
    jmp_buf  Environment;
    int      Thread_id;
    TCB_ptr  Next;
    TCB_ptr  Prev;
    int i, N;
    int w, x, y, z;
} TCB;

extern jmp_buf MAIN, SCHEDULER;
extern TCB_ptr Head;
extern TCB_ptr Current;
extern TCB_ptr Work;
extern sigset_t base_mask, waiting_mask, tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();

// Call function in the argument that is passed in
#define ThreadCreate(function, thread_id, number)\
{\
	/* Please fill this code section. */\
    if (!setjmp(MAIN)) function(thread_id, number);\
}

// Build up TCB_NODE for each function, insert it into circular linked-list
#define ThreadInit(thread_id, number)\
{\
	/* Please fill this code section. */ \
    Current = (TCB *)malloc(sizeof(TCB));\
    Current->Thread_id = thread_id;\
    Current->N = number;\
    if (Head == NULL) {\
        Current->Prev = Current, Current->Next = Current;\
        Head = Current;\
    }\
    else if (Head->Next == Head) {\
        Current->Next = Head;\
        Current->Prev = Head;\
        Head->Next = Current;\
        Head->Prev = Current;\
    }\
    else {\
        Current->Next = Head;\
        Current->Prev = Head->Prev;\
        Head->Prev->Next = Current;\
        Head->Prev = Current;\
    }\
    if (!setjmp(Current->Environment)) longjmp(MAIN, 1);\
}

// Call this while a thread is terminated
#define ThreadExit()\
{\
	/* Please fill this code section. */\
    longjmp(SCHEDULER, 2);\
}

// Decided whether to "context switch" based on the switchmode argument passed in main.c
#define ThreadYield()\
{\
	/* Please fill this code section. */\
    if (!switchmode) {\
        if (!setjmp(Current->Environment))\
            longjmp(SCHEDULER, 1);\
    }\
    else {\
        if (!setjmp(Current->Environment)) {\
            sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL);\
            sigprocmask(SIG_BLOCK, &tstp_mask, NULL);\
            if (!setjmp(Current->Environment)) {\
                sigprocmask(SIG_UNBLOCK, &alrm_mask, NULL);\
                sigprocmask(SIG_BLOCK, &alrm_mask, NULL);\
            }\
            else {\
                if (!setjmp(Current->Environment)) {\
                    longjmp(SCHEDULER, 1);\
                }\
            }\
        }\
        else {\
            if (!setjmp(Current->Environment)) {\
                longjmp(SCHEDULER, 1);\
            }\
        }\
    }\
}
