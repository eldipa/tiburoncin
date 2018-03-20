from helper import *

@kill_processes_tracked
def test():
    src_port, dst_port = pair_ports()

    dst = spawn_netcat(dst_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(src_port, dst_port, to_file=True)

    src = spawn_netcat(src_port, listen_mode=False)
    time.sleep(0.001)

    for i in range(10):
        send(src, dst, tib, ("A" * 12) + "\n")
        send(dst, src, tib, ("B" * 7) + "\n")

    dst.stdin.close()
    src.stdin.close()

    wait_for_process(src)
    wait_for_process(dst)
    wait_for_process(tib)

    print "Tiburoncin's return code: %s" % tib.poll()
    print "src to dst dump:"
    with open('dump.stod', 'rt') as s:
        print s.read()

    print "dst to src dump:"
    with open('dump.dtos', 'rt') as s:
        print s.read()

    print
    dump(tib, "Tiburoncin's point of view")

    os.remove('dump.stod')
    os.remove('dump.dtos')


if __name__ == '__main__':
    test()

