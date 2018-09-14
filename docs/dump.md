
<!--
Import some helper tools
>>> from helper import pair_ports, netcat

Pick two random ports
>>> pair_ports()                            # byexample: +fail-fast
(<port-a>, <port-b>)

Alias
$ alias tiburoncin=../tiburoncin

Clean up first
$ rm -f AtoB.dump BtoA.dump                 # byexample: +fail-fast

-->

``tiburoncin`` allows to dump to a file the data sent in a hexdump format
using the ``-o`` flag.

To see this, let's set up ``tiburoncin`` and the two endpoints as usual:

```python
>>> B = netcat(listen_on = <port-b>)        # byexample: +paste

```

```shell
$ tiburoncin -A 127.0.0.1:<port-a> -B 127.0.0.1:<port-b> -c -o    # byexample: +paste +stop-on-silence +timeout=1
Connecting to B 127.0.0.1:<port-b>...
Waiting for a connection from A 127.0.0.1:<port-a>...

```

```python
>>> A = netcat(connect_to = <port-a>)       # byexample: +paste

```

<!--
Accept the connection and close the circuit
>>> B.accept()  # byexample: +fail-fast

-->

Now let's send some data back and forward

```python
>>> A.send('hello')
>>> B.send('hi!')

>>> A.shutdown()                            # byexample: -skip
>>> B.shutdown()                            # byexample: -skip

```

As usual, ``tiburoncin`` will print to stdout the data sent
```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
Allocating buffers: 2048 and 2048 bytes...
A -> B sent 5 bytes
00000000  68 65 6c 6c 6f                                    |hello           |
B is 5 bytes behind
B -> A sent 3 bytes
00000000  68 69 21                                          |hi!             |
A is 3 bytes behind
A -> B flow shutdown
B is in sync
B -> A flow shutdown
A is in sync

```

But it also it created two additional files with the data sent in both flows

```shell
$ cat AtoB.dump
68656c6c6f

$ cat BtoA.dump
686921

```

The dumps are crude hexadecimal representation of the bytes sent. It is not
intended to be read by a human but to be read by ``xxd``.

To get the ASCII part:

```shell
$ xxd -p -c 16 -r AtoB.dump
hello

```

Or to get a more easy to read hexdump:

```shell
$ xxd -p -c 16 -r BtoA.dump | hexdump -C
00000000  68 69 21                                          |hi!|
00000003

```

<!--
$ kill %% ; wait                           # byexample: -skip +pass
$ rm -f AtoB.dump BtoA.dump                # byexample: -skip +pass

-->
