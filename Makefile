PROTO_DIR = /usr/share/wayland-protocols/staging

CFLAGS = -Wall -I.
LIBS   = -lwayland-client -lrt

PROTO_SRC = \
	xdg-shell-protocol.c \
	ext-foreign-toplevel-list-v1-protocol.c \
	ext-image-capture-source-v1-protocol.c \
	ext-image-copy-capture-v1-protocol.c

generate:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
	wayland-scanner private-code  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
	wayland-scanner client-header $(PROTO_DIR)/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml ext-foreign-toplevel-list-v1-protocol.h
	wayland-scanner private-code  $(PROTO_DIR)/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml ext-foreign-toplevel-list-v1-protocol.c
	wayland-scanner client-header $(PROTO_DIR)/ext-image-capture-source/ext-image-capture-source-v1.xml ext-image-capture-source-v1-protocol.h
	wayland-scanner private-code  $(PROTO_DIR)/ext-image-capture-source/ext-image-capture-source-v1.xml ext-image-capture-source-v1-protocol.c
	wayland-scanner client-header $(PROTO_DIR)/ext-image-copy-capture/ext-image-copy-capture-v1.xml ext-image-copy-capture-v1-protocol.h
	wayland-scanner private-code  $(PROTO_DIR)/ext-image-copy-capture/ext-image-copy-capture-v1.xml ext-image-copy-capture-v1-protocol.c

build:
	gcc $(CFLAGS) -o zoomer $(PROTO_SRC) src/main.c $(LIBS)

clean:
	rm -f zoomer *-protocol.c *-protocol.h
