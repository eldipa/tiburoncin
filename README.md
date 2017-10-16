# tiburoncin

Small man in the middle tool to inspect the traffic between two endpoints.
Mostly to be used for students wanting to know what data is flowing in a TCP channel.

## Usage

First, run the "server"

```shell
nc -l -p 8082 127.0.0.1
```

Then start `tiburoncin`

```shell
./tiburoncin -s 127.0.0.1:8081 -d 127.0.0.1:8082
```

And finally start the "client"

```shell
nc 127.0.0.1 8081
```

Use your server/client as you wish, all the traffic flowing in the channel will be captured and displayed by `tiburoncin`.

The following diagram depicts what's going on:

```
+-----------------+           +--------------------------------+
|    <client>     |      8081 |      <man in the middle>       |
| nc 127.0.0.1 \  |===========| tiburoncin -s 127.0.0.1:8081 \ |
|    8081         |           |            -d 127.0.0.1:8082   |
+-----------------+           +--------------------------------+
                                             || 
+-----------------+                          ||
|    <server>     | 8082                     ||
| nc -l -p 8082 \ |==========================//
|   127.0.0.      |           
+-----------------+      
```

### Capture example

```
Connecting to dst 127.0.0.1:8082...
Waiting for a connection from src 127.0.0.1:8081...
Allocating buffers: 2048 and 2048 bytes...
src -> dst sent 29 bytes
00000000  49 27 6d 20 74 68 65 20  63 6c 69 65 6e 74 20 73  |I'm the client s|
00000010  65 6e 64 69 6e 67 20 27  68 69 21 27 0a           |ending 'hi!'.   |
dst is 29 bytes behind
dst is in sync
dst -> src sent 45 bytes
00000000  41 6e 64 20 49 27 6d 20  74 68 65 20 73 65 72 76  |And I'm the serv|
00000010  65 72 20 73 61 79 69 6e  67 20 27 68 69 20 61 6e  |er saying 'hi an|
00000020  64 20 67 6f 6f 64 20 62  79 65 21 27 0a           |d good bye!'.   |
src is 45 bytes behind
src is in sync
src is in sync
dst is in sync
```

### Help

```
./tiburoncin -s <src> -d <dst> [-b <bsz>] [-z <bsz>] [-o]
 where <src> and <dst> can be of the form:
  - host:serv
  - :serv
  - serv
 If it is not specified, host will be localhost.
 In all the cases the host can be a hostname or an IP;
 for the service it can be a servicename or a port number.
 
 -b <bsz> sets the buffer size of tiburocin
 where <bsz> is a size in bytes of the form:
  - num      sets the size of both buffers to that value
  - num:num  sets sizes for src->dst and dst->src buffers
 by default, both buffers are of 2048 bytes
 
 -z <bsz> sets the buffer size of the sockets
 where <bsz> is a size in bytes of the form:
  - num      sets the size of both buffers SND and RCV to that value
  - num:num  sets sizes for SND and RCV buffers
 by default, both buffers are not changed. See man socket(7)
 
 -o save the received data onto two files:
  dump.stod for the data received from src
  dump.dtos for the data received from dst
 in both cases a raw hexdump is saved which can be recovered later
 running 'xxd -p -c 16 -r <raw hexdump file>'. See man xxd(1)
```

## How to compile it

You will need `make` and `gcc` with support for `c99`. Then just run:

```shell
make compile
```

### Run the tests

```shell
make test
```

## How to contribute

Make a fork and start to hack. 
Keep your code as clean as possible and following the same coding style that the rest of the code and making small commits.
When you finish, do a Pull Request.
