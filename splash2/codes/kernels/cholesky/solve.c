#line 270 "../../../../splash2/codes//null_macros/c.m4.null"

#line 1 "solve.C"
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
/*  Sparse Cholesky Factorization (Fan-Out with no block copy-across)    */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -pP : P = number of processors.                                      */
/*  -Bb : Use a postpass partition size of b.                            */
/*  -Cc : Cache size in bytes.                                           */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Test output.                                                   */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/


#line 34
#include <pthread.h>
#line 34
#include <sys/time.h>
#line 34
#include <unistd.h>
#line 34
#include <stdlib.h>
#line 34
#include "TriggerAction.h"
#line 34

#line 34
pthread_t PThreadTable[64];
#line 34

#line 34


#include <math.h>
#include "matrix.h"

#define SH_MEM_AMT   100000000
#define DEFAULT_PPS         32
#define DEFAULT_CS       16384
#define DEFAULT_P            1

double CacheSize = DEFAULT_CS;
double CS;
long BS = 45;

struct GlobalMemory *Global;

long *T, *nz, *node, *domain, *domains, *proc_domains;

long *PERM, *INVP;

long solution_method = FAN_OUT*10+0;

long distribute = -1;

long target_partition_size = 0;
long postpass_partition_size = DEFAULT_PPS;
long permutation_method = 1;
long join = 1; /* attempt to amalgamate supernodes */
long scatter_decomposition = 0;

long P=DEFAULT_P;
long iters = 1;
SMatrix M;      /* input matrix */

char probname[80];

extern struct Update *freeUpdate[MAX_PROC];
extern struct Task *freeTask[MAX_PROC];
extern long *firstchild, *child;
extern BMatrix LB;
extern char *optarg;

struct gpid {
  long pid;
  unsigned long initdone;
  unsigned long finish;
} *gp;

