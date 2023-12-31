#!/usr/bin/env bash


set -e
sim=true
huge=true
verbose=''


while getopts Sv OPT; do
  case "$OPT" in
    S)
      sim=false
      ;;
    v)
      verbose=-v
      ;;
  esac
done

shift "$(($OPTIND - 1))"
download_dir="${1:-.}"
outdir="${2:-.}"
url_base=http://parsec.cs.princeton.edu/download/3.0
basenames=''
basenames="$basenames parsec-3.0-core.tar.gz"
if "$sim"; then
  basenames="$basenames parsec-3.0-input-sim.tar.gz"
fi

# # Huge. Impractical for simulators, intended for real silicon.
if "$huge"; then
  basenames="$basenames parsec-3.0-input-native.tar.gz"
fi

mkdir -p "$outdir"
for basename in $basenames; do
  if [ ! -f "${download_dir}/${basename}" ]; then
    wget -P "$download_dir" $verbose "${url_base}/${basename}"
  fi
  tar -xz $verbose -f "${download_dir}/${basename}" -C "$outdir" --skip-old-files --strip-components=1
done

# # ====================

kernels=("canneal" "dedup")
apps=("blackscholes" "fluidanimate")

for benchmark in "${kernels[@]}"
do
  echo "Untarring $benchmark"
  files=`ls $outdir/pkgs/kernels/$benchmark/inputs/*.tar`
  for file in $files
  do
    echo "Running tar -xf $file -C $outdir/pkgs/kernels/$benchmark/inputs"
    tar -xf $file -C $outdir/pkgs/kernels/$benchmark/inputs
  done
done

for benchmark in "${apps[@]}"
do
  echo "Untarring $benchmark"
  files=`ls $outdir/pkgs/apps/$benchmark/inputs/*.tar`
  for file in $files
  do
    echo "Running tar -xf $file -C $outdir/pkgs/apps/$benchmark/inputs"
    tar -xf $file -C $outdir/pkgs/apps/$benchmark/inputs
  done
done