#line 270 "../../../../../splash2/codes//null_macros/c.m4.null"

#line 1 "lu.C"
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/*  Parallel dense blocked LU factorization (no pivoting)                */
/*                                                                       */
/*  This version contains one dimensional arrays in which the matrix     */
/*  to be factored is stored.                                            */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -nN : Decompose NxN matrix.                                          */
/*  -pP : P = number of processors.                                      */
/*  -bB : Use a block size of B. BxB elements should fit in cache for    */
/*        good performance. Small block sizes (B=8, B=16) work well.     */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Test output.                                                   */
/*  -o  : Print out matrix values.                                       */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#line 42
#include <pthread.h>
#line 42
#include <sys/time.h>
#line 42
#include <unistd.h>
#line 42
#include <stdlib.h>
#line 42
#include "TriggerAction.h"
#line 42

#line 42
pthread_t PThreadTable[64];
#line 42

#line 42


#define MAXRAND					32767.0
#define DEFAULT_N				128
#define DEFAULT_P				1
#define DEFAULT_B				16
#define min(a,b) ((a) < (b) ? (a) : (b))
#define PAGE_SIZE				4096

struct GlobalMemory {
  double *t_in_fac;
  double *t_in_solve;
  double *t_in_mod;
  double *t_in_bar;
  double *completion;
  unsigned long starttime;
  unsigned long rf;
  unsigned long rs;
  unsigned long done;
  long id;
  
#line 62
struct {
#line 62
	pthread_mutex_t	mutex;
#line 62
	pthread_cond_t	cv;
#line 62
	unsigned long	counter;
#line 62
	unsigned long	cycle;
#line 62
} (start);
#line 62

  pthread_mutex_t (idlock);
} *Global;

struct LocalCopies {
  double t_in_fac;
  double t_in_solve;
  double t_in_mod;
  double t_in_bar;
};

long n = DEFAULT_N;          /* The size of the matrix */
long P = DEFAULT_P;          /* Number of processors */
long block_size = DEFAULT_B; /* Block dimension */
long nblocks;                /* Number of blocks in each dimension */
long num_rows;               /* Number of processors per row of processor grid */
long num_cols;               /* Number of processors per col of processor grid */
double *a;                   /* a = lu; l and u both placed back in a */
double *rhs;
long *proc_bytes;            /* Bytes to malloc per processor to hold blocks of A*/
long test_result = 0;        /* Test result of factorization? */
long doprint = 0;            /* Print out matrix values? */
long dostats = 0;            /* Print out individual processor statistics? */

void SlaveStart(void *);
void OneSolve(long n, long block_size, long MyNum, long dostats);
void lu0(double *a, long n, long stride);
void bdiv(double *a, double *diag, long stride_a, long stride_diag, long dimi, long dimk);
void bmodd(double *a, double *c, long dimi, long dimj, long stride_a, long stride_c);
void bmod(double *a, double *b, double *c, long dimi, long dimj, long dimk, long stride);
void daxpy(double *a, double *b, long n, double alpha);
long BlockOwner(long I, long J);
long BlockOwnerColumn(long I, long J);
long BlockOwnerRow(long I, long J);
void lu(long n, long bs, long MyNum, struct LocalCopies *lc, long dostats);
void InitA(double *rhs);
double TouchA(long bs, long MyNum);
void PrintA(void);
void CheckResult(long n, double *a, double *rhs);
void printerr(char *s);

