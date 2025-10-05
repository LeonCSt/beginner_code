/*  leon420.cpp | This binds the global object wl_output    
    to the clients wl_output interface.
    Queries some of the details regarding the screen resolution etc. 
    g++ leon420.cpp -o leon -lwayland-client                  [250804]  */

#include <stdio.h>
#include <wayland-client.h>
#include <cstring>
using namespace std;

wl_display *dis;
wl_registry *reg;
wl_output *out;

void output_geometry(void* data ,struct wl_output *wl_output,
        int32_t x, int32_t y, int32_t physical_width,
        int32_t physical_height, int32_t subpixel, const char *make,
        const char *model, int32_t transform) {
  printf("Top Left  x: %d, y: %d\n", x, y);
  printf("Physical Size  Width: %d, Height: %d  millimeters\n",
          physical_width, physical_height);
}

void output_mode(void *data, struct wl_output *wl_output,
        uint32_t flags, int32_t width, int32_t height,
	int32_t refresh) {
  printf("Hardware units: %d X %d   Refresh rate: %d mHz\n",
          width, height, refresh);
}

void output_done(void *data, struct wl_output *wl_output) {
  printf("Complete\n");
}

void output_scale(void *data, struct wl_output *wl_output,
        int32_t factor) {
  printf("Scale Factor: %d\n", factor);
}

void output_name(void *data, struct wl_output *wl_output,
        const char *name) {
  printf("NAME: %s\n", name);
}

void output_description(void *data,
        struct wl_output *wl_output, const char *description) {
  printf("Description: %s\n", description);
}

wl_output_listener out_lstnr = {
  .geometry = output_geometry,
  .mode = output_mode,
  .done = output_done,
  .scale = output_scale,
  .name = output_name,
  .description = output_description,
};

void registry_global(void *data, struct wl_registry *regi,
	uint32_t name, const char *interface, uint32_t version) {
  if (strcmp(interface, wl_output_interface.name) == 0) {
    out = (wl_output*)wl_registry_bind(
            regi, name, &wl_output_interface, version);
//    printf("OK! '%s'  '%d'  bound.\n", interface, name);
  }
}

void registry_global_remove(void *data,
        struct wl_registry *reg, uint32_t name) {
  printf("Global listed as '%d' is removed\n", name);
}

wl_registry_listener rgstry_lstnr = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

int main() {
  dis = wl_display_connect(NULL);
  reg = wl_display_get_registry(dis);
  wl_registry_add_listener(reg, &rgstry_lstnr, NULL);
  wl_display_roundtrip(dis);
  wl_output_add_listener(out, &out_lstnr, NULL);
  wl_display_roundtrip(dis);
}
