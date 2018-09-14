
<!--
Import some helper tools
>>> from helper import pair_ports, netcat

Pick two random ports
>>> pair_ports()                            # byexample: +fail-fast
(<port-a>, <port-b>)

Alias
$ alias tiburoncin=../tiburoncin

-->

First, we set up our server listening in the given port

```python
>>> B = netcat(listen_on = <port-b>)        # byexample: +paste

```

Then we run ``tiburoncin`` so he can connect to that port and listen
in a second one

```shell
$ tiburoncin -A 127.0.0.1:<port-a> -B 127.0.0.1:<port-b> -c     # byexample: +paste +stop-on-silence +timeout=1
Connecting to B 127.0.0.1:<port-b>...
Waiting for a connection from A 127.0.0.1:<port-a>...

```

And finally we connect our client to ``tiburoncin``

```python
>>> A = netcat(connect_to = <port-a>)       # byexample: +paste

```

<!--
Accept the connection and close the circuit
>>> B.accept()  # byexample: +fail-fast

-->

Now we can send some data from ``A`` to ``B`` and/or vice versa

```python
>>> A.send("hello\n")

```

All the bytes sent in one or in the other direction will be recorded
by ``tiburoncin`` acting as a *man in the middle*:

```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
Allocating buffers: 2048 and 2048 bytes...
A -> B sent 6 bytes
00000000  68 65 6c 6c 6f 0a                                 |hello.          |
B is 6 bytes behind
B is in sync

```

```python
>>> B.send("hi there!\n")

```

```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
B -> A sent 10 bytes
00000000  68 69 20 74 68 65 72 65  21 0a                    |hi there!.      |
A is 10 bytes behind
A is in sync

```

Notice how ``tiburoncin`` shows the bytes in hexadecimal and in ASCII like
``hexdump`` does keeping track of how much bytes were sent.

If we keep sending data we will see it more clearly:

```python
>>> B.send("how are you?\n")

```

```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
B -> A sent 13 bytes
0000000a                                 68 6f 77 20 61 72  |          how ar|
00000010  65 20 79 6f 75 3f 0a                              |e you?.         |
A is 13 bytes behind
A is in sync

```

In the example above, you can see:

 - how many bytes were sent at that moment (``B -> A`` sent 13 bytes)
 - and how many bytes were sent before the message (``0000000a``, 10 bytes)
 - the hexdump of the message sent
 - for how many bytes the parties are out of sync (``A`` is 13 bytes behind)
 - then, when ``A`` reads the message from ``tiburoncin`` he gets on sync (``A`` is in sync)

```python
>>> A.send("very good, thanks for asking\n")
>>> B.send("good to hear that, bye\n")

```

```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
A -> B sent 29 bytes
00000006                    76 65  72 79 20 67 6f 6f 64 2c  |      very good,|
00000010  20 74 68 61 6e 6b 73 20  66 6f 72 20 61 73 6b 69  | thanks for aski|
00000020  6e 67 0a                                          |ng.             |
B is 29 bytes behind
B -> A sent 23 bytes
00000017                       67  6f 6f 64 20 74 6f 20 68  |       good to h|
00000020  65 61 72 20 74 68 61 74  2c 20 62 79 65 0a        |ear that, bye.  |
A is 23 bytes behind
B is in sync
A is in sync

```

The above repeats for all the message that ``A`` and ``B`` share.

Finally, can shutdown the communication

```python
>>> A.shutdown()                            # byexample: -skip
>>> B.shutdown()                            # byexample: -skip

```

And ``tiburoncin`` will shutdown gracefully

```shell
$ fg
<...>tiburoncin <...>
A -> B flow shutdown
B is in sync
B -> A flow shutdown
A is in sync

$ echo $?
0

```

<!--
$ kill %% ; wait                           # byexample: -skip +pass

-->