int main(int argc, char *argv[])
{
/*************************** For CI evaluation *******************************/
#ifdef LIBFIBER
  fiber_manager_init(1); /* Use 1 thread */
#endif
  init_stats(0);
  struct timeval  rt_begin, rt_end;
  gettimeofday(&rt_begin, NULL);
/*****************************************************************************/
  long i, ch;
  extern char *optarg;
  double mint, maxt, avgt;
  double min_fac, min_solve, min_mod, min_bar;
  double max_fac, max_solve, max_mod, max_bar;
  double avg_fac, avg_solve, avg_mod, avg_bar;
  unsigned long start;

  {
#line 121
	struct timeval	FullTime;
#line 121

#line 121
	gettimeofday(&FullTime, NULL);
#line 121
	(start) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 121
};

  while ((ch = getopt(argc, argv, "n:p:b:cstoh")) != -1) {
    switch(ch) {
    case 'n': n = atoi(optarg); break;
    case 'p': P = atoi(optarg); break;
    case 'b': block_size = atoi(optarg); break;
    case 's': dostats = 1; break;
    case 't': test_result = !test_result; break;
    case 'o': doprint = !doprint; break;
    case 'h': printf("Usage: LU <options>\n\n");
              printf("options:\n");
              printf("  -nN : Decompose NxN matrix.\n");
              printf("  -pP : P = number of processors.\n");
              printf("  -bB : Use a block size of B. BxB elements should fit in cache for \n");
              printf("        good performance. Small block sizes (B=8, B=16) work well.\n");
              printf("  -c  : Copy non-locally allocated blocks to local memory before use.\n");
              printf("  -s  : Print individual processor timing statistics.\n");
              printf("  -t  : Test output.\n");
              printf("  -o  : Print out matrix values.\n");
              printf("  -h  : Print out command line options.\n\n");
              printf("Default: LU -n%1d -p%1d -b%1d\n",
                     DEFAULT_N,DEFAULT_P,DEFAULT_B);
              exit(0);
              break;
    }
  }

  {
#line 149
    }

  printf("\n");
  printf("Blocked Dense LU Factorization\n");
  printf("     %ld by %ld Matrix\n",n,n);
  printf("     %ld Processors\n",P);
  printf("     %ld by %ld Element Blocks\n",block_size,block_size);
  printf("\n");
  printf("\n");

  num_rows = (long) sqrt((double) P);
  for (;;) {
    num_cols = P/num_rows;
    if (num_rows*num_cols == P)
      break;
    num_rows--;
  }
  nblocks = n/block_size;
  if (block_size * nblocks != n) {
    nblocks++;
  }

  a = (double *) malloc(n*n*sizeof(double));;
  if (a == NULL) {
	  printerr("Could not malloc memory for a.\n");
	  exit(-1);
  }
  rhs = (double *) malloc(n*sizeof(double));;
  if (rhs == NULL) {
	  printerr("Could not malloc memory for rhs.\n");
	  exit(-1);
  }

  Global = (struct GlobalMemory *) malloc(sizeof(struct GlobalMemory));;
  Global->t_in_fac = (double *) malloc(P*sizeof(double));;
  Global->t_in_mod = (double *) malloc(P*sizeof(double));;
  Global->t_in_solve = (double *) malloc(P*sizeof(double));;
  Global->t_in_bar = (double *) malloc(P*sizeof(double));;
  Global->completion = (double *) malloc(P*sizeof(double));;

  if (Global == NULL) {
    printerr("Could not malloc memory for Global\n");
    exit(-1);
  } else if (Global->t_in_fac == NULL) {
    printerr("Could not malloc memory for Global->t_in_fac\n");
    exit(-1);
  } else if (Global->t_in_mod == NULL) {
    printerr("Could not malloc memory for Global->t_in_mod\n");
    exit(-1);
  } else if (Global->t_in_solve == NULL) {
    printerr("Could not malloc memory for Global->t_in_solve\n");
    exit(-1);
  } else if (Global->t_in_bar == NULL) {
    printerr("Could not malloc memory for Global->t_in_bar\n");
    exit(-1);
  } else if (Global->completion == NULL) {
    printerr("Could not malloc memory for Global->completion\n");
    exit(-1);
  }

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the a
   matrix data across physically distributed memories in a 
   round-robin fashion as desired. */

  {
#line 213
	unsigned long	Error;
#line 213

#line 213
	Error = pthread_mutex_init(&(Global->start).mutex, NULL);
#line 213
	if (Error != 0) {
#line 213
		printf("Error while initializing barrier.\n");
#line 213
		exit(-1);
#line 213
	}
#line 213

#line 213
	Error = pthread_cond_init(&(Global->start).cv, NULL);
#line 213
	if (Error != 0) {
#line 213
		printf("Error while initializing barrier.\n");
#line 213
		pthread_mutex_destroy(&(Global->start).mutex);
#line 213
		exit(-1);
#line 213
	}
#line 213

#line 213
	(Global->start).counter = 0;
#line 213
	(Global->start).cycle = 0;
#line 213
};
  {pthread_mutex_init(&(Global->idlock), NULL);};
  Global->id = 0;

  InitA(rhs);
  if (doprint) {
    printf("Matrix before decomposition:\n");
    PrintA();
  }

/*************************** For CI evaluation *******************************/
  {
#line 224
	long	i, Error;
#line 224

#line 224
  unsigned int thrIndex = 1; 
#line 224
	for (i = 0; i < (P) - 1; i++) {
#line 224
		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))(SlaveStart), (void*)thrIndex);
