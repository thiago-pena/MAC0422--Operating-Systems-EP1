#!/bin/bash

# script

make

for i in {1..30}
do
   ./ep1 1 traceGrande.txt report3.txt $i 2> stderr.txt
done
for i in {1..30}
do
   ./ep1 2 traceGrande.txt report3.txt $i 2> stderr.txt
done
for i in {1..30}
do
   ./ep1 3 traceGrande.txt report3.txt $i 2> stderr.txt
done
