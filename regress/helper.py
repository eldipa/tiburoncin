import subprocess, os
import random, time, math, fcntl

_processes = []

def process_track(func):
    global _processes

    def wrapper(*args, **kargs):
        proc = func(*args, **kargs)
        _processes.append(proc)
        return proc

    return wrapper

original_time_sleep = time.sleep
def scale_sleep_time(factor):
    def scaled_sleep(s):
        global original_time_sleep
        return original_time_sleep(s * factor)

    time.sleep = scaled_sleep

def kill_processes_tracked(func):
    global _processes

    def wrapper(*args, **kargs):
        try:
            return func(*args, **kargs)
        finally:
            for proc in _processes:
                if proc.poll() is None:
                    try:
                        proc.terminate()
                    except:
                        pass

            time.sleep(0.001)
            for proc in _processes:
                if proc.poll() is None:
                    try:
                        proc.kill()
                    except:
                        pass
                    proc.wait()

    return wrapper


@process_track
def spawn_netcat(port, listen_mode, receive_buf_sz=None):
    if listen_mode:
        cmd = ["nc", "-q", "1", "-l", "-p", str(port), "-s", "127.0.0.1"]
    else:
        cmd = ["nc", "-q", "1", "127.0.0.1", str(port)]

    if receive_buf_sz:
        cmd.append("-I")
        cmd.append(str(receive_buf_sz))

    else:
        receive_buf_sz = 2048

    proc = subprocess.Popen(cmd, shell=False,
                     stdout=subprocess.PIPE,
                     stdin=subprocess.PIPE,
                     stderr=open(os.devnull, 'r'))

    proc.script = []
    proc.receive_buf_sz = receive_buf_sz
    return proc


@process_track
def spawn_tiburoncin(src_port, dst_port, buf_sizes=None, skt_buf_sizes=None, to_file=False):
    cmd = ["./tiburoncin", "-s", "127.0.0.1:"+str(src_port), 
                           "-d", "127.0.0.1:"+str(dst_port)]

    if buf_sizes is not None:
        try:
            parm = "-b %i:%i" % buf_sizes
        except:
            parm = "-b %i" % buf_sizes
            buf_sizes = (buf_sizes, buf_sizes)

        cmd.append(parm)
    else:
        buf_sizes = (2048, 2048)

    if skt_buf_sizes is not None:
        try:
            parm = "-z %i:%i" % skt_buf_sizes
        except:
            parm = "-z %i" % skt_buf_sizes

        cmd.append(parm)

    if to_file:
        cmd.append("-o")
 
    proc = subprocess.Popen(cmd, shell=False, 
                     stdout=subprocess.PIPE,
                     stdin=open(os.devnull),
                     stderr=subprocess.STDOUT)
    proc.script = []
    proc.buf_sizes = buf_sizes
    
    fd = proc.stdout.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    time.sleep(1)
    return proc


def wait_for_process(proc, timeout=10):
    elapsed = 0
    while proc.poll() is None and elapsed < timeout:
        time.sleep(0.1)
        elapsed += 0.1

    if proc.poll() is None:
        raise Exception("timeout")

def collect_mitm_partial_output(mitm):
    tries = 10
    while tries > 0:
        try:
            chunk = mitm.stdout.read(80)
        except IOError:
            chunk = None

        if chunk:
            mitm.script.append(chunk)
            tries = 3
        else:
            time.sleep(0.01)
            tries -= 1


def send(sender, receiver, mitm, msg):
    assert msg[-1] == '\n'

    sender.script.append("> ")
    sender.script.append(msg)
    sender.stdin.write(msg);

    nlines = msg.count('\n')

    collect_mitm_partial_output(mitm)

    if receiver:
        for n in range(nlines):
            line = receiver.stdout.readline()
            receiver.script.append(": ")
            receiver.script.append(line)


def pair_ports():
    return (random.randint(1000, 32000),
            random.randint(1000, 32000))

def dump(proc_or_str, title):
    if hasattr(proc_or_str, 'script'):
        d = ''.join(proc_or_str.script)
        proc_or_str.script = []

    else:
        d = proc_or_str

    print(title)
    print(d)