#line 224
    thrIndex++;
#line 224
		if (Error != 0) {
#line 224
			printf("Error in pthread_create().\n");
#line 224
			exit(-1);
#line 224
		}
#line 224
	}
#line 224
	SlaveStart((void*)0);
#line 224
};
/*****************************************************************************/
  {
#line 226
	unsigned long	i, Error;
#line 226
	for (i = 0; i < (P) - 1; i++) {
#line 226
		Error = pthread_join(PThreadTable[i], NULL);
#line 226
		if (Error != 0) {
#line 226
			printf("Error in pthread_join().\n");
#line 226
			exit(-1);
#line 226
		}
#line 226
	}
#line 226
};

  if (doprint) {
    printf("\nMatrix after decomposition:\n");
    PrintA();
  }

  if (dostats) {
    maxt = avgt = mint = Global->completion[0];
    for (i=1; i<P; i++) {
      if (Global->completion[i] > maxt) {
        maxt = Global->completion[i];
      }
      if (Global->completion[i] < mint) {
        mint = Global->completion[i];
      }
      avgt += Global->completion[i];
    }
    avgt = avgt / P;

    min_fac = max_fac = avg_fac = Global->t_in_fac[0];
    min_solve = max_solve = avg_solve = Global->t_in_solve[0];
    min_mod = max_mod = avg_mod = Global->t_in_mod[0];
    min_bar = max_bar = avg_bar = Global->t_in_bar[0];

    for (i=1; i<P; i++) {
      if (Global->t_in_fac[i] > max_fac) {
        max_fac = Global->t_in_fac[i];
      }
      if (Global->t_in_fac[i] < min_fac) {
        min_fac = Global->t_in_fac[i];
      }
      if (Global->t_in_solve[i] > max_solve) {
        max_solve = Global->t_in_solve[i];
      }
      if (Global->t_in_solve[i] < min_solve) {
        min_solve = Global->t_in_solve[i];
      }
      if (Global->t_in_mod[i] > max_mod) {
        max_mod = Global->t_in_mod[i];
      }
      if (Global->t_in_mod[i] < min_mod) {
        min_mod = Global->t_in_mod[i];
      }
      if (Global->t_in_bar[i] > max_bar) {
        max_bar = Global->t_in_bar[i];
      }
      if (Global->t_in_bar[i] < min_bar) {
        min_bar = Global->t_in_bar[i];
      }
      avg_fac += Global->t_in_fac[i];
      avg_solve += Global->t_in_solve[i];
      avg_mod += Global->t_in_mod[i];
      avg_bar += Global->t_in_bar[i];
    }
    avg_fac = avg_fac/P;
    avg_solve = avg_solve/P;
    avg_mod = avg_mod/P;
    avg_bar = avg_bar/P;
  }
  printf("                            PROCESS STATISTICS\n");
  printf("              Total      Diagonal     Perimeter      Interior       Barrier\n");
  printf(" Proc         Time         Time         Time           Time          Time\n");
  printf("    0    %10.0f    %10.0f    %10.0f    %10.0f    %10.0f\n",
          Global->completion[0],Global->t_in_fac[0],
          Global->t_in_solve[0],Global->t_in_mod[0],
          Global->t_in_bar[0]);
  if (dostats) {
    for (i=1; i<P; i++) {
      printf("  %3ld    %10.0f    %10.0f    %10.0f    %10.0f    %10.0f\n",
              i,Global->completion[i],Global->t_in_fac[i],
              Global->t_in_solve[i],Global->t_in_mod[i],
              Global->t_in_bar[i]);
    }
    printf("  Avg    %10.0f    %10.0f    %10.0f    %10.0f    %10.0f\n",
           avgt,avg_fac,avg_solve,avg_mod,avg_bar);
    printf("  Min    %10.0f    %10.0f    %10.0f    %10.0f    %10.0f\n",
           mint,min_fac,min_solve,min_mod,min_bar);
    printf("  Max    %10.0f    %10.0f    %10.0f    %10.0f    %10.0f\n",
           maxt,max_fac,max_solve,max_mod,max_bar);
  }
  printf("\n");
  Global->starttime = start;
  printf("                            TIMING INFORMATION\n");
  printf("Start time                        : %16lu\n", Global->starttime);
  printf("Initialization finish time        : %16lu\n", Global->rs);
  printf("Overall finish time               : %16lu\n", Global->rf);
  printf("Total time with initialization    : %16lu\n", Global->rf-Global->starttime);
  printf("Total time without initialization : %16lu\n", Global->rf-Global->rs);
  printf("\n");

  if (test_result) {
    printf("                             TESTING RESULTS\n");
    CheckResult(n, a, rhs);
  }

