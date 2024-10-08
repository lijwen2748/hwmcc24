import os
import subprocess
import multiprocessing
import resource
import shutil
import click
import signal
import time

_verbose = False


def check_condition():
    # To implement later.
    return True


def set_memory_limit(memory_limit):
    # Set memory limit in bytes
    resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))


def do_one(bin_path, res_path, filename, config, timeout, memory_limit, terminate_flag, pid_list):
    cmd_args = [bin_path] + config
    if _verbose:
        print(f"begin : {cmd_args}")
    try:
        set_memory_limit(memory_limit)

        try:
            proc = subprocess.Popen(
                cmd_args,
                stdin=None,
                stderr=subprocess.PIPE,
                shell=False,
                universal_newlines=False,
            )
        except Exception as e:
            print(f"Failed to start process for {bin_path}: {e}")
            return

        pid_list.append(proc.pid)

        try:
            output, error = proc.communicate(timeout=timeout)
            possible_cex_path1 = os.path.join(res_path, filename+".res")
            possible_cex_path2 = os.path.join(res_path, filename+".cex")
            possible_cert_path = os.path.join(res_path, filename+".w.aag")
            res_outputed = (os.path.exists(possible_cert_path) and file_not_empty(possible_cert_path)) \
                        or (os.path.exists(possible_cex_path1) and file_not_empty(possible_cex_path1)) \
                        or (os.path.exists(possible_cex_path2) and file_not_empty(possible_cex_path2)) 
            # should we copy it to target dir here?
            if _verbose and error is not None and error != b'':
                print(f"error of {cmd_args} : {error}")
            if check_condition() and (error is None or error == b'') and res_outputed:
                terminate_flag.value = True
            if _verbose:
                print(f"end : {cmd_args}")
        except subprocess.TimeoutExpired:
            proc.kill()  # Ensure the process is killed on timeout
            if _verbose:
                print(f"Timeout for {cmd_args}")
        except subprocess.CalledProcessError as e:
            print(f"Error in {bin_path}: {e}")
        finally:
            if proc.poll() is None: # terminated.
                proc.terminate()
                proc.wait()
    except Exception as e:
        print(f"Unexpected error in {bin_path}: {e}")


def print_error(e):
    print(f"Error in process pool: {e}")


def check_cex(cex_path, aig_path):
    # temp deal
    return False


def safe_kill(pid):
    try:
        os.kill(pid, 0) # a touch. Not really killed
        os.kill(pid, signal.SIGTERM) # send SIGTERM
        time.sleep(0.5)
        # os.killpg(os.getpgid(pid), signal.SIGKILL)  # Use SIGKILL if SIGTERM doesn't work
    except ProcessLookupError:
        pass
    except PermissionError:
        print(f"No permission to kill process {pid}")
    except Exception as e:
        print(f"Error terminating process {pid}: {e}")


def parallel_one(config_list, filename):
    """start a process pool, pass all the args in, and stop if any one terminates.

    Args:
        config_list (list): list of configs.
    """
    with multiprocessing.Manager() as manager:
        terminate_flag = manager.Value("b", False)
        pid_list = manager.list()

        pool = multiprocessing.Pool(processes=os.cpu_count())
        for bin_path, config, tlimit, mem_limit, res_path in config_list:
            pool.apply_async(
                do_one,
                (bin_path, res_path, filename, config, tlimit, mem_limit, terminate_flag, pid_list),
                error_callback=print_error,
            )
        pool.close()
        try:
            while True:
                if terminate_flag.value:
                    for pid in pid_list:
                        safe_kill(pid)
                    pool.terminate()
                    if _verbose:
                        print("mission complete, process pool terminates")
                    break
                time.sleep(0.1)
        except Exception as e:
            print(f"Error during pool management: {e}")
            pool.terminate()
        finally:
            pool.join()
            

def file_not_empty(file_path):
    with open(file_path, 'r') as file:
        return file.read() != ''

def collect_res(tmp_dir, output_dir, case_dir):
    basename = os.path.basename(case_dir)
    filename = basename.replace(".aig", "").replace(".aag", "")

    cert_found = False
    cert_path = ""
    for dir in os.listdir(tmp_dir):
        possible_cert_path = os.path.join(tmp_dir, dir, filename + ".w.aag")
        if os.path.exists(possible_cert_path) and file_not_empty(possible_cert_path):
            cert_found = True
            cert_path = possible_cert_path
            if _verbose:
                print(f"cert found at {cert_path}")
            break

    cex_found = False
    cex_path = ""
    for dir in os.listdir(tmp_dir):
        possible_cex_path1 = os.path.join(tmp_dir, dir, filename + ".cex")
        possible_cex_path2 = os.path.join(tmp_dir, dir, filename + ".res")
        if os.path.exists(possible_cex_path1) and file_not_empty(possible_cex_path1):
            cex_found = True
            cex_path = possible_cex_path1
            if _verbose:
                print(f"cex found at {cex_path}")
            break
        elif os.path.exists(possible_cex_path2) and file_not_empty(possible_cex_path2):
            cex_found = True
            cex_path = possible_cex_path2
            if _verbose:
                print(f"cex found at {cex_path}")
            break

    if cert_found and cex_found:
        # conflict?
        # unlikely to happen
        if _verbose:
            print(f"conflict!")
        if check_cex(case_dir, cex_path):
            shutil.copyfile(cex_path, os.path.join(output_dir, filename + ".cex"))
        else:
            shutil.copyfile(cert_path, os.path.join(output_dir, filename + ".w.aag"))
    elif cex_found:
        shutil.copyfile(cex_path, os.path.join(output_dir, filename + ".cex"))
    elif cert_found:
        shutil.copyfile(cert_path, os.path.join(output_dir, filename + ".w.aag"))


