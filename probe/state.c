/* Print what the tracker has stored: display area, mounting and calibration. */
#include <tobii/tobii.h>
#include <tobii/tobii_config.h>
#include <stdio.h>
#include <string.h>

static void url_cb(char const *url, void *ctx) { char *b = ctx; if (!*b && strlen(url) < 256) strcpy(b, url); }
static void cal_cb(void const *data, size_t size, void *ctx) { (void)data; *(size_t *)ctx = size; }

int main(void) {
    tobii_api_t *api; tobii_device_t *dev; char url[256] = {0};
    if (tobii_api_create(&api, NULL, NULL) || tobii_enumerate_local_device_urls(api, url_cb, url) || !*url
        || tobii_device_create(api, url, &dev)) { printf("could not connect\n"); return 1; }

    tobii_display_area_t d; tobii_error_t e = tobii_get_display_area(dev, &d);
    printf("display area: %s\n", tobii_error_message(e));
    if (!e) printf("  top-left %.1f %.1f %.1f\n  top-right %.1f %.1f %.1f\n  bottom-left %.1f %.1f %.1f (mm)\n",
        d.top_left_mm_xyz[0], d.top_left_mm_xyz[1], d.top_left_mm_xyz[2],
        d.top_right_mm_xyz[0], d.top_right_mm_xyz[1], d.top_right_mm_xyz[2],
        d.bottom_left_mm_xyz[0], d.bottom_left_mm_xyz[1], d.bottom_left_mm_xyz[2]);

    tobii_geometry_mounting_t m; e = tobii_get_geometry_mounting(dev, &m);
    printf("mounting: %s\n", tobii_error_message(e));
    if (!e) printf("  guides %d, width %.1f mm, angle %.1f deg, offset %.1f %.1f %.1f\n",
        m.guides, m.width_mm, m.angle_deg, m.external_offset_mm_xyz[0], m.external_offset_mm_xyz[1], m.external_offset_mm_xyz[2]);

    size_t cal = 0; e = tobii_calibration_retrieve(dev, cal_cb, &cal);
    printf("calibration retrieve: %s, %zu bytes\n", tobii_error_message(e), cal);


    tobii_device_destroy(dev); tobii_api_destroy(api); return 0;
}
