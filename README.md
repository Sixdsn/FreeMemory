# FreeMemory
Build Requirements:

libboost-dev

libboost-thread-dev

libboost-log-dev

libboost-system-dev

libboost-program-options-dev

This program is used to free unused memory such a cached pages and remount empty swap if possible.
It can run silently as a watchdog in background and check every N minutes.

USAGE:

$>sixfree -h

Allowed options:

-h [ --help ]         print this menu

-b [ --bg ]           run in background

-w [ --watchdog ]     run as watchdog

-s [ --silent ]       do not print output

-S [ --noswap ]       do not swapoff/swapon

-t [ --time ] arg     wait N minutes between each run

-m [ --memory ] arg   free if % memory available is below N
