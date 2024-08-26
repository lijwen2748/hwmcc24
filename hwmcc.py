import os
import sys
import subprocess
import multiprocessing

def check_condition():
    # To implement later.
    return True

def do_one(bin_path, config, case_path, out_path, timeout, terminate_flag):
    cmd_args = [bin_path] + config + [case_path, out_path]
    try:
        output = subprocess.check_output(cmd_args,stdin=None, stderr=None, shell=False, universal_newlines=False, timeout=timeout)
        if check_condition():
            terminate_flag.value = True
    except subprocess.TimeoutExpired:
        pass
    except subprocess.CalledProcessError as e:
        print(f"Error in {case_path}: {e}")

def print_error(e):
    print(e)

def parallel_one(case_path, out_path, config_list):
    with multiprocessing.Manager() as manager:
        terminate_flag = manager.Value('b', False)
        pool = multiprocessing.Pool(processes=os.cpu_count())
        for bin_path, config, tlimit in config_list:
            pool.apply_async(do_one,((bin_path, config, case_path, out_path, tlimit, terminate_flag)),error_callback=print_error)
        pool.close()
        while True:
            pool.join()
            if terminate_flag.value:
                pool.terminate()
                break
    

if __name__ == "__main__":
    case_dir = "~/miniCAR/bench/notsafe/6s31.aig"
    case_dir = os.path.expanduser(case_dir) # expand ~

    output_dir = "output"
    config_list = [
        ("./minicar", ["--vb"], 3600),
        ("./minicar", ["-b", "--inter", "2"], 3600),
        # ..
    ]
    parallel_one(case_dir, output_dir, config_list)
