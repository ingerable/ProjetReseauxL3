#!/bin/sh

sleep 4
xterm -hold -title "client get" -e "echo ../client ::1 40003 get hash1example;../client ::1 40003 get hash1example;echo test avec un hash non existant;echo ../client ::1 40003 get hashexampleInconnu;../client ::1 40003 get hashexampleInconnu"



