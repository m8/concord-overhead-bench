#!/bin/bash

BENCHMARK="water_ns_"
STAT_FILE=$BENCHMARK"stats.csv"
STAT_OUT_FILE=$BENCHMARK"stats.txt"
LOG_FILE=$BENCHMARK"out.log"
ERROR_FILE=$BENCHMARK"error.log"
PI=5000
CI=1000
AD=0
NUMBER_OF_RUNS=1

rm -f $STAT_FILE $STAT_OUT_FILE $LOG_FILE $ERROR_FILE out

run_naive() {
  # Running in naive instrumentation level
  make clean -f Makefile.single.lc > /dev/null 2>>$ERROR_FILE
  ALLOWED_DEVIATION=$AD PUSH_INTV=$PI CMMT_INTV=$CI INST_LEVEL=3 make -f Makefile.single.lc > /dev/null 2>>$ERROR_FILE
  DIVISOR=`expr $NUMBER_OF_RUNS \* 1000`
  rm -f sum
  echo -n "scale=2;(" > sum
  for j in `seq 1 $NUMBER_OF_RUNS`
  do
    ./OCEAN -n130 -p32 -e1e-07 -r20000 -t28800 > out
    time_in_us=`cat out | grep "Total time without initialization : " | cut -d ':' -f 2 | tr -d '[:space:]'`
    cat out >> $LOG_FILE
    echo "All_blocks,$time_in_us usec" | tee -a $STAT_FILE $STAT_OUT_FILE
    echo $time_in_us | tr -d '\n' >> sum
    if [ $j -lt $NUMBER_OF_RUNS ]; then
      echo -n "+" >> sum
    fi
  done
  echo ")/$DIVISOR" >> sum
  time_in_us=`cat sum | bc`
  echo "All_blocks mean time,$time_in_us ms" | tee -a $STAT_OUT_FILE
  naive_time=$time_in_us

  ./OCEAN -n130 -p1 -e1e-07 -r20000 -t28800 > out
  lc=`cat out | grep "Logical Clock:" | tail -n 1 | cut -d: -f 2`
  naive_lc=$lc
  echo "Naive instrumentation time taken $naive_time ms" | tee -a $STAT_OUT_FILE
}

run_opt() {
  # Running in optimised instrumentation level
  make clean -f Makefile.single.lc > /dev/null 2>>$ERROR_FILE
  ALLOWED_DEVIATION=$1 PUSH_INTV=$PI CMMT_INTV=$CI INST_LEVEL=1 make -f Makefile.single.lc > /dev/null 2>>$ERROR_FILE
  DIVISOR=`expr $NUMBER_OF_RUNS \* 1000`
  rm -f sum
  echo -n "scale=2;(" > sum
  for j in `seq 1 $NUMBER_OF_RUNS`
  do
    ./OCEAN -n130 -p32 -e1e-07 -r20000 -t28800 > out
    time_in_us=`cat out | grep "Total time without initialization : " | cut -d ':' -f 2 | tr -d '[:space:]'`
    cat out >> $LOG_FILE
    echo "Opt,$time_in_us usec" | tee -a $STAT_FILE $STAT_OUT_FILE
    echo $time_in_us | tr -d '\n' >> sum
    if [ $j -lt $NUMBER_OF_RUNS ]; then
      echo -n "+" >> sum
    fi
  done
  echo ")/$DIVISOR" >> sum
  time_in_us=`cat sum | bc`
  echo "Opt mean time(allowed deviation:$1),$time_in_us ms" | tee -a $STAT_OUT_FILE
  opt_time=$time_in_us

  ./OCEAN -n130 -p1 -e1e-07 -r20000 -t28800 > out
  lc=`cat out | grep "Logical Clock:" | tail -n 1 | cut -d: -f 2`
  opt_lc=$lc
  echo "Optimal instrumentation time taken (allowed deviation $1) $opt_time ms" | tee -a $STAT_OUT_FILE
  echo "Logical clock for naive instrumentation: $naive_lc" | tee -a $STAT_OUT_FILE
  echo "Logical clock for optimized instrumentation: $opt_lc" | tee -a $STAT_OUT_FILE  
  if [ $naive_lc -gt $opt_lc ]; then
    err=`echo "scale = 5; (($naive_lc - $opt_lc) * 100 / $naive_lc)" | bc -l`
    echo "Err (in instrument count): $err% less" | tee -a $STAT_OUT_FILE
  else
    err=`echo "scale = 5; (($opt_lc - $naive_lc) * 100 / $naive_lc)" | bc -l`
    echo "Err (in instrument count): $err% more" | tee -a $STAT_OUT_FILE
  fi
}

run_naive
run_opt 0
rm -f out