long do_test = 0;
long do_stats = 0;

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
  double *b, *x;
  double norm;
  long i;
  long c;
  long *assigned_ops, num_nz, num_domain, num_alloc, ps;
  long *PERM2;
  extern double *work_tree;
  extern long *partition;
  unsigned long start;
  double mint, maxt, avgt;

  {
#line 106
	struct timeval	FullTime;
#line 106

#line 106
	gettimeofday(&FullTime, NULL);
#line 106
	(start) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 106
}

  while ((c = getopt(argc, argv, "B:C:p:D:sth")) != -1) {
    switch(c) {
    case 'B': postpass_partition_size = atoi(optarg); break;  
    case 'C': CacheSize = (double) atoi(optarg); break;  
    case 'p': P = atol(optarg); break;  
    case 's': do_stats = 1; break;  
    case 't': do_test = 1; break;  
    case 'h': printf("Usage: CHOLESKY <options> file\n\n");
              printf("options:\n");
              printf("  -Bb : Use a postpass partition size of b.\n");
              printf("  -Cc : Cache size in bytes.\n");
              printf("  -pP : P = number of processors.\n");
              printf("  -s  : Print individual processor timing statistics.\n");
              printf("  -t  : Test output.\n");
              printf("  -h  : Print out command line options.\n\n");
              printf("Default: CHOLESKY -p%1d -B%1d -C%1d\n",
                     DEFAULT_P,DEFAULT_PPS,DEFAULT_CS);
              exit(0);
              break;
    }
  }

  CS = CacheSize / 8.0;
  CS = sqrt(CS);
  BS = (long) floor(CS+0.5);

  {
#line 134
    }

  gp = (struct gpid *) malloc(sizeof(struct gpid));;
  gp->pid = 0;
  Global = (struct GlobalMemory *)
    malloc(sizeof(struct GlobalMemory));;
  {
#line 140
	unsigned long	Error;
#line 140

#line 140
	Error = pthread_mutex_init(&(Global->start).mutex, NULL);
#line 140
	if (Error != 0) {
#line 140
		printf("Error while initializing barrier.\n");
#line 140
		exit(-1);
#line 140
	}
#line 140

#line 140
	Error = pthread_cond_init(&(Global->start).cv, NULL);
#line 140
	if (Error != 0) {
#line 140
		printf("Error while initializing barrier.\n");
#line 140
		pthread_mutex_destroy(&(Global->start).mutex);
#line 140
		exit(-1);
#line 140
	}
#line 140

#line 140
	(Global->start).counter = 0;
#line 140
	(Global->start).cycle = 0;
#line 140
}
  {pthread_mutex_init(&(Global->waitLock), NULL);}
  {pthread_mutex_init(&(Global->memLock), NULL);}

  MallocInit(P);  

  i = 0;
  while (++i < argc && argv[i][0] == '-')
    ;
  M = ReadSparse(argv[i], probname);

  distribute = LB_DOMAINS*10 + EMBED;

  printf("\n");
  printf("Sparse Cholesky Factorization\n");
  printf("     Problem: %s\n",probname);
  printf("     %ld Processors\n",P);
  printf("     Postpass partition size: %ld\n",postpass_partition_size);
  printf("     %0.0f byte cache\n",CacheSize);
  printf("\n");
  printf("\n");

  printf("true partitions\n");

  printf("Fan-out, ");
  printf("no block copy-across\n");

  printf("LB domain, "); 
  printf("embedded ");
  printf("distribution\n");

  printf("No ordering\n");

  PERM = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  INVP = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);

  CreatePermutation(M.n, PERM, NO_PERM);

  InvertPerm(M.n, PERM, INVP);

  T = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  EliminationTreeFromA(M, T, PERM, INVP);

  firstchild = (long *) MyMalloc((M.n+2)*sizeof(long), DISTRIBUTED);
  child = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  ParentToChild(T, M.n, firstchild, child);

  nz = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  ComputeNZ(M, T, nz, PERM, INVP);

  work_tree = (double *) MyMalloc((M.n+1)*sizeof(double), DISTRIBUTED);
  ComputeWorkTree(M, nz, work_tree);

  node = (long *) MyMalloc((M.n+1)*sizeof(long), DISTRIBUTED);
  FindSupernodes(M, T, nz, node);

  Amalgamate2(1, M, T, nz, node, (long *) NULL, 1);


  assigned_ops = (long *) malloc(P*sizeof(long));
  domain = (long *) MyMalloc(M.n*sizeof(long), DISTRIBUTED);
  domains = (long *) MyMalloc(M.n*sizeof(long), DISTRIBUTED);
  proc_domains = (long *) MyMalloc((P+1)*sizeof(long), DISTRIBUTED);
  printf("before partition\n");
  fflush(stdout);
  Partition(M, P, T, assigned_ops, domain, domains, proc_domains);
  free(assigned_ops);

  {
    long i, tot_domain_updates, tail_length;

    tot_domain_updates = 0;
    for (i=0; i<proc_domains[P]; i++) {
      tail_length = nz[domains[i]]-1;
      tot_domain_updates += tail_length*(tail_length+1)/2;
    }
    printf("%ld total domain updates\n", tot_domain_updates);
  }

  num_nz = num_domain = 0;
  for (i=0; i<M.n; i++) {
    num_nz += nz[i];
    if (domain[i])
      num_domain += nz[i];
  }
  
  ComputeTargetBlockSize(M, P);

  printf("Target partition size %ld, postpass size %ld\n",
	 target_partition_size, postpass_partition_size);

  NoSegments(M);

  PERM2 = (long *) malloc((M.n+1)*sizeof(long));
  CreatePermutation(M.n, PERM2, permutation_method);
  ComposePerm(PERM, PERM2, M.n);
  free(PERM2);

  InvertPerm(M.n, PERM, INVP);

  b = CreateVector(M);

  ps = postpass_partition_size;
  num_alloc = num_domain + (num_nz-num_domain)*10/ps/ps;
  CreateBlockedMatrix2(M, num_alloc, T, firstchild, child, PERM, INVP,
		       domain, partition);

  FillInStructure(M, firstchild, child, PERM, INVP);

  AssignBlocksNow();

  AllocateNZ();

  FillInNZ(M, PERM, INVP);
  FreeMatrix(M);

  InitTaskQueues(P);

  PreAllocate1FO();
  ComputeRemainingFO();
  ComputeReceivedFO();

