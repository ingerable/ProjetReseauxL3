#!/bin/sh 

sleep 1
xterm -hold -title "client put" -e "sleep 1 && echo ../client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:6666 && ../client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:6666 && sleep 1;echo ../client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:1111 && ../client ::1 40003 put hash1example 2001:0db8:85a3:0000:0000:8a2e:0370:1111" 



