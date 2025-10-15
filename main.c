#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

void print_usage(const char *program_name) {
  fprintf(stderr, "Usage: %s <card_name> <red_scale> <green_scale> <blue_scale>\n", program_name);
  fprintf(stderr, "  card_name:   Name of the DRM card (e.g., card0, card1)\n");
  fprintf(stderr, "  red_scale:   Red channel scaling factor (0.0 to 1.0)\n");
  fprintf(stderr, "  green_scale: Green channel scaling factor (0.0 to 1.0)\n");
  fprintf(stderr, "  blue_scale:  Blue channel scaling factor (0.0 to 1.0)\n");
  fprintf(stderr, "\nExample: %s card1 1.0 0.8 0.7\n", program_name);
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    print_usage(argv[0]);
    return 1;
  }

  const char *card_name = argv[1];
  double red_scale = atof(argv[2]);
  double green_scale = atof(argv[3]);
  double blue_scale = atof(argv[4]);

  if (red_scale < 0.0 || red_scale > 1.0 ||
      green_scale < 0.0 || green_scale > 1.0 ||
      blue_scale < 0.0 || blue_scale > 1.0) {
    fprintf(stderr, "Error: Scale values must be between 0.0 and 1.0\n");
    print_usage(argv[0]);
    return 1;
  }

  char device_path[256];
  snprintf(device_path, sizeof(device_path), "/dev/dri/%s", card_name);

  printf("Opening device: %s\n", device_path);
  printf("RGB Scale values: R=%.2f, G=%.2f, B=%.2f\n", red_scale, green_scale, blue_scale);

  int fd = open(device_path, O_RDWR);
  if (fd < 0) {
    perror(device_path);
    return 1;
  }

  drmModeRes *res = drmModeGetResources(fd);
  if (!res) {
    perror("drmModeGetResources");
    close(fd);
    return 1;
  }

  for (int i = 0; i < res->count_crtcs; i++) {
    drmModeCrtc *crtc = drmModeGetCrtc(fd, res->crtcs[i]);
    if (!crtc)
      continue;

    uint32_t size = crtc->gamma_size;
    if (size == 0) {
      drmModeFreeCrtc(crtc);
      continue;
    }

    uint16_t *r = malloc(size * sizeof(uint16_t));
    uint16_t *g = malloc(size * sizeof(uint16_t));
    uint16_t *b = malloc(size * sizeof(uint16_t));

    if (!r || !g || !b) {
      fprintf(stderr, "Error: Failed to allocate memory for gamma tables\n");
      free(r); free(g); free(b);
      drmModeFreeCrtc(crtc);
      drmModeFreeResources(res);
      close(fd);
      return 1;
    }

    for (uint32_t j = 0; j < size; j++) {
      double v = (double)j / (double)(size - 1);
      r[j] = (uint16_t)(0xFFFF * v * red_scale);
      g[j] = (uint16_t)(0xFFFF * v * green_scale);
      b[j] = (uint16_t)(0xFFFF * v * blue_scale);
    }

    if (drmModeCrtcSetGamma(fd, res->crtcs[i], size, r, g, b) != 0)
      perror("drmModeCrtcSetGamma");
    else
      printf("Applied gamma correction on CRTC %d (gamma size: %u)\n", i, size);

    free(r); free(g); free(b);
    drmModeFreeCrtc(crtc);
  }

  drmModeFreeResources(res);
  close(fd);
  return 0;
}
