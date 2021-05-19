<!--
Import some helper tools
>>> from helper import pair_ports, netcat, check_transfer

Pick two random ports
>>> pair_ports()                            # byexample: +fail-fast
(<port-a>, <port-b>)

Alias
$ alias tiburoncin=../tiburoncin
-->

To show you how ``tiburoncin`` handles a slow receiver.

First, we run ``tiburoncin`` and the two endpoints as usual
with one, the receiver, with a small buffer size

```python
>>> B = netcat(listen_on = <port-b>, rcv_buf=1024)        # byexample: +paste +fail-fast
```

```shell
$ tiburoncin -A 127.0.0.1:<port-a> -B 127.0.0.1:<port-b> -c -z 1024:4096 -b 512:2048    # byexample: +paste +stop-on-silence +timeout=2
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

``tiburoncin`` sets up two buffers one for the ``A -> B`` flow and the other
for the ``B -> A`` flow.

You can see it at the begin of the run:

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
Allocating buffers: 512 and 2048 bytes...
```

Now we can fill the ``tiburoncin``'s and receiver's buffers up
sending enough data.

The first chunk will fill the receiver's buffer at the socket layer
(Operative System).

```python
>>> A.send('X' * 1024) # fill receiver's buffer at socket layer
```

Then we need to fill the ``tiburoncin``'s buffer at the socket layer.

This was set
to 1024 for sending and 4096 for receiving using the ``-z`` option.

The important value is 1024 because ``tiburoncin`` is *sending* the data
to the receiver (netcat ``B``).

```python
>>> A.send('Y' * 1024) # fill tiburoncin's send buffer at the socket layer
```

Finally send a third chunk to fill the ``tiburoncin``'s buffer for
the ``A -> B`` flow. This was set to 512 using the ``-b`` option

```python
>>> A.send('Z' * 512) # fill the tiburoncin's buffer for the A -> B flow
```

Now, to prove you that everything is filled, we can send more data.

Because all the buffers are filled, this data will be stored in ``tiburoncin``'s
receiver buffer at the socket layer and/or in the sender buffer (netcat ``A``)
at the socket layer:

```python
>>> A.send('A' * 1024) # this will fill up the tiburoncin's buffer
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
A -> B sent 512 bytes
00000000  58 58 58 58 58 58 58 58  58 58 58 58 58 58 58 58  |XXXXXXXXXXXXXXXX|
<...>
000009f0  5a 5a 5a 5a 5a 5a 5a 5a  5a 5a 5a 5a 5a 5a 5a 5a  |ZZZZZZZZZZZZZZZZ|
B is 512 bytes behind
```

Notice how ``tiburoncin`` sends data in chunks of 512 bytes. This is because
the buffer for the ``A -> B`` flow is limited to 512.

Also, see that ``0x000009f0+0x10`` is how many bytes ``tiburoncin`` saw:
2560 bytes (2 chunks of 1024 plus 1 of 512).

And also, ``B`` reads everything except the last 512 bytes which it makes sense:
the first 1024 bytes are in the bufffer of B`` ``at the socket layer, the
second 1024 bytes are in ``tiburoncin``'s buffer at the socket layer and
the last 512 bytes are in ``tiburoncin``'s buffer.

If ``B`` consumes 2048 bytes, he will empty his buffer and
``tiburoncin``'s buffer at the socket layer and therefore the kernel should
request more data from ``tiburoncin`` which should deliver it in
two chunks of 512 bytes:

```python
>>> B.consume(2048)
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
00000df0  41 41 41 41 41 41 41 41  41 41 41 41 41 41 41 41  |AAAAAAAAAAAAAAAA|
B is 512 bytes behind
B is in sync
```

``B`` gets in sync at the end because the 512 ``'Z'``s and
the 1024 ``'A'``s were sent
from ``tiburoncin``'s point of view: the first 1024 bytes
are in the ``B``'s buffer
and the last 512 in the ``tiburoncin``'s buffer at the socket layer.


We can now close the channel from ``A`` *even* if there is
data on the ``tiburoncin``'s
buffers: the shutdown will propagate to the receiver only after he received
all the data first.

```python
>>> A.send('B' * 512)
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
00000ff0  42 42 42 42 42 42 42 42  42 42 42 42 42 42 42 42  |BBBBBBBBBBBBBBBB|
B is 512 bytes behind
B is in sync
```

```python
>>> A.shutdown("write-channel")
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
A -> B flow shutdown
B is in sync
```

```python
>>> B.consume(1024)
>>> B.consume(1024)
```

However, if some data is still in the buffer and who shutdown is the receiver
that means that the data cannot be delivered.

That's what we call a *broken pipe*.

```python
>>> B.send('C' * (1024 * 8))
>>> A.shutdown("both-channels")
```

```shell
$ fg                                        # byexample: +stop-on-silence
<...>tiburoncin <...>
Passthrough from B to A failed: Broken pipe
```

<!--
>>> check_transfer(src=A, dst=B)
4096 bytes transferred correctly.

>>> check_transfer(src=B, dst=A)
0 bytes transferred correctly.
subsequent 8192 bytes were sent but not received (lost).
-->

<!--
$ kill %% ; wait                           # byexample: -skip +pass
-->
