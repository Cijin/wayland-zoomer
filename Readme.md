# Inspired by [Boomer](https://github.com/tsoding/boomer)

## Let's break it down
This is what we need to be able to achieve:
1. Open a window
2. Take a screenshot of the screen
3. Be able to zoom in

### Opening a window
You could either go with SDL or Raylib but for my purposes of education I'd rather build it with Wayland. The thingy that opens a window on Pop-OS 24.xx.
This is obviously easier said than done but luckily I've written a wayland client in Zig end of last year (2025) so it's not entirely foreign. Plus I could 
reuse it for other things along the line (maybe).

[Reference](https://wayland-book.com/xdg-shell-basics/example-code.html)

### 01. Generate `xdg-shell-client-protocol.h`
Not as straightforward as I thought it'd be. But not too bad either. Install `wayland-protocols` and use the `wayland-scanner` to generate the code needed.
Check the reference above for details.

### 02. Connect wayland display
Not too bad, this one is quite easy. Check `src/main.c`

03. Get registry
        a. Add listener to collect globals
        b. Mainly: `wl_compositor`, `xdg_wm_base, `wl_shm`.
04. Recieve registry events: `wl_display_roundtrip`?
05. Bind globals
06. Add `ping/pong` listener to `xdg_wm_base`
07. Create `wl_surface`
08. Wrap surface into `xdg_surface` and get `xdg_toplevel`
        a. Add listeners for both (surface & toplevel)
09. Commit the surface
10. In configure callback ack serial
11. Create `shm_buffer`
12. Attach it
13. Mark it as `damaged`
14. Commit it
15. Run `wl_display_dispatch`

Most of these are sort of coming back from when I was working on them last time. But some of these I don't really remember. I'll add details as I go along.


### Take a screenshot of the screen
I'll break this down once the above is done.