/*************************** For CI evaluation *******************************/
   gettimeofday(&rt_end, NULL);
   uint64_t run_time = (rt_end.tv_sec - rt_begin.tv_sec)*1000000 + (rt_end.tv_usec - rt_begin.tv_usec);
   print_timing_stats();
   printf("lu-nc runtime: %lu usec\n", run_time);  
/*****************************************************************************/

  {
#line 329
    exit(0);};
}

void SlaveStart(void *vpIndex)
{
/*************************** For CI evaluation *******************************/
  int idx = (int)vpIndex;
  {
#line 336
    init_stats(idx);
#line 336
    };
/*****************************************************************************/
  long MyNum;

  {pthread_mutex_lock(&(Global->idlock));}
    MyNum = Global->id;
    Global->id ++;
  {pthread_mutex_unlock(&(Global->idlock));}

/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

  {;};
  OneSolve(n, block_size, MyNum, dostats);
  print_timing_stats();
}


void OneSolve(long n, long block_size, long MyNum, long dostats)
{
  unsigned long myrs, myrf, mydone;
  struct LocalCopies *lc;

  lc = (struct LocalCopies *) malloc(sizeof(struct LocalCopies));
  if (lc == NULL) {
    fprintf(stderr,"Proc %ld could not malloc memory for lc\n",MyNum);
    exit(-1);
  }
  lc->t_in_fac = 0.0;
  lc->t_in_solve = 0.0;
  lc->t_in_mod = 0.0;
  lc->t_in_bar = 0.0;

  /* barrier to ensure all initialization is done */
  {
#line 370
	unsigned long	Error, Cycle;
#line 370
	long		Cancel, Temp;
#line 370

#line 370
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 370
	if (Error != 0) {
#line 370
		printf("Error while trying to get lock in barrier.\n");
#line 370
		exit(-1);
#line 370
	}
#line 370

#line 370
	Cycle = (Global->start).cycle;
#line 370
	if (++(Global->start).counter != (P)) {
#line 370
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 370
		while (Cycle == (Global->start).cycle) {
#line 370
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 370
			if (Error != 0) {
#line 370
				break;
#line 370
			}
#line 370
		}
#line 370
		pthread_setcancelstate(Cancel, &Temp);
#line 370
	} else {
#line 370
		(Global->start).cycle = !(Global->start).cycle;
#line 370
		(Global->start).counter = 0;
#line 370
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 370
	}
#line 370
	pthread_mutex_unlock(&(Global->start).mutex);
#line 370
};

  /* to remove cold-start misses, all processors begin by touching a[] */
  TouchA(block_size, MyNum);

  {
#line 375
	unsigned long	Error, Cycle;
#line 375
	long		Cancel, Temp;
#line 375

#line 375
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 375
	if (Error != 0) {
#line 375
		printf("Error while trying to get lock in barrier.\n");
#line 375
		exit(-1);
#line 375
	}
#line 375

#line 375
	Cycle = (Global->start).cycle;
#line 375
	if (++(Global->start).counter != (P)) {
#line 375
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 375
		while (Cycle == (Global->start).cycle) {
#line 375
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 375
			if (Error != 0) {
#line 375
				break;
#line 375
			}
#line 375
		}
#line 375
		pthread_setcancelstate(Cancel, &Temp);
#line 375
	} else {
#line 375
		(Global->start).cycle = !(Global->start).cycle;
#line 375
		(Global->start).counter = 0;
#line 375
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 375
	}
#line 375
	pthread_mutex_unlock(&(Global->start).mutex);
#line 375
};

/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */

  if ((MyNum == 0) || (dostats)) {
    {
#line 381
	struct timeval	FullTime;
#line 381

#line 381
	gettimeofday(&FullTime, NULL);
#line 381
	(myrs) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 381
};
  }

  lu(n, block_size, MyNum, lc, dostats);

  if ((MyNum == 0) || (dostats)) {
    {
#line 387
	struct timeval	FullTime;
#line 387

#line 387
	gettimeofday(&FullTime, NULL);
#line 387
	(mydone) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 387
};
  }

  {
#line 390
	unsigned long	Error, Cycle;
#line 390
	long		Cancel, Temp;
#line 390

#line 390
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 390
	if (Error != 0) {
#line 390
		printf("Error while trying to get lock in barrier.\n");
#line 390
		exit(-1);
#line 390
	}
#line 390

#line 390
	Cycle = (Global->start).cycle;
#line 390
	if (++(Global->start).counter != (P)) {
#line 390
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 390
		while (Cycle == (Global->start).cycle) {
#line 390
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 390
			if (Error != 0) {
#line 390
				break;
#line 390
			}
#line 390
		}
#line 390
		pthread_setcancelstate(Cancel, &Temp);
#line 390
	} else {
#line 390
		(Global->start).cycle = !(Global->start).cycle;
#line 390
		(Global->start).counter = 0;
#line 390
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 390
	}
#line 390
	pthread_mutex_unlock(&(Global->start).mutex);
#line 390
};

  if ((MyNum == 0) || (dostats)) {
    {
#line 393
	struct timeval	FullTime;
#line 393

#line 393
	gettimeofday(&FullTime, NULL);
#line 393
	(myrf) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 393
};
    Global->t_in_fac[MyNum] = lc->t_in_fac;
    Global->t_in_solve[MyNum] = lc->t_in_solve;
    Global->t_in_mod[MyNum] = lc->t_in_mod;
    Global->t_in_bar[MyNum] = lc->t_in_bar;
    Global->completion[MyNum] = mydone-myrs;
  }
  if (MyNum == 0) {
    Global->rs = myrs;
    Global->done = mydone;
    Global->rf = myrf;
  }
}


