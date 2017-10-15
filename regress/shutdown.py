from helper import *

@kill_processes_tracked
def test_sender_close_pipe():
    print "Close the pipe from the sender side after all the data was sent."
    src_port, dst_port = pair_ports()

    dst = spawn_netcat(dst_port, listen_mode=True)
    time.sleep(0.001)
    
    tib = spawn_tiburoncin(src_port, dst_port)
    
    src = spawn_netcat(src_port, listen_mode=False)
    time.sleep(0.001)

    send(src, dst, tib, "hello\n")

    print "Closing src..."
    src.stdin.close()

    wait_for_process(src)
    wait_for_process(tib)
    wait_for_process(dst)
    
    print "All processes finished."
    dst.stdin.close()

    dump(src, "Src's point of view")
    print
    dump(dst, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    print "Tiburoncin's return code: %s" % tib.poll()

@kill_processes_tracked
def test_receiver_close_pipe():
    print "Close the pipe from the receiver side after all the data was received."
    src_port, dst_port = pair_ports()

    dst = spawn_netcat(dst_port, listen_mode=True)
    time.sleep(0.001)
    
    tib = spawn_tiburoncin(src_port, dst_port)
    
    src = spawn_netcat(src_port, listen_mode=False)
    time.sleep(0.001)

    send(src, dst, tib, "hello\n")

    print "Closing dst..."
    dst.stdin.close()

    wait_for_process(src)
    wait_for_process(tib)
    wait_for_process(dst)
    
    print "All processes finished."
    src.stdin.close()

    dump(src, "Src's point of view")
    print
    dump(dst, "Dst's point of view")
    print
    dump(tib, "Tiburoncin's point of view")

    print "Tiburoncin's return code: %s" % tib.poll()


if __name__ == '__main__':
    test_sender_close_pipe()
    test_receiver_close_pipe()
