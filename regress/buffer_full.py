from helper import *
import math

@kill_processes_tracked
def test(buf_sizes):
    print "We want to test what happen if the buffer is too short\n" \
          "or it is full. We expect that the sender will block waiting\n" \
          "for tiburoncin to flush its buffer to make more room for the\n" \
          "incoming data.\n"

    print "Buf sizes (A->B : B->A): %s" % str(buf_sizes)
    print
    A_port, B_port = pair_ports()

    B = spawn_netcat(B_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(A_port, B_port, buf_sizes=buf_sizes)
    buf_sizes = tib.buf_sizes

    A = spawn_netcat(A_port, listen_mode=False)
    time.sleep(0.001)

    send(A, B, tib, ("A" * 3) + "\n")
    send(B, A, tib, ("B" * 3) + "\n")

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
    print

if __name__ == '__main__':
    test(buf_sizes=(1, 2048))
    test(buf_sizes=(2, 2048))
    test(buf_sizes=(4, 2048))
    test(buf_sizes=3)