void lu0(double *a, long n, long stride)
{
  long j, k, length;
  double alpha;

  for (k=0; k<n; k++) {
    /* modify subsequent columns */
    for (j=k+1; j<n; j++) {
      a[k+j*stride] /= a[k+k*stride];
      alpha = -a[k+j*stride];
      length = n-k-1;
      daxpy(&a[k+1+j*stride], &a[k+1+k*stride], n-k-1, alpha);
    }
  }
}


void bdiv(double *a, double *diag, long stride_a, long stride_diag, long dimi, long dimk)
{
  long j, k;
  double alpha;

  for (k=0; k<dimk; k++) {
    for (j=k+1; j<dimk; j++) {
      alpha = -diag[k+j*stride_diag];
      daxpy(&a[j*stride_a], &a[k*stride_a], dimi, alpha);
    }
  }
}


void bmodd(double *a, double *c, long dimi, long dimj, long stride_a, long stride_c)
{
  long j, k, length;
  double alpha;

  for (k=0; k<dimi; k++)
    for (j=0; j<dimj; j++) {
      c[k+j*stride_c] /= a[k+k*stride_a];
      alpha = -c[k+j*stride_c];
      length = dimi - k - 1;
      daxpy(&c[k+1+j*stride_c], &a[k+1+k*stride_a], dimi-k-1, alpha);
    }
}


void bmod(double *a, double *b, double *c, long dimi, long dimj, long dimk, long stride)
{
  long j, k;
  double alpha;

  for (k=0; k<dimk; k++) {
    for (j=0; j<dimj; j++) {
      alpha = -b[k+j*stride];
      daxpy(&c[j*stride], &a[k*stride], dimi, alpha);
    }
  }
}


void daxpy(double *a, double *b, long n, double alpha)
{
  long i;

  for (i=0; i<n; i++) {
    a[i] += alpha*b[i];
  }
}


