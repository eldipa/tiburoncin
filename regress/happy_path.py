from helper import *

@kill_processes_tracked
def test():
    print "The idea is to send some data back and forward\n" \
          "and check if all the data is forwarded by tiburoncin\n" \
          "and the correct hexdump is shown for the data sent.\n"
    A_port, B_port = pair_ports()

    B = spawn_netcat(B_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(A_port, B_port)

    A = spawn_netcat(A_port, listen_mode=False)
    time.sleep(0.001)

    send(A, B, tib, "hello\n")

    send(B, A, tib, "hi there!\n")
    send(B, A, tib, "how are you?\n")

    send(A, B, tib, "very good, thanks for asking\n")

    send(B, A, tib, "good to hear that, bye\n")

    send(A, B, tib, "bye!\n")

    B.stdin.close()
    A.stdin.close()

    dump(A, "Src's point of view")
    print
    dump(B, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    wait_for_process(A)
    wait_for_process(B)
    wait_for_process(tib)

    print "Tiburoncin's return code: %s" % tib.poll()


if __name__ == '__main__':
    import sys
    factor = 1 if len(sys.argv) == 1 else int(sys.argv[1])
    scale_sleep_time(factor)
    test()
