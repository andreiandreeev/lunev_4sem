#! /bin/bash


for ((i=1; i<=8; i++))
do
    echo "$i cores"
    time ./calculus $i
done