/*************************** For CI evaluation *******************************/
  {
#line 263
	long	i, Error;
#line 263

#line 263
  unsigned int thrIndex = 1; 
#line 263
	for (i = 0; i < (P) - 1; i++) {
#line 263
		Error = pthread_create(&PThreadTable[i], NULL, (void * (*)(void *))(Go), (void*)thrIndex);
#line 263
    thrIndex++;
#line 263
		if (Error != 0) {
#line 263
			printf("Error in pthread_create().\n");
#line 263
			exit(-1);
#line 263
		}
#line 263
	}
#line 263
	Go((void*)0);
#line 263
};
/*****************************************************************************/
  {
#line 265
	unsigned long	i, Error;
#line 265
	for (i = 0; i < (P) - 1; i++) {
#line 265
		Error = pthread_join(PThreadTable[i], NULL);
#line 265
		if (Error != 0) {
#line 265
			printf("Error in pthread_join().\n");
#line 265
			exit(-1);
#line 265
		}
#line 265
	}
#line 265
};

  printf("%.0f operations for factorization\n", work_tree[M.n]);

  printf("\n");
  printf("                            PROCESS STATISTICS\n");
  printf("              Total\n");
  printf(" Proc         Time \n");
  printf("    0    %10.0ld\n", Global->runtime[0]);
  if (do_stats) {
    maxt = avgt = mint = Global->runtime[0];
    for (i=1; i<P; i++) {
      if (Global->runtime[i] > maxt) {
        maxt = Global->runtime[i];
      }
      if (Global->runtime[i] < mint) {
        mint = Global->runtime[i];
      }
      avgt += Global->runtime[i];
    }
    avgt = avgt / P;
    for (i=1; i<P; i++) {
      printf("  %3ld    %10ld\n",i,Global->runtime[i]);
    }
    printf("  Avg    %10.0f\n",avgt);
    printf("  Min    %10.0f\n",mint);
    printf("  Max    %10.0f\n",maxt);
    printf("\n");
  }

  printf("                            TIMING INFORMATION\n");
  printf("Start time                        : %16lu\n",
          start);
  printf("Initialization finish time        : %16lu\n",
          gp->initdone);
  printf("Overall finish time               : %16lu\n",
          gp->finish);
  printf("Total time with initialization    : %16lu\n",
          gp->finish-start);
  printf("Total time without initialization : %16lu\n",
          gp->finish-gp->initdone);
  printf("\n");

  if (do_test) {
    printf("                             TESTING RESULTS\n");
    x = TriBSolve(LB, b, PERM);
    norm = ComputeNorm(x, LB.n);
    if (norm >= 0.0001) {
      printf("Max error is %10.9f\n", norm);
    } else {
      printf("PASSED\n");
    }
  }

/*************************** For CI evaluation *******************************/
  gettimeofday(&rt_end, NULL);
  uint64_t run_time = (rt_end.tv_sec - rt_begin.tv_sec)*1000000 + (rt_end.tv_usec - rt_begin.tv_usec);
  print_timing_stats();
  printf("cholesky runtime: %lu usec\n", run_time);  
/*****************************************************************************/

  {
#line 326
    exit(0);}
}