long BlockOwner(long I, long J)
{
//	return((I%num_cols) + (J%num_rows)*num_cols);
	return((I + J) % P);
}

long BlockOwnerColumn(long I, long J)
{
	return(I % P);
}

long BlockOwnerRow(long I, long J)
{
	return(((J % P) + (P / 2)) % P);
}

void lu(long n, long bs, long MyNum, struct LocalCopies *lc, long dostats)
{
  long i, il, j, jl, k, kl, I, J, K;
  double *A, *B, *C, *D;
  long strI;
  unsigned long t1, t2, t3, t4, t11, t22;

  strI = n;
  for (k=0, K=0; k<n; k+=bs, K++) {
    kl = k+bs; 
    if (kl>n) {
      kl = n;
    }

    if ((MyNum == 0) || (dostats)) {
      {
#line 509
	struct timeval	FullTime;
#line 509

#line 509
	gettimeofday(&FullTime, NULL);
#line 509
	(t1) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 509
};
    }

    /* factor diagonal block */
    if (BlockOwner(K, K) == MyNum) {
      A = &(a[k+k*n]); 
      lu0(A, kl-k, strI);
    }

    if ((MyNum == 0) || (dostats)) {
      {
#line 519
	struct timeval	FullTime;
#line 519

#line 519
	gettimeofday(&FullTime, NULL);
#line 519
	(t11) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 519
};
    }

    {
#line 522
	unsigned long	Error, Cycle;
#line 522
	long		Cancel, Temp;
#line 522

#line 522
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 522
	if (Error != 0) {
#line 522
		printf("Error while trying to get lock in barrier.\n");
#line 522
		exit(-1);
#line 522
	}
#line 522

#line 522
	Cycle = (Global->start).cycle;
#line 522
	if (++(Global->start).counter != (P)) {
#line 522
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 522
		while (Cycle == (Global->start).cycle) {
#line 522
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 522
			if (Error != 0) {
#line 522
				break;
#line 522
			}
#line 522
		}
#line 522
		pthread_setcancelstate(Cancel, &Temp);
#line 522
	} else {
#line 522
		(Global->start).cycle = !(Global->start).cycle;
#line 522
		(Global->start).counter = 0;
#line 522
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 522
	}
#line 522
	pthread_mutex_unlock(&(Global->start).mutex);
#line 522
};

    if ((MyNum == 0) || (dostats)) {
      {
#line 525
	struct timeval	FullTime;
#line 525

#line 525
	gettimeofday(&FullTime, NULL);
#line 525
	(t2) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 525
};
    }

    /* divide column k by diagonal block */
    D = &(a[k+k*n]);
    for (i=kl, I=K+1; i<n; i+=bs, I++) {
      if (BlockOwner/*Column*/(I, K) == MyNum) {  /* parcel out blocks */
	      /*if (K == 0) printf("C%lx\n", BlockOwnerColumn(I, K));*/
        il = i + bs;
        if (il > n) {
          il = n;
        }
        A = &(a[i+k*n]);
        bdiv(A, D, strI, n, il-i, kl-k);
      }
    }
    /* modify row k by diagonal block */
    for (j=kl, J=K+1; j<n; j+=bs, J++) {
      if (BlockOwner/*Row*/(K, J) == MyNum) {  /* parcel out blocks */
	      /*if (K == 0) printf("R%lx\n", BlockOwnerRow(K, J));*/
        jl = j+bs;
        if (jl > n) {
          jl = n;
        }
        A = &(a[k+j*n]);
        bmodd(D, A, kl-k, jl-j, n, strI);
      }
    }

    if ((MyNum == 0) || (dostats)) {
      {
#line 555
	struct timeval	FullTime;
#line 555

#line 555
	gettimeofday(&FullTime, NULL);
#line 555
	(t22) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 555
};
    }

    {
#line 558
	unsigned long	Error, Cycle;
#line 558
	long		Cancel, Temp;
#line 558

#line 558
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 558
	if (Error != 0) {
#line 558
		printf("Error while trying to get lock in barrier.\n");
#line 558
		exit(-1);
#line 558
	}
#line 558

#line 558
	Cycle = (Global->start).cycle;
#line 558
	if (++(Global->start).counter != (P)) {
#line 558
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 558
		while (Cycle == (Global->start).cycle) {
#line 558
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 558
			if (Error != 0) {
#line 558
				break;
#line 558
			}
#line 558
		}
#line 558
		pthread_setcancelstate(Cancel, &Temp);
#line 558
	} else {
#line 558
		(Global->start).cycle = !(Global->start).cycle;
#line 558
		(Global->start).counter = 0;
#line 558
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 558
	}
#line 558
	pthread_mutex_unlock(&(Global->start).mutex);
#line 558
};

    if ((MyNum == 0) || (dostats)) {
      {
#line 561
	struct timeval	FullTime;
#line 561

#line 561
	gettimeofday(&FullTime, NULL);
#line 561
	(t3) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 561
};
    }

    /* modify subsequent block columns */
    for (i=kl, I=K+1; i<n; i+=bs, I++) {
      il = i+bs;
      if (il > n) {
        il = n;
      }
      A = &(a[i+k*n]);
      for (j=kl, J=K+1; j<n; j+=bs, J++) {
        jl = j + bs;
        if (jl > n) {
          jl = n;
        }
        if (BlockOwner(I, J) == MyNum) {  /* parcel out blocks */
//		if (K == 0) printf("%lx\n", BlockOwner(I, J));
          B = &(a[k+j*n]);
          C = &(a[i+j*n]);
          bmod(A, B, C, il-i, jl-j, kl-k, n);
        }
      }
    }
    if ((MyNum == 0) || (dostats)) {
      {
#line 585
	struct timeval	FullTime;
#line 585

#line 585
	gettimeofday(&FullTime, NULL);
#line 585
	(t4) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 585
};
      lc->t_in_fac += (t11-t1);
      lc->t_in_solve += (t22-t2);
      lc->t_in_mod += (t4-t3);
      lc->t_in_bar += (t2-t11) + (t3-t22);
    }
  }
}


