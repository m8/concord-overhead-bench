import subprocess
import os
import csv
import re

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

unroll_configs = {}
overhead_results = "overhead_results.txt"
benchs = [
    {
        "name": "splash2",
        "path": "splash2/codes/",
        "benchs": ["water-nsquared", "water-spatial", "fmm", "raytrace", "radix", "fft", "lu-c", "lu-nc", "ocean-cp", "ocean-ncp", "volrend"]
    },
    {
        "name": "parsec",
        "path": "parsec-benchmark/pkgs/",
        "benchs": ["blackscholes", "fluidanimate", "swaptions", "canneal", "streamcluster", "dedup"]
    },
    {
        "name": "phoenix",
        "path": "phoenix/phoenix-2.0/",
        "benchs": ["histogram", "kmeans", "pca", "string_match", "linear_regression", "word_count"]
    }
]

def load_configs():
    global unroll_configs
    with open("configs/unroll.conf", "r") as f:
        reader = csv.reader(f)
        for row in reader:
            unroll_configs[row[0]] = [row[1], row[2]]    
    unroll_configs.pop("bench")


def run_bench(bench_category, bench_name, accuracy=0, pass_type="cache"):

    os.chdir(os.path.join(SCRIPT_DIR, bench_category["path"]))

    cmd = f" RUNS={1 if accuracy else 3 } \
                                        MODIFIED_SUBLOOP_COUNT={int(unroll_configs[bench_name][1])} \
                                        UNROLL_COUNT={int(unroll_configs[bench_name][0])} \
                                        ACCURACY_TEST={accuracy} CONCORD_PASS_TYPE={pass_type} \
                                        ./perf_test.sh " + bench_name
    
    print("Running command: ", cmd)
    output = subprocess.check_output(cmd, shell=True).decode("utf-8")

    if accuracy:
        move_accuracy_result(bench_category["name"], bench_name)
    else:
        try:
            overhead = re.search("Overhead: ((\d+)?\.\d+)", output).group(1)
        except:
            overhead = "error"

        with open(os.path.join(SCRIPT_DIR, overhead_results), "a") as f:
            f.write(f"{bench_category['name']},{bench_name},{overhead}\n")

    return output


def move_accuracy_result(bench_category, bench_name):
    os.system(f"mv {bench_category}_stats/accuracy-{bench_name}.txt {SCRIPT_DIR}/results/accuracy-{bench_name}.bin")

def run_category(bench_category, timeliness=False, overhead=True):    
    os.chdir(os.path.join(SCRIPT_DIR, bench_category["path"]))

    if timeliness:
        for bench in bench_category["benchs"]:
            print("Running", bench)
            run_bench(bench_category, bench, accuracy=1, pass_type="rdtsc")
    
    if overhead:
        for bench in bench_category["benchs"]:
            print("Running", bench)
            run_bench(bench_category, bench, accuracy=0, pass_type="cache")

if __name__ == "__main__":
    
    print("Running benchmarks")
    print("=================================")

    load_configs()

    print(unroll_configs)

    if not os.path.exists("results"):
        os.mkdir("results")

    run_category(benchs[0], timeliness=True, overhead=True)
    run_category(benchs[2], timeliness=True, overhead=True)
    run_category(benchs[1], timeliness=True, overhead=True)

    # run_bench(benchs[1], "blackscholes", 0, "cache")
    # run_bench(benchs[0], "radix", accuracy=0, pass_type="cache")
    # run_bench(benchs[0], "lu-nc", accuracy=0, pass_type="cache-line")
    # run_specific_bench("splash2/codes/", "fft")
    # run_splash_benchs(timeliness=False, overhead=True)
    # run_phoenix_benchs(timeliness=False, overhead=True)
    # run_parsec_benchs(timeliness=False, overhead=True)