void Go(void *vpIndex)
{
/*************************** For CI evaluation *******************************/
  int idx = (int)vpIndex;
  {
#line 334
    init_stats(idx);
#line 334
    };
/*****************************************************************************/
  long MyNum;
  struct LocalCopies *lc;

  {pthread_mutex_lock(&(Global->waitLock));}
    MyNum = gp->pid;
    gp->pid++;
  {pthread_mutex_unlock(&(Global->waitLock));}

  {;};
/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

  lc =(struct LocalCopies *) malloc(sizeof(struct LocalCopies)+2*PAGE_SIZE);
#line 350
  lc->freeUpdate = NULL;
  lc->freeTask = NULL;
  lc->runtime = 0;

  PreAllocateFO(MyNum,lc);

    /* initialize - put original non-zeroes in L */

  PreProcessFO(MyNum);

  {
#line 360
	unsigned long	Error, Cycle;
#line 360
	long		Cancel, Temp;
#line 360

#line 360
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 360
	if (Error != 0) {
#line 360
		printf("Error while trying to get lock in barrier.\n");
#line 360
		exit(-1);
#line 360
	}
#line 360

#line 360
	Cycle = (Global->start).cycle;
#line 360
	if (++(Global->start).counter != (P)) {
#line 360
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 360
		while (Cycle == (Global->start).cycle) {
#line 360
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 360
			if (Error != 0) {
#line 360
				break;
#line 360
			}
#line 360
		}
#line 360
		pthread_setcancelstate(Cancel, &Temp);
#line 360
	} else {
#line 360
		(Global->start).cycle = !(Global->start).cycle;
#line 360
		(Global->start).counter = 0;
#line 360
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 360
	}
#line 360
	pthread_mutex_unlock(&(Global->start).mutex);
#line 360
};

/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */

  if ((MyNum == 0) || (do_stats)) {
    {
#line 366
	struct timeval	FullTime;
#line 366

#line 366
	gettimeofday(&FullTime, NULL);
#line 366
	(lc->rs) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 366
};
  }

  BNumericSolveFO(MyNum,lc);

  {
#line 371
	unsigned long	Error, Cycle;
#line 371
	long		Cancel, Temp;
#line 371

#line 371
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 371
	if (Error != 0) {
#line 371
		printf("Error while trying to get lock in barrier.\n");
#line 371
		exit(-1);
#line 371
	}
#line 371

#line 371
	Cycle = (Global->start).cycle;
#line 371
	if (++(Global->start).counter != (P)) {
#line 371
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 371
		while (Cycle == (Global->start).cycle) {
#line 371
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 371
			if (Error != 0) {
#line 371
				break;
#line 371
			}
#line 371
		}
#line 371
		pthread_setcancelstate(Cancel, &Temp);
#line 371
	} else {
#line 371
		(Global->start).cycle = !(Global->start).cycle;
#line 371
		(Global->start).counter = 0;
#line 371
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 371
	}
#line 371
	pthread_mutex_unlock(&(Global->start).mutex);
#line 371
};

  if ((MyNum == 0) || (do_stats)) {
    {
#line 374
	struct timeval	FullTime;
#line 374

#line 374
	gettimeofday(&FullTime, NULL);
#line 374
	(lc->rf) = (unsigned long)(FullTime.tv_usec + FullTime.tv_sec * 1000000);
#line 374
};
    lc->runtime += (lc->rf-lc->rs);
  }

  if (MyNum == 0) {
    CheckRemaining();
    CheckReceived();
  }

  {
#line 383
	unsigned long	Error, Cycle;
#line 383
	long		Cancel, Temp;
#line 383

#line 383
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 383
	if (Error != 0) {
#line 383
		printf("Error while trying to get lock in barrier.\n");
#line 383
		exit(-1);
#line 383
	}
#line 383

#line 383
	Cycle = (Global->start).cycle;
#line 383
	if (++(Global->start).counter != (P)) {
#line 383
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 383
		while (Cycle == (Global->start).cycle) {
#line 383
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 383
			if (Error != 0) {
#line 383
				break;
#line 383
			}
#line 383
		}
#line 383
		pthread_setcancelstate(Cancel, &Temp);
#line 383
	} else {
#line 383
		(Global->start).cycle = !(Global->start).cycle;
#line 383
		(Global->start).counter = 0;
#line 383
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 383
	}
#line 383
	pthread_mutex_unlock(&(Global->start).mutex);
#line 383
};

  if ((MyNum == 0) || (do_stats)) {
    Global->runtime[MyNum] = lc->runtime;
  }
  if (MyNum == 0) {
    gp->initdone = lc->rs;
    gp->finish = lc->rf;
  } 

  {
#line 393
	unsigned long	Error, Cycle;
#line 393
	long		Cancel, Temp;
#line 393

#line 393
	Error = pthread_mutex_lock(&(Global->start).mutex);
#line 393
	if (Error != 0) {
#line 393
		printf("Error while trying to get lock in barrier.\n");
#line 393
		exit(-1);
#line 393
	}
#line 393

#line 393
	Cycle = (Global->start).cycle;
#line 393
	if (++(Global->start).counter != (P)) {
#line 393
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &Cancel);
#line 393
		while (Cycle == (Global->start).cycle) {
#line 393
			Error = pthread_cond_wait(&(Global->start).cv, &(Global->start).mutex);
#line 393
			if (Error != 0) {
#line 393
				break;
#line 393
			}
#line 393
		}
#line 393
		pthread_setcancelstate(Cancel, &Temp);
#line 393
	} else {
#line 393
		(Global->start).cycle = !(Global->start).cycle;
#line 393
		(Global->start).counter = 0;
#line 393
		Error = pthread_cond_broadcast(&(Global->start).cv);
#line 393
	}
#line 393
	pthread_mutex_unlock(&(Global->start).mutex);
#line 393
};

  print_timing_stats();
}


void PlaceDomains(long P)
{
  long p, d, first;
  char *range_start, *range_end;

  for (p=P-1; p>=0; p--)
    for (d=LB.proc_domains[p]; d<LB.proc_domains[p+1]; d++) {
      first = LB.domains[d];
      while (firstchild[first] != firstchild[first+1])
        first = child[firstchild[first]];

      /* place indices */
      range_start = (char *) &LB.row[LB.col[first]];
      range_end = (char *) &LB.row[LB.col[LB.domains[d]+1]];
      MigrateMem(&LB.row[LB.col[first]],
		 (LB.col[LB.domains[d]+1]-LB.col[first])*sizeof(long),
		 p);

      /* place non-zeroes */
      range_start = (char *) &BLOCK(LB.col[first]);
      range_end = (char *) &BLOCK(LB.col[LB.domains[d]+1]);
      MigrateMem(&BLOCK(LB.col[first]),
		 (LB.col[LB.domains[d]+1]-LB.col[first])*sizeof(double),
		 p);
    }
}



/* Compute result of first doing PERM1, then PERM2 (placed back in PERM1) */

void ComposePerm(long *PERM1, long *PERM2, long n)
{
  long i, *PERM3;

  PERM3 = (long *) malloc((n+1)*sizeof(long));

  for (i=0; i<n; i++)
    PERM3[i] = PERM1[PERM2[i]];

  for (i=0; i<n; i++)
    PERM1[i] = PERM3[i];

  free(PERM3);
}

