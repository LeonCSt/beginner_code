Notes about following cpp apps: -

leon410.cpp and leon420.cpp are simple CLI apps.

xdg-shell-protocols.cpp | Needs to reside in the working directory,
   for all of the following GUI examples. Initially generated with the
   wayland-scanner, then modified to suit g++ compilation. Either copy
   from this repository or make your own, by: -
    In the working directory : -
  wayland-scanner private-code \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-protocol.cpp
    MODIFY xdg-shell-protocol.cpp TO SUIT g++ COMPILER AS FOLLOWS:-
    1) Delete the first 52 lines e.g. all includes and externs
    2] Replace with #include <wayland-util.h>
        and         #include "xdg-shell-client-protocol.h"
    3) Remove all WL_PRIVATE prepends (all 5 of them).

   ** note **    xdg-shell-client-protocol.h   is also needed in the
   working directory for #include "" :-
  wayland-scanner client-header \
  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  > xdg-shell-client-protocol.h

leon430.cpp is a single draw window, where you can see what happens
   when you move, resize, toggle layout, maximise, etc. Leave alone for 5
   seconds and the window will self shutdown.

leon440.cpp correctly redraws everytime the user or WM requests a size
   change. Also keyboard events are enabled - but without a keymap. Just
   raw linux keycodes are used. Cursor not properly implemented and is
   highjacked from adjacent client or root window.

leon450.cpp enables pointer events and uses system cursors. Needs
   -lwayland-cursor appended to g++ string. Still no keymap, just
   linux keycodes.

leon460.cpp does not use <wayland=cursor.h>, but generates it's own
   cursors which are stored in the main shm_pool. No need for the
   -lwayland-cursor linker on g++ command. Can be used as-is to create
   your own cursors if the following edits are made: -
    (1) Make sure crsrscale = 1.0 .
    (2) Modify the subset in crsr[]  (do not change offsets). Width *
          Height must not exceed 308.
    (3) Foundry working space is 4 times larger in each dimension.
          Modify the associated section in cursors_draw(). Then cut and
          paste that section so it is the last section in that block.
    (4) Run.  Min window 832 X 540 to see working.
