from helper import *
import math, socket

def dump(tib):
    collect_mitm_partial_output(tib)
    lines = ''.join(tib.script).split('\n')

    out = [l for l in lines if 'src' in l or 'dst' in l]
    if out:
        print '\n'.join(out)

    tib.script = []

@kill_processes_tracked
def test(buf_sizes, skt_buf_sizes, break_the_pipe=False):
    print "We test what happen if the receiver is too slow\n" \
          "which it will fill the buffer up and make the receiver \n" \
          "out of sync.\n"
    print "Buf sizes (src->dst : dst->src): %s" % str(buf_sizes)
    print "Socket buf sizes (SND : RCV): %s" % str(skt_buf_sizes)
    print 
    src_port, dst_port = pair_ports()

    skt_a = socket.socket()
    skt_a.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 2048)
    skt_a.bind(("127.0.0.1", dst_port))
    skt_a.listen(1)
    
    tib = spawn_tiburoncin(src_port, dst_port, buf_sizes=buf_sizes,
                                               skt_buf_sizes=skt_buf_sizes)
    
    src_skt = socket.socket()
    src_skt.connect(("127.0.0.1", src_port))
    time.sleep(0.001)

    dst_skt, _ = skt_a.accept()
    skt_a.shutdown(2)
    skt_a.close()
    time.sleep(0.001)

    bytes_on_the_wire = 0
    for i in range(16): # 4 to fill up tib's buffer, plus 8 per socket buffer (2 in tib and 2 in each raw soket)
        src_skt.sendall(("A" * 1023) + "\n")
        bytes_on_the_wire += 1024
        dump(tib)
    
    time.sleep(1)
    dump(tib)

    print "Bytes on the wire: %i" % bytes_on_the_wire
    
    if not break_the_pipe:
        while bytes_on_the_wire > 0:
            bytes_on_the_wire -= len(dst_skt.recv(1024))
            time.sleep(0.1)
            dump(tib)
            print "Bytes on the wire: %i" % bytes_on_the_wire
    
    time.sleep(1)
    dump(tib)

    print "Closing everything"
    src_skt.shutdown(2)
    src_skt.close()
    time.sleep(5)
    dst_skt.shutdown(2)
    dst_skt.close()
    
    time.sleep(1)
    dump(tib)

    wait_for_process(tib)

    print "Tiburoncin's return code: %s" % tib.poll()
    print

if __name__ == '__main__':
    test(buf_sizes=(4096, 4096), skt_buf_sizes=(2048, 2048))
    test(buf_sizes=(4096, 4096), skt_buf_sizes=(2048, 2048), break_the_pipe=True)

