from helper import *

@kill_processes_tracked
def test():
    print "The idea is to send some data back and forward\n" \
          "and check if all the data is forwarded by tiburoncin\n" \
          "and the correct hexdump is shown for the data sent.\n"
    src_port, dst_port = pair_ports()

    dst = spawn_netcat(dst_port, listen_mode=True)
    time.sleep(0.001)
    
    tib = spawn_tiburoncin(src_port, dst_port)
    
    src = spawn_netcat(src_port, listen_mode=False)
    time.sleep(0.001)

    send(src, dst, tib, "hello\n")

    send(dst, src, tib, "hi there!\n")
    send(dst, src, tib, "how are you?\n")
    
    send(src, dst, tib, "very good, thanks for asking\n")
    
    send(dst, src, tib, "good to hear that, bye\n")
    
    send(src, dst, tib, "bye!\n")
    
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


if __name__ == '__main__':
    import sys
    factor = 1 if len(sys.argv) == 1 else int(sys.argv[1])
    scale_sleep_time(factor)
    test()
