#!/bin/bash

mkfifo /tmp/stream_pipe

#cat /path/to/transparent/or/splash/screen.png > stream_pipe &

echo "hello world" > stream_pipe
cat stream_pipe
echo "some more data" > stream_pipe
cat stream_pipe



mkfifo pipe
(while cat pipe; do : Nothing; done &)
echo "some data" > pipe
echo "more data" > pipe
An alternative is to keep some process with the FIFO open.

mkfifo pipe
sleep 10000 > pipe &
cat pipe &
echo "some data" > pipe
echo "more data" > pipe

