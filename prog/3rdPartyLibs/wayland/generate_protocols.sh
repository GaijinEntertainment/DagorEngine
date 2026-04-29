# generates various wayland protocols headers & impl
wayland-scanner public-code < /usr/share/wayland/wayland.xml > wayland-protocol.c
wayland-scanner client-header < /usr/share/wayland/wayland.xml > wayland-client-protocol.h
wayland-scanner public-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c
wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h
wayland-scanner public-code < /usr/share/wayland-protocols/stable/viewporter/viewporter.xml > viewporter-protocol.c
wayland-scanner client-header < /usr/share/wayland-protocols/stable/viewporter/viewporter.xml > viewporter-client-protocol.h
wayland-scanner public-code < /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml > relative-pointer-unstable-v1-protocol.c
wayland-scanner client-header < /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml > relative-pointer-unstable-v1-client-protocol.h
wayland-scanner public-code < /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml > pointer-constraints-unstable-v1-protocol.c
wayland-scanner client-header < /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml > pointer-constraints-unstable-v1-client-protocol.h
