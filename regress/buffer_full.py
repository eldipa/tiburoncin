from helper import *
import math

@kill_processes_tracked
def test(buf_sizes):
    print "We want to test what happen if the buffer is too short\n" \
          "or it is full. We expect that the sender will block waiting\n" \
          "for tiburoncin to flush its buffer to make more room for the\n" \
          "incoming data.\n"

    print "Buf sizes (src->dst : dst->src): %s" % str(buf_sizes)
    print
    src_port, dst_port = pair_ports()

    dst = spawn_netcat(dst_port, listen_mode=True)
    time.sleep(0.001)

    tib = spawn_tiburoncin(src_port, dst_port, buf_sizes=buf_sizes)
    buf_sizes = tib.buf_sizes

    src = spawn_netcat(src_port, listen_mode=False)
    time.sleep(0.001)

    send(src, dst, tib, ("A" * 3) + "\n")
    send(dst, src, tib, ("B" * 3) + "\n")

    dst.stdin.close()
    src.stdin.close()

    dump(src, "Src's point of view")
    print
    dump(dst, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    wait_for_process(src)
    wait_for_process(dst)
    wait_for_process(tib)

    print "Tiburoncin's return code: %s" % tib.poll()
    print

if __name__ == '__main__':
    test(buf_sizes=(1, 2048))
    test(buf_sizes=(2, 2048))
    test(buf_sizes=(4, 2048))
    test(buf_sizes=3)
