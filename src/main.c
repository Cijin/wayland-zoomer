#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>

#include "xdg-shell-protocol.h"
#include "ext-foreign-toplevel-list-v1-protocol.h"
#include "ext-image-capture-source-v1-protocol.h"
#include "ext-image-copy-capture-v1-protocol.h"

struct client_state {
  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct wl_shm *wl_shm;
  struct wl_compositor *wl_compositor;
  struct xdg_wm_base *xdg_wm_base;
  struct wl_surface *wl_surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct ext_output_image_capture_source_manager_v1 *ext_output_image_capture_source_manager;
  struct ext_foreign_toplevel_image_capture_source_manager_v1 *ext_foreign_toplevel_image_capture_source_manager;
  struct ext_image_copy_capture_manager_v1 *ext_image_copy_capture_manager;
};

static void registry_handle_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
  struct client_state *state = data; 
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
  } else if (strcmp(interface, ext_output_image_capture_source_manager_v1_interface.name) == 0) {
    state->ext_output_image_capture_source_manager = wl_registry_bind(wl_registry, name, 
        &ext_output_image_capture_source_manager_v1_interface, version);
  } else if (strcmp(interface, ext_foreign_toplevel_image_capture_source_manager_v1_interface.name) == 0) {
    state->ext_foreign_toplevel_image_capture_source_manager = wl_registry_bind(wl_registry, name,
        &ext_foreign_toplevel_image_capture_source_manager_v1_interface, version);
  } else if (strcmp(interface, ext_image_copy_capture_manager_v1_interface.name) == 0) {
    state->ext_image_copy_capture_manager = wl_registry_bind(wl_registry, name,
        &ext_image_copy_capture_manager_v1_interface, version);
  }
}

static void registry_handle_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
  // left blank for now
}

static struct wl_registry_listener wl_registry_listener = {
	.global = &registry_handle_global,
  .global_remove = &registry_handle_global_remove,
};

int main(int argc, char *argv[]) {
  struct client_state state = {0};
  const char *display = getenv("WAYLAND_DISPLAY");
  if (!display) {
    fprintf(stderr, "WAYLAND_DISPLAY not found\n");
    return 1;
  }

  const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
  if (!xdg_runtime_dir) {
    fprintf(stderr, "XDG_RUNTIME_DIR not found\n");
    return 1;
  }

  state.wl_display = wl_display_connect(NULL);
  state.wl_registry = wl_display_get_registry(state.wl_display);
  wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
  wl_display_roundtrip(state.wl_display);

  wl_display_disconnect(state.wl_display);
  return 0;
}
