import socket, time, random

#   proc = subprocess.Popen(cmd, shell=False,
#                    stdout=subprocess.PIPE,
#                    stdin=open(os.devnull),
#                    stderr=subprocess.STDOUT)
#
#   fd = proc.stdout.fileno()
#   fl = fcntl.fcntl(fd, fcntl.F_GETFL)
#   fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)


def pair_ports():
    return (random.randint(1000, 32000),
            random.randint(1000, 32000))

class netcat:
    def __init__(self, listen_on=None, connect_to=None, rcv_buf=2048):
        # one and only one
        assert (listen_on == None and connect_to != None) or \
               (listen_on != None and connect_to == None)

        self.skt = socket.socket()
        self.rcv_buf = rcv_buf
        self._conf_socket()

        self.flows = [socket.SHUT_RD, socket.SHUT_WR]
        if listen_on != None:
            self.skt.bind(('127.0.0.1', int(listen_on)))
            self.skt.listen(1)

        else:
            self.skt.connect(('127.0.0.1', int(connect_to)))

    def accept(self):
        s = self.skt.accept()
        self.skt.shutdown(socket.SHUT_RDWR)
        self.skt.close()

        self.skt = s[0]
        self._conf_socket()

    def _conf_socket(self):
        self.skt.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.skt.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, self.rcv_buf)

    def send(self, msg):
        msg = bytes(msg, 'ascii')
        self.skt.sendall(msg)

    def consume(self, n):
        while n > 0:
            s = len(self.skt.recv(n))
            if s > 0:
                n -= s
            else:
                return s

        return None # yes, none

    def shutdown(self, how=None):
        if how == None or how == socket.SHUT_RDWR:
            for f in self.flows:
                self.skt.shutdown(f)

            self.flows = []
        else:
            self.flows.remove(how)
            self.skt.shutdown(how)

        if not self.flows:
            self.skt.close()