@click.command()
@click.option(
    "--input", "-I", "case_dir", required=True, help="Path to input aiger file"
)
@click.option(
    "--output",
    "-O",
    "output_dir",
    required=True,
    help="Path to output cex/cert.",
)
@click.option(
    "--verbose", "-V", "verbose", default=False, help="Turn on printing", is_flag=True
)
def main(case_dir, output_dir, verbose):
    global _verbose
    _verbose = verbose

    # setting up
    case_dir = os.path.expanduser(case_dir)  # expand ~
    if not os.path.exists(case_dir):
        raise RuntimeError(f"input case is not found at {case_dir}")
    output_dir = os.path.expanduser(output_dir)
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
    tmp_dir = "_tmpRes"
    if not os.path.exists(tmp_dir):
        os.mkdir(tmp_dir)

    basename = os.path.basename(case_dir)
    filename = basename.replace(".aig", "").replace(".aag", "")

    path1 = os.path.join(tmp_dir, "mcar1")
    conf1 = (
        "./bin/MCAR",
        ["--vb", f"{case_dir}", f"{path1}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path1
    )

    path2 = os.path.join(tmp_dir, "fcarbr1")
    conf2 = (
        "./bin/simplecar",
        ["-f", "-br", "1", "-rs", "-w", f"{path2}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path2
    )

    path3 = os.path.join(tmp_dir, "bcarbr1")
    conf3 = (
        "./bin/simplecar",
        ["-b", "-br", "1", "-rs", "-w", f"{path3}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path3
    )

    path4 = os.path.join(tmp_dir, "fcarbr2")
    conf4 = (
        "./bin/simplecar",
        ["-f", "-br", "2", "-rs", "-w", f"{path4}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path4
    )

    path5 = os.path.join(tmp_dir, "bcarbr2")
    conf5 = (
        "./bin/simplecar",
        ["-b", "-br", "2", "-rs", "-w", f"{path5}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path5
    )

    path6 = os.path.join(tmp_dir, "fcarbr3")
    conf6 = (
        "./bin/simplecar",
        ["-f", "-br", "3", "-rs", "-w", f"{path6}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path6
    )

    path7 = os.path.join(tmp_dir, "bcarbr3")
    conf7 = (
        "./bin/simplecar",
        ["-b", "-br", "3", "-rs", "-w", f"{path7}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path7
    )

    path8 = os.path.join(tmp_dir, "bmc")
    conf8 = (
        "./bin/simplecar_cadical",
        ["-bmc", "-w", f"{path8}", f"{case_dir}"],
        3600,
        24 * 1024 * 1024 * 1024,  # 24GB memory limit
        path8
    )

    path9 = os.path.join(tmp_dir, "fcarcadicalbr1")
    conf9 = (
        "./bin/simplecar_cadical",
        ["-f", "-br", "1", "-rs", "-w", f"{path9}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path9
    )

    path10 = os.path.join(tmp_dir, "bcarcadicalbr1")
    conf10 = (
        "./bin/simplecar_cadical",
        ["-b", "-br", "1", "-rs", "-w", f"{path10}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path10
    )

    path11 = os.path.join(tmp_dir, "bcarcadicalbr2")
    conf11 = (
        "./bin/simplecar_cadical",
        ["-b", "-br", "2", "-rs", "-w", f"{path11}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path11
    )

    path12 = os.path.join(tmp_dir, "bcarcadicalbr3")
    conf12 = (
        "./bin/simplecar_cadical",
        ["-b", "-br", "3", "-rs", "-w", f"{path12}", f"{case_dir}"],
        3600,
        8 * 1024 * 1024 * 1024,  # 8GB memory limit
        path12
    )

    path13 = os.path.join(tmp_dir, "mcarlocal2raw")
    conf13 = (
        "./bin/MCAR",
        ["--vb","--inter","2", f"{case_dir}", f"{path13}"],
        3600,
        4 * 1024 * 1024 * 1024,  # 4GB memory limit
        path13
    )

    path14 = os.path.join(tmp_dir, "mcarlocal3raw")
    conf14 = (
        "./bin/MCAR",
        ["--vb","--inter","3", f"{case_dir}", f"{path14}"],
        3600,
        4 * 1024 * 1024 * 1024,  # 4GB memory limit
        path14
    )
    
    path15 = os.path.join(tmp_dir, "mcarmuclow3nosimp")
    conf15 = (
        "./bin/MCAR",
        ["--vb","--inter","1", "--convParam","3", f"{case_dir}", f"{path15}"],
        3600,
        4 * 1024 * 1024 * 1024,  # 4GB memory limit
        path15
    )

    config_list = [
        conf1,
        conf2,
        conf3,
        conf4,
        conf5,
        conf6,
        conf7,
        conf8,
        conf9,
        conf10,
        conf11,
        conf12,
        conf13,
        conf14,
        conf15,
    ]

    paths_list = [
        path1,
        path2,
        path3,
        path4,
        path5,
        path6,
        path7,
        path8,
        path9,
        path10,
        path11,
        path12,
        path13,
        path14,
        path15,
    ]


    for pt in paths_list:
        if not os.path.exists(pt):
            os.mkdir(pt)
        if os.path.exists(os.path.join(pt, filename+".w.aag")):
            os.remove(os.path.join(pt, filename+".w.aag"))
        if os.path.exists(os.path.join(pt, filename+".cex")):
            os.remove(os.path.join(pt, filename+".cex"))
        if os.path.exists(os.path.join(pt, filename+".res")):
            os.remove(os.path.join(pt, filename+".res"))

    # run them in parallel
    parallel_one(config_list, filename)

    # collect the result.
    collect_res(tmp_dir, output_dir, case_dir)


if __name__ == "__main__":
    main()
