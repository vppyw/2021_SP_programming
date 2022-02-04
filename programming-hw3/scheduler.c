#include "threadtools.h"

/*
1) You should state the signal you received by: printf('TSTP signal caught!\n') or printf('ALRM signal caught!\n')
2) If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
3) You should longjmp(SCHEDULER,1) once you're done.
*/
void sighandler(int signo){
	/* Please fill this code section. */
    if (signo == SIGTSTP) {
        printf("TSTP signal caught!\n");
        longjmp(Current->Environment, 1);
    }
    else if (signo == SIGALRM) {
        printf("ALRM signal caught!\n");
        alarm(timeslice);
        longjmp(Current->Environment, 1);
    }
}

/*
1) You are stronly adviced to make 
	setjmp(SCHEDULER) = 1 for ThreadYield() case
	setjmp(SCHEDULER) = 2 for ThreadExit() case
2) Please point the Current TCB_ptr to correct TCB_NODE
3) Please maintain the circular linked-list here
*/
void scheduler(){
	/* Please fill this code section. */
    int getjmp;
    getjmp = setjmp(SCHEDULER);
    if (getjmp == 1) {
        Current = Current->Next;
        longjmp(Current->Environment, 1);
    }
    else if (getjmp == 2) {
        if (Current->Next == Current) {
            free(Current);
            Head = NULL, Current = NULL;
            longjmp(MAIN, 1);
        }
        else {
            if (Head == Current)
                Head = Head->Next;
            Work = Current;
            Current = Current->Next;
            Work->Prev->Next = Current;
            Current->Prev = Work->Prev;
            free(Work);
        }
        longjmp(Current->Environment, 1);
    }
    else if (getjmp == 0) {
        Current = Head;
        longjmp(Current->Environment, 1);
    }
}