void InitA(double *rhs)
{
  long i, j;

  srand48((long) 1);
  for (j=0; j<n; j++) {
    for (i=0; i<n; i++) {
      a[i+j*n] = (double) lrand48()/MAXRAND;
      if (i == j) {
	a[i+j*n] *= 10;
      }
    }
  }

  for (j=0; j<n; j++) {
    rhs[j] = 0.0;
  }
  for (j=0; j<n; j++) {
    for (i=0; i<n; i++) {
      rhs[i] += a[i+j*n];
    }
  }
}


double TouchA(long bs, long MyNum)
{
  long i, j, I, J;
  double tot = 0.0;

  for (J=0; J*bs<n; J++) {
    for (I=0; I*bs<n; I++) {
      if (BlockOwner(I, J) == MyNum) {
        for (j=J*bs; j<(J+1)*bs && j<n; j++) {
          for (i=I*bs; i<(I+1)*bs && i<n; i++) {
            tot += a[i+j*n];
          }
        }
      }
    }
  }
  return(tot);
}


void PrintA()
{
  long i, j;

  for (i=0; i<n; i++) {
    for (j=0; j<n; j++) {
      printf("%8.1f ", a[i+j*n]);
    }
    printf("\n");
  }
}


void CheckResult(long n, double *a, double *rhs)
{
  long i, j, bogus = 0;
  double *y, diff, max_diff;

  y = (double *) malloc(n*sizeof(double));
  if (y == NULL) {
    printerr("Could not malloc memory for y\n");
    exit(-1);
  }
  for (j=0; j<n; j++) {
    y[j] = rhs[j];
  }
  for (j=0; j<n; j++) {
    y[j] = y[j]/a[j+j*n];
    for (i=j+1; i<n; i++) {
      y[i] -= a[i+j*n]*y[j];
    }
  }

  for (j=n-1; j>=0; j--) {
    for (i=0; i<j; i++) {
      y[i] -= a[i+j*n]*y[j];
    }
  }

  max_diff = 0.0;
  for (j=0; j<n; j++) {
    diff = y[j] - 1.0;
    if (fabs(diff) > 0.00001) {
      bogus = 1;
      max_diff = diff;
    }
  }
  if (bogus) {
    printf("TEST FAILED: (%.5f diff)\n", max_diff);
  } else {
    printf("TEST PASSED\n");
  }
  free(y);
}


void printerr(char *s)
{
  fprintf(stderr,"ERROR: %s\n",s);
}

