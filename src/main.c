#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
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

const char *const SHM_FILENAME = "/wl_shm_zoomer";

struct client_state {
  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct wl_shm *wl_shm;
  struct wl_compositor *wl_compositor;
  struct xdg_wm_base *xdg_wm_base;
  struct wl_surface *wl_surface;
  struct wl_output *wl_output;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct ext_output_image_capture_source_manager_v1 *ext_output_image_capture_source_manager_v1;
  struct ext_image_capture_source_v1 *ext_image_capture_source_v1;
  struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1;
  struct ext_foreign_toplevel_image_capture_source_manager_v1 *ext_foreign_toplevel_image_capture_source_manager_v1;
  struct ext_image_copy_capture_manager_v1 *ext_image_copy_capture_manager_v1;
  uint32_t buffer_width;
  uint32_t buffer_height;
  uint32_t shm_format;
  bool has_shm_format;
};

static int create_shm_file(void) {
  int fd = shm_open(SHM_FILENAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd > 0) {
    shm_unlink(SHM_FILENAME);
    return fd;
  }
  return fd;
}

int allocate_shm_file(size_t size) {
  int fd = create_shm_file();
  assert(fd > 0 && "shm creation failed");

  int ret = ftruncate(fd, size);
  if (ret < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
  /* Sent by the compositor when it's no longer using this buffer */
  wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static struct wl_buffer *draw_frame(struct client_state *state) {
  const int height = 720, width = 480;
  const int stride = width * 4;
  const int shm_pool_size = height * stride * 2;

  int fd = allocate_shm_file(shm_pool_size);
  assert(fd > 0 && "shm allocation failed");

  uint32_t *data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, shm_pool_size);
  struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
      width, height, stride, WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  /* Draw checkerboxed background */
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((x + y / 8 * 8) % 16 < 8)
        data[y * width + x] = 0xFF666666;
      else
        data[y * width + x] = 0xFFEEEEEE;
    }
  }

  munmap(data, shm_pool_size);
  wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
  return buffer;
}

void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
  struct client_state *state = data;
  xdg_surface_ack_configure(xdg_surface, serial);

  struct wl_buffer *buffer = draw_frame(state);
  wl_surface_attach(state->wl_surface, buffer, 0, 0);
  wl_surface_commit(state->wl_surface);
}

struct xdg_surface_listener xdg_surface_listener = {
  .configure = &xdg_surface_configure,
};

static void xdg_wm_base_handle_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}

struct xdg_wm_base_listener xdg_wm_base_listener = {
  .ping = &xdg_wm_base_handle_ping,
};

static void registry_handle_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
  struct client_state *state = data; 
  printf("%s\n", interface);
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
    int xdg_listener_status = xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, NULL);
    assert(xdg_listener_status != -1);
  } else if (strcmp(interface, ext_output_image_capture_source_manager_v1_interface.name) == 0) {
    state->ext_output_image_capture_source_manager_v1 = wl_registry_bind(wl_registry, name, 
        &ext_output_image_capture_source_manager_v1_interface, version);
  } else if (strcmp(interface, ext_foreign_toplevel_image_capture_source_manager_v1_interface.name) == 0) {
    state->ext_foreign_toplevel_image_capture_source_manager_v1 = wl_registry_bind(wl_registry, name,
        &ext_foreign_toplevel_image_capture_source_manager_v1_interface, version);
  } else if (strcmp(interface, ext_image_copy_capture_manager_v1_interface.name) == 0) {
    state->ext_image_copy_capture_manager_v1 = wl_registry_bind(wl_registry, name,
        &ext_image_copy_capture_manager_v1_interface, version);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    state->wl_output = wl_registry_bind(wl_registry, name, &wl_output_interface, version);
  }
}

static void registry_handle_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
  // No-op
}

static struct wl_registry_listener wl_registry_listener = {
	.global = &registry_handle_global,
  .global_remove = &registry_handle_global_remove,
};

void handle_session_buffer_size (
    void *data, struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    uint32_t width, uint32_t height) {
  struct client_state *state = data;
  state->buffer_width = width;
  state->buffer_height = height;
}

void handle_session_shm_format(
    void *data, struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    uint32_t format) {
  struct client_state *state = data;
  if (state->has_shm_format) {
    return;
  }

  // Note: not exactly sure how format translates to this enum value
  if (format == WL_SHM_FORMAT_XBGR8888) {
    state->shm_format = format;
    state->has_shm_format = true;
  }
}

void handle_dmabuf_device(void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    struct wl_array *device) {
  // No-op
}

void handle_dmabuf_format(void *data,
  struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
  uint32_t format,
  struct wl_array *modifiers) {
  // No-op
}

void handle_done(void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1) {
  // No-op
}

void handle_stopped(void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1) {
  // No-op
}

struct ext_image_copy_capture_session_v1_listener ext_image_copy_capture_session_v1_listener = {
  .buffer_size = &handle_session_buffer_size,
  .shm_format = &handle_session_shm_format,
	.dmabuf_device = &handle_dmabuf_device,
	.dmabuf_format = &handle_dmabuf_format,
	.done = &handle_done,
	.stopped = &handle_stopped,
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

  assert(state.wl_compositor != NULL && "failed to bind compositor");
  state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
  state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);

  int xdg_surface_listener_status = xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
  assert(xdg_surface_listener_status != -1);

  state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
  xdg_toplevel_set_title(state.xdg_toplevel, "Zoomer Client");

  state.ext_image_capture_source_v1 = ext_output_image_capture_source_manager_v1_create_source(
      state.ext_output_image_capture_source_manager_v1, state.wl_output);
  state.ext_image_copy_capture_session_v1 = ext_image_copy_capture_manager_v1_create_session(
      state.ext_image_copy_capture_manager_v1, state.ext_image_capture_source_v1, 0);
  ext_image_copy_capture_session_v1_add_listener(state.ext_image_copy_capture_session_v1,
					       &ext_image_copy_capture_session_v1_listener, &state);
  wl_display_roundtrip(state.wl_display);


  wl_surface_commit(state.wl_surface);

  // https://wayland.app/protocols/ext-image-capture-source-v1
  // https://wayland.app/protocols/ext-image-copy-capture-v1
  // Creating a source needs to need a `wl_output`
  // https://wayland.app/protocols/wayland#wl_output

  while (wl_display_dispatch(state.wl_display)) {
    // No-op
  }

  // Todo: 
  // might need to cleanup some of the ext stuff too
  ext_image_capture_source_v1_destroy(state.ext_image_capture_source_v1);
  ext_output_image_capture_source_manager_v1_destroy(state.ext_output_image_capture_source_manager_v1);
  wl_output_release(state.wl_output);
  wl_display_disconnect(state.wl_display);
  return 0;
}
