from helper import *

@kill_processes_tracked
def test():
    A_port, B_port = pair_ports()

    B = spawn_netcat(B_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(A_port, B_port, to_file=True)

    A = spawn_netcat(A_port, listen_mode=False)
    time.sleep(0.001)

    for i in range(10):
        send(A, B, tib, ("A" * 12) + "\n")
        send(B, A, tib, ("B" * 7) + "\n")

    B.stdin.close()
    A.stdin.close()

    wait_for_process(A)
    wait_for_process(B)
    wait_for_process(tib)

    print "Tiburoncin's return code: %s" % tib.poll()
    print "A to B dump:"
    with open('AtoB.dump', 'rt') as s:
        print s.read()

    print "B to A dump:"
    with open('BtoA.dump', 'rt') as s:
        print s.read()

    print
    dump(tib, "Tiburoncin's point of view")

    os.remove('AtoB.dump')
    os.remove('BtoA.dump')


if __name__ == '__main__':
    test()

