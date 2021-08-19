/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <signal.h>   /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
                         stack size) */
#include <stdio.h>    /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
                         stdout, stderr */
#include <stdlib.h>   /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
                         setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include "sthreads.h"

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/

#include <string.h> /* memset() */
#include <sys/time.h> /* ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF, struct itimerval, setitimer() */

#define TIMEOUT 50 //ms
#define TIMER_TYPE ITIMER_VIRTUAL // Type of timer

thread_t * head = 0x0 ;
thread_t * cursor = 0x0 ;
tid_t next_tid = 1 ;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/

int
timer_signal (int timer_type)
{
	int sig ;
	switch (timer_type) {
		case ITIMER_REAL :
			sig = SIGALRM ;
			break ;
		case ITIMER_VIRTUAL :
			sig = SIGVTALRM ;
			break ;
		case ITIMER_PROF :
			sig = SIGPROF ;
			break ;
		default :
			fprintf(stderr, "ERROR: unknown timer type %d!\n", timer_type) ;
			exit(EXIT_FAILURE) ;
	}
	return sig ;
}

void
timer_handler (int sig)
{
	if (sig == timer_signal(TIMER_TYPE))
		yield() ;
}

void
set_timer ()
{
	struct itimerval timer ;
	struct sigaction sa ;
	
	/* Install signal handler for the timer. */
	memset (&sa, 0, sizeof(sa)) ;
	sa.sa_handler = timer_handler ;
	sigaction (timer_signal(TIMER_TYPE), &sa, 0x0) ;

	/* Configure the timer to expire after ms mssec */
	timer.it_value.tv_sec = 0 ;
	timer.it_value.tv_usec = TIMEOUT * 1000 ;
	timer.it_interval.tv_sec = 0 ;
	timer.it_interval.tv_usec = 0 ;
	
	if (setitimer(TIMER_TYPE, &timer, 0x0) < 0) {
		perror("set_timer") ;
		exit(EXIT_FAILURE) ;
	}
}

void
disarm_timer ()
{
	struct itimerval timer ;
	timer.it_value.tv_sec = 0 ;
	timer.it_value.tv_usec = 0 ;
	timer.it_interval.tv_sec = 0 ;
	timer.it_interval.tv_usec = 0 ;

	if (setitimer(TIMER_TYPE, &timer, 0x0) < 0) {
		perror("disarm_timer") ;
		exit(EXIT_FAILURE) ;
	}
}

void
scheduler ()
{
	thread_t * curr = cursor ;
	for (cursor = cursor->next; cursor; cursor = cursor->next) {
		if (cursor->state == ready) {
			cursor->state = running ;
			set_timer() ;
			if (swapcontext(&curr->ctx, &cursor->ctx) < 0) {
				perror("swapcontext") ;
				exit(EXIT_FAILURE) ;
			}
			return ;
			//setcontext(&cursor->ctx) ;
			//perror("setcontext") ;
			//exit(EXIT_FAILURE) ;
		}
	}
	for (cursor = head; cursor != curr->next; cursor = cursor->next) {
		if (cursor->state == ready) {
			cursor->state = running ;
			set_timer() ;
			if (swapcontext(&curr->ctx, &cursor->ctx) < 0) {
				perror("swapcontext") ;
				exit(EXIT_FAILURE) ;
			}
			return ;
			//setcontext(&cursor->ctx) ;
			//perror("setcontext") ;
			//exit(EXIT_FAILURE) ;
		}
	}
	perror("No ready threads!") ;
	exit(EXIT_FAILURE) ;
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/


int init(){
	if (head != 0x0)
		return -1 ;

	head = (thread_t *)malloc(sizeof(thread_t)) ;
	if (head == 0x0) {
		perror("Allocating head") ;
		exit(EXIT_FAILURE) ;
	}
	head->tid = next_tid++ ;
	head->state = running ;
	if (getcontext(&head->ctx) < 0) {
		perror("getcontext") ;
		exit(EXIT_FAILURE) ;
	}
	head->next = 0x0 ;
	cursor = head ;
	set_timer() ;
	return 1;
}

tid_t spawn(void (*start)()){
	disarm_timer() ;

	thread_t * spawn = (thread_t *)malloc(sizeof(thread_t)) ;
	if (spawn == 0x0) {
		perror("spawn") ;
		exit(EXIT_FAILURE) ;
	}
	
	void * stack = malloc(STACK_SIZE) ;
	if (stack == 0x0) {
		perror("Allocating stack") ;
		exit(EXIT_FAILURE) ;
	}

	if (getcontext(&spawn->ctx) < 0) {
		perror("getcontext") ;
		exit(EXIT_FAILURE) ;
	}

	spawn->ctx.uc_link = 0 ;//&cursor->ctx ;
	spawn->ctx.uc_stack.ss_sp = stack ;
	spawn->ctx.uc_stack.ss_size = STACK_SIZE ;
	spawn->ctx.uc_stack.ss_flags = 0 ;

	makecontext(&spawn->ctx, start, 0) ;

	spawn->tid = next_tid++ ;
	spawn->state = ready ;
	spawn->next = head ;
	head = spawn ;

	set_timer() ;
	return spawn->tid ;
}

void yield(){
	disarm_timer() ;
	if (cursor->state == running) {
		cursor->state = ready ;
		/*if (getcontext(&cursor->ctx) < 0) {
			perror("getcontext") ;
			exit(EXIT_FAILURE) ;
		}
		if (cursor->state == running)
			return ;*/
	} else {
		perror("yield") ;
		exit(EXIT_FAILURE) ;
	}
	
	scheduler() ;
}

void  done(){
	disarm_timer() ;
	int flag = 0 ;
	while (!flag) {
		for (thread_t * iter = head; iter; iter = iter->next) {
			if (iter->state == waiting) {
				iter->state = ready ;
				flag = 1 ;
			}
		}
		if (flag)
			break ;
		else
			yield() ;
	}
	if (cursor->state == running) {
		cursor->state = terminated ;
	} else {
		perror("done") ;
		exit(EXIT_FAILURE) ;
	}

	scheduler() ;
}

tid_t join() {
	disarm_timer() ;
	if (cursor->state == running) {
		cursor->state = waiting ;
	} else {
		perror("join") ;
		exit(EXIT_FAILURE) ;
	}
	scheduler() ;
	tid_t ret = -1 ;
	thread_t * iter = head ;
	thread_t * prev = 0x0 ;
	for (iter = head; iter; prev = iter, iter = iter->next) {
		if (iter->state == terminated) {
			ret = iter->tid ;
			if (prev == 0x0)
				head = head->next ;
			else
				prev->next = iter->next ;
			free(iter->ctx.uc_stack.ss_sp) ;
			free(iter) ;
			break ;
		}
	}
	return ret ;
}
