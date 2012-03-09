smidgen is a small daemon that waits on keyboard events to disable the trackpad
while typing

based on syndaemon and fspd: ( http://int.ed.ntnu.no/fspd.tar or
https://int.ed.ntnu.no/svn/public/fspd)

this is meant to be a stopgap until the drivers work properly, but any patches
are welcome.

you will need to install the libevent-dev package (for Debian) or the equivalent
for your distribution.

until we have a full build, run `gcc -O2 -o smidgen smidgen.c -levent` to build; and
`sudo ./smidgen <kb event file>` to run. On my system, that file is
`/dev/input/by-path/platform-i8042-serio-0-event-kbd`

