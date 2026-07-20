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

## TODO
### Before building more features

- [ ] Mark the entire surface as damaged before committing the captured buffer.
- [ ] Destroy `ext_image_copy_capture_frame_v1` after both `ready` and `failed`; the protocol requires this.
- [ ] Set the frame pointer to `NULL` after destruction.
- [ ] Track whether the initial XDG configure was received before presenting a buffer.
- [ ] Add an `xdg_toplevel` listener to handle window configuration and close requests.
- [ ] Exit the event loop specifically when `wl_display_dispatch()` returns `-1`.
- [ ] Handle capture failure reasons instead of asserting.
- [ ] Handle the capture session's `stopped` event.

### Buffer ownership

- [ ] Decide whether buffers are one-shot or reusable.
- [ ] Avoid leaving `state->wl_buffer` pointing to a buffer destroyed by `wl_buffer_release()`.
- [ ] If reusing buffers, mark them available on `wl_buffer.release` instead of destroying them.
- [ ] Reallocate buffers when the session reports changed size or format constraints.
- [ ] Eventually use two or three buffers for continuous capture.
- [ ] Check buffer-size and stride arithmetic for overflow.
- [ ] Remove the unused second half of the SHM pool unless a second buffer is created from it.
- [ ] Remove the temporary `mmap()` if the client never directly reads or writes the pixels.

### Wayland object cleanup

Destroy objects in roughly reverse dependency order:

- [ ] Capture frame.
- [ ] SHM buffer, if it has not already been destroyed.
- [ ] Capture session.
- [ ] Capture source.
- [ ] Image-copy capture manager.
- [ ] Output capture-source manager.
- [ ] XDG toplevel.
- [ ] XDG surface.
- [ ] Wayland surface.
- [ ] Output.
- [ ] SHM interface.
- [ ] XDG WM base.
- [ ] Compositor.
- [ ] Registry.
- [ ] Finally, disconnect the display.

### Error handling and compatibility

- [ ] Check whether `wl_display_connect()` returned `NULL`.
- [ ] Validate every required registry global, not only `wl_compositor`.
- [ ] Bind each global using the lower of the advertised version and the version supported by the generated protocol.
- [ ] Treat file descriptor `0` as valid; the current `fd > 0` checks reject it.
- [ ] Replace runtime `assert()` calls with recoverable errors and cleanup paths.
- [ ] Support another advertised SHM format if `XBGR8888` is unavailable.
- [ ] Check return values from listener registration and object creation consistently.
- [ ] Handle output removal instead of leaving a stale `wl_output` pointer.

### Capture correctness

- [ ] Apply or account for the capture `transform` event so rotated outputs display correctly.
- [ ] Use capture damage events for efficient partial updates later.
- [ ] Store presentation timestamps if frame timing will matter.
- [ ] Destroy and recreate the capture-frame object for every new capture.
- [ ] Consider the feedback effect when the captured output contains the zoomer window itself.

### General cleanup

- [ ] Set an XDG application ID in addition to the title.
- [ ] Remove unused headers and the unused foreign-toplevel protocol until window selection is implemented.
- [ ] Make functions and listener structures `static` where they are file-local.
- [ ] Replace the fixed SHM name with a collision-safe temporary SHM or `memfd` approach.
