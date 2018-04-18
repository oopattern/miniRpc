#!/bin/sh

# since this dir is mount share with windows
# found that write file is so slowly, because mount will slow down writing operation
# so we need to put bin exec file to other dir
cp demo /home/lidi/code/run
cd /home/lidi/code/run
./demo
cd -

