
<!--
Import some helper tools
>>> from helper import pair_ports, netcat, check_transfer

Pick two random ports
>>> pair_ports()                            # byexample: +fail-fast
(<port-a>, <port-b>)

>>> pair_ports()                            # byexample: +fail-fast
(<port-c>, <port-d>)

>>> pair_ports()                            # byexample: +fail-fast
(<port-e>, <port-f>)

Alias
$ alias tiburoncin=../tiburoncin

Clean up first
$ rm -f AtoB.dump BtoA.dump zazAtoB.dump zazBtoA.dump  # byexample: +fail-fast
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

<!--
>>> check_transfer(src=A, dst=B)
0 bytes transferred correctly.
subsequent 5 bytes were sent but not received (lost).

>>> check_transfer(src=B, dst=A)
0 bytes transferred correctly.
subsequent 3 bytes were sent but not received (lost).
-->

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

## Binary

What happen if we send binary instead of letters?

```python
>>> B = netcat(listen_on = <port-d>)        # byexample: +paste
```

```shell
$ tiburoncin -A 127.0.0.1:<port-c> -B 127.0.0.1:<port-d> -c -o    # byexample: +paste +stop-on-silence +timeout=1
Connecting to B 127.0.0.1:<port-d>...
Waiting for a connection from A 127.0.0.1:<port-c>...
```

```python
>>> A = netcat(connect_to = <port-c>)       # byexample: +paste
```

<!--
Accept the connection and close the circuit
>>> B.accept()  # byexample: +fail-fast
-->

Now let's send some data back and forward in binary. Use lower
and higher bytes to stress the *signess*:

```python
>>> A.send(b'\x01\x02')
>>> B.send(b'\xff\xfe')

>>> A.shutdown()                            # byexample: -skip
>>> B.shutdown()                            # byexample: -skip
```

```shell
$ fg                                        # byexample: +stop-on-silence +timeout=1
<...>tiburoncin <...>
Allocating buffers: 2048 and 2048 bytes...
A -> B sent 2 bytes
00000000  01 02                                             |..              |
B is 2 bytes behind
B -> A sent 2 bytes
00000000  ff fe                                             |..              |
A is 2 bytes behind
A -> B flow shutdown
B is in sync
B -> A flow shutdown
A is in sync
```

<!--
>>> check_transfer(src=A, dst=B)
0 bytes transferred correctly.
subsequent 2 bytes were sent but not received (lost).

>>> check_transfer(src=B, dst=A)
0 bytes transferred correctly.
subsequent 2 bytes were sent but not received (lost).
-->

```shell
$ xxd -p -c 16 -r AtoB.dump | hexdump -C
00000000  01 02                                             |..|
00000002

$ xxd -p -c 16 -r BtoA.dump | hexdump -C
00000000  ff fe                                             |..|
00000002
```

## Prefix names

`tiburoncin` allows you to prefix the filenames of the dumps.

This can
be useful if you want to run several `tiburoncin` instances in parallel
and you want to get a different dump for each run.

```python
>>> B = netcat(listen_on = <port-e>)        # byexample: +paste
```

```shell
$ tiburoncin -A 127.0.0.1:<port-f> -B 127.0.0.1:<port-e> -c -f zaz    # byexample: +paste +stop-on-silence
Connecting to B 127.0.0.1:<port-e>...
Waiting for a connection from A 127.0.0.1:<port-f>...
```

Note that `-f <prefix>` and `-o` options are mutually exclusive (`-f`
implies `-o` in any case).

```python
>>> A = netcat(connect_to = <port-f>)       # byexample: +paste
```

<!--
Accept the connection and close the circuit
>>> B.accept()  # byexample: +fail-fast
-->


```python
>>> A.send(b'foo')
>>> B.send(b'bahbah')

>>> A.shutdown()                            # byexample: -skip
>>> B.shutdown()                            # byexample: -skip
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
Allocating buffers: 2048 and 2048 bytes...
<...>
A -> B flow shutdown
B is in sync
B -> A flow shutdown
A is in sync
```

<!--
>>> check_transfer(src=A, dst=B)
0 bytes transferred correctly.
subsequent 3 bytes were sent but not received (lost).

>>> check_transfer(src=B, dst=A)
0 bytes transferred correctly.
subsequent 6 bytes were sent but not received (lost).
-->

```shell
$ xxd -p -c 16 -r zazAtoB.dump | hexdump -C
00000000  66 6f 6f                                          |foo|
00000003

$ xxd -p -c 16 -r zazBtoA.dump | hexdump -C
00000000  62 61 68 62 61 68                                 |bahbah|
00000006
```

<!--
$ kill %% ; wait                                        # byexample: -skip +pass
$ rm -f AtoB.dump BtoA.dump zazAtoB.dump zazBtoA.dump   # byexample: -skip +pass
-->
