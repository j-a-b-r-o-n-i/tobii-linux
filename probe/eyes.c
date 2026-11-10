/* Print the 3D position of each eye (gaze origin, in mm from the tracker),
 * and the head yaw and roll they imply. */
#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int n;
static float const *l0(tobii_gaze_origin_t const *o) { return o->left_xyz; }
static void url_cb(char const *url, void *ctx) { char *b = ctx; if (!*b && strlen(url) < 256) strcpy(b, url); }
static void origin_cb(tobii_gaze_origin_t const *o, void *ctx) {
    (void)ctx;
    if (n++ % 15) return;
    if (o->left_validity != TOBII_VALIDITY_VALID || o->right_validity != TOBII_VALIDITY_VALID) { printf("validity left %d right %d  left %.0f %.0f %.0f\n", o->left_validity, o->right_validity, l0(o)[0], l0(o)[1], l0(o)[2]); return; }
    float const *l = o->left_xyz, *r = o->right_xyz;
    float dx = r[0] - l[0], dy = r[1] - l[1], dz = r[2] - l[2];
    printf("left %6.0f %6.0f %6.0f  right %6.0f %6.0f %6.0f  centre %6.0f %6.0f %6.0f  apart %4.0f mm  yaw %6.1f  roll %6.1f\n",
           l[0], l[1], l[2], r[0], r[1], r[2], (l[0]+r[0])/2, (l[1]+r[1])/2, (l[2]+r[2])/2,
           sqrtf(dx*dx+dy*dy+dz*dz), atan2f(-dz, dx) * 57.2958f, atan2f(dy, dx) * 57.2958f);
}
int main(int argc, char **argv) {
    int seconds = argc > 1 ? atoi(argv[1]) : 8;
    tobii_api_t *api; tobii_device_t *dev; char url[256] = {0};
    if (tobii_api_create(&api, NULL, NULL) || tobii_enumerate_local_device_urls(api, url_cb, url) || !*url
        || tobii_device_create(api, url, &dev)) { printf("could not connect\n"); return 1; }
    tobii_error_t e = tobii_gaze_origin_subscribe(dev, origin_cb, NULL);
    printf("gaze_origin_subscribe: %s\n", tobii_error_message(e));
    time_t end = time(NULL) + seconds;
    while (!e && time(NULL) < end) {
        tobii_wait_for_callbacks(1, &dev);
        if (tobii_device_process_callbacks(dev)) break;
    }
    printf("%d samples\n", n);
    tobii_device_destroy(dev); tobii_api_destroy(api); return 0;
}
