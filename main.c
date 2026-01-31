#include <fcntl.h>
#include <math.h>
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
  fprintf(stderr, "Usage: %s <card_name> <temperature>\n", program_name);
  fprintf(stderr, "  card_name:   Name of the DRM card (e.g., card0, card1)\n");
  fprintf(stderr, "  temperature: Color temperature in Kelvin (1000-10000)\n");
  fprintf(stderr, "               6500K = neutral white (daylight)\n");
  fprintf(stderr, "               5000K = slightly warm\n");
  fprintf(stderr, "               3000K = warm (evening)\n");
  fprintf(stderr, "               2000K = very warm (night shift)\n");
  fprintf(stderr, "\nExample: %s card1 2000\n", program_name);
}

// Convert color temperature (in Kelvin) to RGB values
// Based on Tanner Helland's algorithm
void kelvin_to_rgb(int temp, double *r, double *g, double *b) {
  double temperature = temp / 100.0;
  
  // Calculate Red
  if (temperature <= 66) {
    *r = 1.0;
  } else {
    double red = temperature - 60.0;
    red = 329.698727446 * pow(red, -0.1332047592);
    *r = red / 255.0;
    if (*r < 0.0) *r = 0.0;
    if (*r > 1.0) *r = 1.0;
  }
  
  // Calculate Green
  if (temperature <= 66) {
    double green = temperature;
    green = 99.4708025861 * log(green) - 161.1195681661;
    *g = green / 255.0;
  } else {
    double green = temperature - 60.0;
    green = 288.1221695283 * pow(green, -0.0755148492);
    *g = green / 255.0;
  }
  if (*g < 0.0) *g = 0.0;
  if (*g > 1.0) *g = 1.0;
  
  // Calculate Blue
  if (temperature >= 66) {
    *b = 1.0;
  } else if (temperature <= 19) {
    *b = 0.0;
  } else {
    double blue = temperature - 10.0;
    blue = 138.5177312231 * log(blue) - 305.0447927307;
    *b = blue / 255.0;
    if (*b < 0.0) *b = 0.0;
    if (*b > 1.0) *b = 1.0;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    print_usage(argv[0]);
    return 1;
  }

  const char *card_name = argv[1];
  int temperature = atoi(argv[2]);

  if (temperature < 1000 || temperature > 10000) {
    fprintf(stderr, "Error: Temperature must be between 1000K and 10000K\n");
    print_usage(argv[0]);
    return 1;
  }

  double red_scale, green_scale, blue_scale;
  kelvin_to_rgb(temperature, &red_scale, &green_scale, &blue_scale);

  char device_path[256];
  snprintf(device_path, sizeof(device_path), "/dev/dri/%s", card_name);

  printf("Opening device: %s\n", device_path);
  printf("Color Temperature: %dK\n", temperature);
  printf("RGB Scale values: R=%.3f, G=%.3f, B=%.3f\n", red_scale, green_scale, blue_scale);

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
