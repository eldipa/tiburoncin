# tiburoncin

Small man in the middle tool to inspect the traffic between two endpoints A and B.
Mostly used by students wanting to know what data is flowing in a TCP channel.

## Usage

First, run the "server"

```shell
nc -l -p 8082 127.0.0.1                              # byexample: +skip
```

Then start ``tiburoncin``

```shell
./tiburoncin -A 127.0.0.1:8081 -B 127.0.0.1:8082     # byexample: +skip
```

And finally start the "client"

```shell
nc 127.0.0.1 8081                                    # byexample: +skip
```

Use your server/client as you wish, all the traffic flowing in
the channel will be captured and displayed by ``tiburoncin``.

The following diagram depicts what's going on:

```
+-----------------+           +--------------------------------+
|  <client A>     |      8081 |      <man in the middle>       |
| nc 127.0.0.1 \  |===========| tiburoncin -A 127.0.0.1:8081 \ |
|    8081         |           |            -B 127.0.0.1:8082   |
+-----------------+           +--------------------------------+
                                             ||
                                             ||
+-----------------+                          ||
|  <server B>     | 8082                     ||
| nc -l -p 8082 \ |==========================//
|   127.0.0.      |
+-----------------+
```

### Capture example

```
Connecting to B 127.0.0.1:8082...
Waiting for a connection from A 127.0.0.1:8081...
Allocating buffers: 2048 and 2048 bytes...
A -> B sent 29 bytes
00000000  49 27 6d 20 74 68 65 20  63 6c 69 65 6e 74 20 73  |I'm the client s|
00000010  65 6e 64 69 6e 67 20 27  68 69 21 27 0a           |ending 'hi!'.   |
B is 29 bytes behind
B is in sync
B -> A sent 45 bytes
00000000  41 6e 64 20 49 27 6d 20  74 68 65 20 73 65 72 76  |And I'm the serv|
00000010  65 72 20 73 61 79 69 6e  67 20 27 68 69 20 61 6e  |er saying 'hi an|
00000020  64 20 67 6f 6f 64 20 62  79 65 21 27 0a           |d good bye!'.   |
A is 45 bytes behind
A is in sync
A is in sync
B is in sync
```

### Help

```shell
$ ./tiburoncin -h                             # byexample: -tags +rm=~ +norm-ws
tiburoncin
==========
~
Small man in the middle tool to inspect the traffic between two endpoints A and B.
Mostly to be used for students wanting to know what data is flowing in a TCP channel.
~
Author: Martin Di Paola
URL: https://github.com/eldipa/tiburoncin
License: GPLv3
Version: 2.2.0
~
./tiburoncin -A <addr> -B <addr> [-b <bsz>] [-z <bsz>] [-o | -f <prefix>] [-c]
 where <addr> can be of the form:
  - host:serv
  - :serv
  - serv
 If it is not specified, host will be localhost.
 In all the cases the host can be a hostname or an IP;
 for the service it can be a servicename or a port number.
~
 -b <bsz> sets the buffer size of tiburocin
 where <bsz> is a size in bytes of the form:
  - num      sets the size of both buffers to that value
  - num:num  sets sizes for A->B and B->A buffers
 by default, both buffers are of 2048 bytes
~
 -z <bsz> sets the buffer size of the sockets
 where <bsz> is a size in bytes of the form:
  - num      sets the size of both buffers SND and RCV to that value
  - num:num  sets sizes for SND and RCV buffers
 by default, both buffers are not changed. See man socket(7)
~
 -o save the received data onto two files:
  AtoB.dump for the data received from A
  BtoA.dump for the data received from B
 in both cases a raw hexdump is saved which can be recovered later
 running 'xxd -p -c 16 -r <raw hexdump file>'. See man xxd(1)
 This option is incompatible with -f option
~
 -f <prefix> same as -o, but it pre-concatenates the specified prefix
 This option is incompatible with -o option
~
 -c disable the color in the output (colorless)

```

## How to compile it

You will need ``make`` and ``gcc`` with support for ``c99``. Then just run:

```shell
make compile                           # byexample: +skip
```

### Run the tests

For this you will need [python](https://www.python.org/downloads/),
[cling](https://github.com/root-project/cling) and
[byexample](https://byexamples.github.io/byexample/).

```shell
make test                              # byexample: +skip
```

## How to contribute

Make a fork and start to hack.
Keep your code as clean as possible and following the same coding style.
Make small commits and when you finish, do a Pull Request.
