from helper import *

@kill_processes_tracked
def test_sender_close_pipe():
    print "Close the pipe from the sender side after all the data was sent."
    A_port, B_port = pair_ports()

    B = spawn_netcat(B_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(A_port, B_port)

    A = spawn_netcat(A_port, listen_mode=False)
    time.sleep(0.001)

    send(A, B, tib, "hello\n")

    print "Closing A..."
    A.stdin.close()

    wait_for_process(A)
    wait_for_process(tib)
    wait_for_process(B)

    print "All processes finished."
    B.stdin.close()

    dump(A, "Src's point of view")
    print
    dump(B, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    print "Tiburoncin's return code: %s" % tib.poll()

@kill_processes_tracked
def test_receiver_close_pipe():
    print "Close the pipe from the receiver side after all the data was received."
    A_port, B_port = pair_ports()

    B = spawn_netcat(B_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(A_port, B_port)

    A = spawn_netcat(A_port, listen_mode=False)
    time.sleep(0.001)

    send(A, B, tib, "hello\n")

    print "Closing B..."
    B.stdin.close()

    wait_for_process(A)
    wait_for_process(tib)
    wait_for_process(B)

    print "All processes finished."
    A.stdin.close()

    dump(A, "Src's point of view")
    print
    dump(B, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    print "Tiburoncin's return code: %s" % tib.poll()


if __name__ == '__main__':
    test_sender_close_pipe()
    test_receiver_close_pipe()
