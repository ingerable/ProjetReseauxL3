#!/bin/sh

./server ::1 40003 > test.txt&
./client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:6666 > test.txt
./client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:1111 > test.txt
./client ::1 40003 get hash1example > test.txt

kill $!




