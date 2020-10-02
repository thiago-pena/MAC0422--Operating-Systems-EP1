#!/bin/bash

# script

make

for i in {1..30}
do
   ./ep1 1 traceMedio.txt report3.txt $i 2> stderr.txt
done
for i in {1..30}
do
   ./ep1 2 traceMedio.txt report3.txt $i 2> stderr.txt
done
for i in {1..30}
do
   ./ep1 3 traceMedio.txt report3.txt $i 2> stderr.txt
done
