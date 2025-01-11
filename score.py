import subprocess
import logging
import numpy as np
logger = logging.getLogger(__name__)

my_mean = []
my_std = []
my_score = []

bench_mean = []
bench_std = []
bench_score = []

def backup():
    logger.info("clean up and prepare backup files")
    subprocess.run(["make", "clean"])
    subprocess.run(["mkdir", "-p", "backup"])
    subprocess.run(["cp", "engine.c", "backup/engine.c"])
    subprocess.run(["cp", "winning_engine.c", "backup/winning_engine.c"])

def score_my(count):
    logger.info("build my limit order book implementation")
    subprocess.run(["cp", "backup/engine.c", "engine.c"])
    subprocess.run(["make", "all"])

    logger.info("test my limit order book performance")
    score_once() # warm up
    for i in range(count):
        mean, std, score = score_once()
        my_mean.append(mean)
        my_std.append(std)
        my_score.append(score)

def score_bench(count):
    logger.info("build benchmark limit order book implementation")
    subprocess.run(["cp", "backup/winning_engine.c", "engine.c"])
    subprocess.run(["make", "all"])

    logger.info("test benchmark limit order book performance")
    score_once() # warm up
    for i in range(count):
        mean, std, score = score_once()
        bench_mean.append(mean)
        bench_std.append(std)
        bench_score.append(score)
    subprocess.run(["cp", "backup/engine.c", "engine.c"])

def score_once():
    result = subprocess.run(["./score"], text = True, capture_output=True)
    result = result.stdout.split("\n")[-3:]
    mean = float(result[0].strip())
    std = float(result[1].strip())
    score = float(result[2].strip())
    return mean, std, score


if __name__ == "__main__":
    test_time = 10
    backup()
    score_my(test_time)
    score_bench(test_time)

    my_mean_avg = np.mean(my_mean)
    my_std_avg = np.mean(my_std)
    my_score_avg = np.mean(my_score)

    bench_mean_avg = np.mean(bench_mean)
    bench_std_avg = np.mean(bench_std)
    bench_score_avg = np.mean(bench_score)

    print(f"Across {test_time} scoring:")
    print(f"my implementation: mean {my_mean_avg} std {my_std_avg} score {my_score_avg}")
    print(f"benchmark implementation: mean {bench_mean_avg} std {bench_std_avg} score {bench_score_avg}")