#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()

/*******************************************************************************
                   Functions to be used together with spawn()

    You may add your own functions or change these functions to your liking.
********************************************************************************/

#define TEST 3

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers_timeout() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
  }
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters_timeout() {
  char c = 'a';

  while (true) {
      printf(" c = %c\n", c);
      c = (c == 'z') ? 'a' : c + 1;
	}
}

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers_yield() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
	yield() ;
  }
}

/* Prints the sequence a, b, c, ..., z over and over again. Yield
 */
void letters_yield() {
  char c = 'a';

  while (true) {
  	printf(" c = %c\n", c);
  	c = (c == 'z') ? 'a' : c + 1;
		yield() ;
	}
}

/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
    if (n > 3) done();
    yield();
  }
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
  char c = 'a';

  while (true) {
      printf(" c = %c\n", c);
      if (c == 'f') done();
      yield();
      c = (c == 'z') ? 'a' : c + 1;
    }
}

/* Calculates the nth Fibonacci number using recursion.
 */
int fib(int n) {
  switch (n) {
  case 0:
    return 0;
  case 1:
    return 1;
  default:
    return fib(n-1) + fib(n-2);
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This is deliberately an unnecessary slow and CPU intensive
   implementation where each number in the sequence is calculated recursively
   from scratch.
*/

void fibonacci_slow() {
  int n = 0;
  int f;
  while (true) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf(" fib_slow(%02d) = %d\n", n, fib(n));
    n = (n + 1) % INT_MAX;
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
  int a = 0;
  int b = 1;
  int n = 0;
  int next = a + b;

  while(true) {
    printf(" fib_fast(%02d) = %d\n", n, a);
    next = a + b;
    a = b;
    b = next;
    n++;
    if (a < 0) {
      // Restart on overflow.
      a = 0;
      b = 1;
      n = 0;
    }
  }
}

/* Prints the sequence of magic constants over and over again.

   https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
  int n = 3;
  int m;
  while (true) {
    m = (n*(n*n+1)/2);
    if (m > 0) {
      printf(" magic(%d) = %d\n", n, m);
      n = (n+1) % INT_MAX;
    } else {
      // Start over when m overflows.
      n = 3;
    }
    yield();
  }
}

/*******************************************************************************
                                     main()

            Here you should add code to test the Simple Threads API.
********************************************************************************/


int main(){
  puts("\n==== Test program for the Simple Threads API ====\n");

  init(); // Initialization

#if TEST==1 // done&join test
  spawn(numbers) ;
  spawn(numbers) ;
  spawn(numbers) ;
	
	int a = join() ;
	printf("join %d\n", a) ;
	yield() ;
	yield() ;
	int b = join() ;
	printf("join %d\n", b) ;
	int c = join() ;
	printf("join %d\n", c) ;

#elif TEST==2 // cooperative scheduling test
	spawn(numbers_yield) ;
	spawn(letters_yield) ;
	
	while(1) {
		printf("main\n") ;
		yield() ;
	}

#elif TEST==3 // preemptive scheduling test
	spawn(numbers_timeout) ;
	spawn(letters_timeout) ;
	spawn(fibonacci_fast) ;
	
	while(1)
		printf("main\n") ;

#endif
}
