/* Report each step of connecting to a Tobii tracker through Stream Engine,
 * then print gaze, head pose and presence for a few seconds. */
#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static int gaze_n, head_n;

static void log_cb(void *ctx, tobii_log_level_t level, char const *text) {
    (void)ctx;
    if (level <= TOBII_LOG_LEVEL_INFO) fprintf(stderr, "[se %d] %s\n", level, text);
}

static void url_cb(char const *url, void *ctx) {
    printf("found device: %s\n", url);
    char *buf = ctx;
    if (!*buf && strlen(url) < 256) strcpy(buf, url);
}

static void gaze_cb(tobii_gaze_point_t const *g, void *ctx) {
    (void)ctx;
    if (gaze_n++ % 30 == 0)
        printf("gaze %s %.3f %.3f\n", g->validity == TOBII_VALIDITY_VALID ? "valid" : "invalid",
               g->position_xy[0], g->position_xy[1]);
}

static void head_cb(tobii_head_pose_t const *h, void *ctx) {
    (void)ctx;
    if (head_n++ % 30 == 0)
        printf("head pos(%d) %.0f %.0f %.0f  rot(%d%d%d) %.3f %.3f %.3f\n",
               h->position_validity, h->position_xyz[0], h->position_xyz[1], h->position_xyz[2],
               h->rotation_validity_xyz[0], h->rotation_validity_xyz[1], h->rotation_validity_xyz[2],
               h->rotation_xyz[0], h->rotation_xyz[1], h->rotation_xyz[2]);
}

static void presence_cb(tobii_user_presence_status_t s, int64_t ts, void *ctx) {
    (void)ts; (void)ctx;
    printf("presence %d\n", s);
}

#define CHECK(what, call) do { tobii_error_t e = (call); \
    printf("%-28s %s\n", what, tobii_error_message(e)); if (e != TOBII_ERROR_NO_ERROR) return 1; } while (0)
#define TRY(what, call) do { tobii_error_t e = (call); \
    printf("%-28s %s\n", what, tobii_error_message(e)); } while (0)

int main(int argc, char **argv) {
    int seconds = argc > 1 ? atoi(argv[1]) : 5;
    tobii_version_t v; tobii_get_api_version(&v);
    printf("stream engine %d.%d.%d.%d\n", v.major, v.minor, v.revision, v.build);

    tobii_custom_log_t log = { NULL, log_cb };
    tobii_api_t *api;
    CHECK("api_create", tobii_api_create(&api, NULL, &log));

    char url[256] = {0};
    CHECK("enumerate", tobii_enumerate_local_device_urls(api, url_cb, url));
    if (!*url) { printf("no device found\n"); return 1; }

    tobii_device_t *dev;
    CHECK("device_create", tobii_device_create(api, url, &dev));

    tobii_device_info_t info;
    if (tobii_get_device_info(dev, &info) == TOBII_ERROR_NO_ERROR)
        printf("serial %s\nmodel %s\ngeneration %s\nfirmware %s\nintegration %s\n",
               info.serial_number, info.model, info.generation, info.firmware_version,
               info.integration_type);

    tobii_supported_t sup;
    tobii_capability_supported(dev, TOBII_CAPABILITY_CALIBRATION_2D, &sup);
    printf("calibration 2d supported: %d\n", sup);
    tobii_stream_supported(dev, TOBII_STREAM_HEAD_POSE, &sup);
    printf("head pose stream supported: %d\n", sup);

    TRY("gaze_point_subscribe", tobii_gaze_point_subscribe(dev, gaze_cb, NULL));
    TRY("head_pose_subscribe", tobii_head_pose_subscribe(dev, head_cb, NULL));
    TRY("presence_subscribe", tobii_user_presence_subscribe(dev, presence_cb, NULL));

    time_t end = time(NULL) + seconds;
    while (time(NULL) < end) {
        tobii_error_t e = tobii_wait_for_callbacks(1, &dev);
        if (e != TOBII_ERROR_NO_ERROR && e != TOBII_ERROR_TIMED_OUT) { printf("wait: %s\n", tobii_error_message(e)); break; }
        e = tobii_device_process_callbacks(dev);
        if (e != TOBII_ERROR_NO_ERROR) { printf("process: %s\n", tobii_error_message(e)); break; }
    }
    printf("received %d gaze and %d head samples in %d s\n", gaze_n, head_n, seconds);
    tobii_device_destroy(dev);
    tobii_api_destroy(api);
    return 0;
}
