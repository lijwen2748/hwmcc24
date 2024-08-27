import os
import sys
import subprocess
import multiprocessing


def check_condition():
    # To implement later.
    return True


def do_one(bin_path, config, timeout, terminate_flag):
    cmd_args = [bin_path] + config
    try:
        output = subprocess.check_output(
            cmd_args,
            stdin=None,
            stderr=None,
            shell=False,
            universal_newlines=False,
            timeout=timeout,
        )
        if check_condition():
            terminate_flag.value = True
    except subprocess.TimeoutExpired:
        pass
    except subprocess.CalledProcessError as e:
        print(f"Error in {case_path}: {e}")


def print_error(e):
    print(e)


def parallel_one(config_list):
    with multiprocessing.Manager() as manager:
        terminate_flag = manager.Value("b", False)
        pool = multiprocessing.Pool(processes=os.cpu_count())
        for bin_path, config, tlimit in config_list:
            pool.apply_async(
                do_one,
                ((bin_path, config, tlimit, terminate_flag)),
                error_callback=print_error,
            )
        pool.close()
        while True:
            pool.join()
            if terminate_flag.value:
                pool.terminate()
                break


if __name__ == "__main__":
    case_dir = "~/miniCAR/bench/notsafe/counterp0.aig"  # for test use.
    case_dir = os.path.expanduser(case_dir)  # expand ~

    output_dir = "output"
    output_dir = os.path.expanduser(output_dir)
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
    config_list = [
        ("./bin/MCAR", ["--vb", f"{case_dir}", f"{output_dir}"], 3600),
        (
            "./bin/simplecar",
            [f"{case_dir}", "-v 1", "-f", "-br 1", "-rs", "-w", f"{output_dir}"],
            3600,
        ),
        (
            "./bin/simplecar",
            [f"{case_dir}", "-v 1", "-b", "-br 1", "-rs", "-w", f"{output_dir}"],
            3600,
        ),
        (
            "./bin/simplecar",
            [f"{case_dir}", "-v 1", "-bmc", "-w", f"{output_dir}"],
            3600,
        ),  # change here
        # ..
    ]
    parallel_one(config_list)
