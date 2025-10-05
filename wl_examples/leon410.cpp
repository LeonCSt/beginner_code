/* this lists all the global objects available on this display[250731]
   g++ leon410.cpp -o leon -lwayland-client   */

#include <stdio.h>
#include <wayland-client.h>

static void registry_global(void *data,
        struct wl_registry *reg,
        uint32_t name, const char *interface, uint32_t version) {
  printf("interface: '%s', version: %d, name: %d\n",
          interface, version, name);
}

static void registry_global_remove(void *data,
        struct wl_registry *reg, uint32_t name) {
  // not applicable
}

static const struct wl_registry_listener rgstry_lstnr = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

int main() {
  struct wl_display *dis = wl_display_connect(NULL);
  struct wl_registry *reg = wl_display_get_registry(dis);
  wl_registry_add_listener(reg, &rgstry_lstnr, NULL);
  wl_display_roundtrip(dis);
}
