import os
import subprocess
import multiprocessing
import resource
import shutil
import click

_verbose = False

def check_condition():
    # To implement later.
    return True

def set_memory_limit(memory_limit):
    # Set memory limit in bytes
    resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))


def do_one(bin_path, config, timeout, memory_limit, terminate_flag):
    cmd_args = [bin_path] + config
    if _verbose:
        print(f"begin : {cmd_args}")
    try:
        # Set memory limit
        set_memory_limit(memory_limit)

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
        if _verbose:
            print(f"end : {cmd_args}")
    except subprocess.TimeoutExpired:
        pass
    except subprocess.CalledProcessError as e:
        print(f"Error in {bin_path}: {e}")


def print_error(e):
    print(e)


def check_cex(cex_path, aig_path):
    # temp deal
    return False

def parallel_one(config_list):
    """start a process pool, pass all the args in, and stop if any one terminates.

    Args:
        config_list (list): list of configs. 
    """
    with multiprocessing.Manager() as manager:
        terminate_flag = manager.Value("b", False)
        
        pool = multiprocessing.Pool(processes=os.cpu_count())
        for bin_path, config, tlimit, mem_limit in config_list:
            pool.apply_async(
                do_one,
                (bin_path, config, tlimit, mem_limit, terminate_flag),
                error_callback=print_error,
            )
        pool.close()
        while True:
            if terminate_flag.value:
                if _verbose:
                    print("mission complete, process pool terminates")
                pool.terminate()
                break

def collect_res(tmp_dir, output_dir, case_dir):
    basename = os.path.basename(case_dir)
    filename = basename.replace(".aig","").replace(".aag","")

    cert_found = False
    cert_path = ""
    for dir in os.listdir(tmp_dir):
        if os.path.exists(os.path.join(tmp_dir,dir,filename+".w.aag")):
            cert_found = True
            cert_path = os.path.join(tmp_dir,dir,filename+".w.aag")
            if _verbose:
                print(f"cert found at {cert_path}")
            break

    cex_found = False
    cex_path = ""
    for dir in os.listdir(tmp_dir):
        if os.path.exists(os.path.join(tmp_dir,dir,filename+".cex")):
            cex_found = True
            cex_path = os.path.join(tmp_dir,dir,filename+".cex")
            if _verbose:
                print(f"cex found at {cex_path}")
            break
        elif os.path.exists(os.path.join(tmp_dir,dir,filename+".res")):
            cex_found = True
            cex_path = os.path.join(tmp_dir,dir,filename+".res")
            if _verbose:
                print(f"cex found at {cex_path}")
            break

    if cert_found and cex_found:
        # conflict?
        # unlikely to happen     
        if _verbose:
            print(f"conflict!")
        if check_cex(case_dir, cex_path):
            shutil.copyfile(cex_path,os.path.join(output_dir, filename+".cex"))
        else:
            shutil.copyfile(cert_path,os.path.join(output_dir, filename+".w.aag"))
    elif cex_found:
        shutil.copyfile(cex_path,os.path.join(output_dir, filename+".cex"))
    else:
        shutil.copyfile(cert_path,os.path.join(output_dir, filename+".w.aag"))
        
@click.command()
@click.option("--input","-I","case_dir",    required=True,  help="Path to input aiger file")
@click.option("--output","-O","output_dir", required=True,  help="Path to output cex/cert. Please make sure it's empty.")
@click.option("--verbose","-V","verbose",   default=False,  help="Turn on printing", is_flag=True)
def main(case_dir, output_dir,verbose):
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
    if os.path.exists(tmp_dir):
        shutil.rmtree(tmp_dir)
    os.mkdir(tmp_dir)

    path1 = os.path.join(tmp_dir, 'mcar1')
    conf1 = ("./bin/MCAR", ["--vb", f"{case_dir}", f"{path1}"], 
            3600,
            8 * 1024 * 1024 * 1024  # 8GB memory limit
            )
              
    path2 = os.path.join(tmp_dir, 'fcar1')
    conf2 = (
            "./bin/simplecar",
            ["-v", "1", "-f", "-br", "1", "-rs", "-w", f"{path2}", f"{case_dir}"],
            3600,
            8 * 1024 * 1024 * 1024  # 8GB memory limit
        )

    path3 = os.path.join(tmp_dir, 'bcar1')
    conf3 = (
            "./bin/simplecar",
            ["-v", "1", "-b", "-br","1", "-rs", "-w", f"{path3}", f"{case_dir}"],
            3600,
            8 * 1024 * 1024 * 1024  # 8GB memory limit
        )
    
    path4 = os.path.join(tmp_dir, 'bmc1')
    conf4 = (
            "./bin/simplecar",
            ["-v", "1", "-bmc", "-w", f"{path4}", f"{case_dir}"],
            3600,
            16 * 1024 * 1024 * 1024  # 16GB memory limit
        )

    config_list = [
        conf1, 
        conf2,
        conf3,
        conf4
        # ..
    ]

    paths_list = [
        path1, 
        path2, 
        path3, 
        path4
    ]

    for pt in paths_list:
        os.mkdir(pt)
    
    # run them in parallel
    parallel_one(config_list)

    # collect the result.
    collect_res(tmp_dir, output_dir, case_dir)

if __name__ == "__main__":
    main()