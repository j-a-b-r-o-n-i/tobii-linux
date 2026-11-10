/* Send Tobii head pose to opentrack.
 *
 * Reads head pose through Stream Engine (Tobii Engine must be running, see
 * scripts/with-engine.sh) and sends each valid sample to opentrack's
 * "UDP over network" input: six little-endian doubles, X Y Z in centimetres
 * then yaw, pitch and roll in degrees.
 *
 * Usage: tobii_opentrack [--host 127.0.0.1] [--port 4242] [--print]
 *
 * Axes are passed through as Tobii reports them (yaw is rotation about Y,
 * pitch about X, roll about Z). Flip or remap them in opentrack's Options
 * window if a direction comes out backwards.
 */
#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RAD2DEG 57.29577951308232

static volatile sig_atomic_t running = 1;
static void on_signal(int sig) { (void)sig; running = 0; }

struct ctx {
    int sock;
    struct sockaddr_in dest;
    int print;
    double last[6];   /* last valid pose, resent when one part goes invalid */
    long sent;
};

static void url_cb(char const *url, void *user) {
    char *buf = user;
    if (!*buf && strlen(url) < 256) strcpy(buf, url);
}

static void head_cb(tobii_head_pose_t const *h, void *user) {
    struct ctx *c = user;
    int any = 0;
    if (h->position_validity == TOBII_VALIDITY_VALID) {
        for (int i = 0; i < 3; i++) c->last[i] = h->position_xyz[i] / 10.0; /* mm to cm */
        any = 1;
    }
    /* Tobii: rotation_xyz is about X (pitch), Y (yaw), Z (roll), in radians. */
    static int const to_slot[3] = { 4, 3, 5 };
    for (int i = 0; i < 3; i++) {
        if (h->rotation_validity_xyz[i] == TOBII_VALIDITY_VALID) {
            c->last[to_slot[i]] = h->rotation_xyz[i] * RAD2DEG;
            any = 1;
        }
    }
    if (!any) return; /* nobody in view: send nothing, opentrack holds the last pose */
    sendto(c->sock, c->last, sizeof c->last, 0, (struct sockaddr *)&c->dest, sizeof c->dest);
    if (c->print && c->sent % 15 == 0)
        printf("x %6.1f y %6.1f z %6.1f cm  yaw %6.1f pitch %6.1f roll %6.1f\n",
               c->last[0], c->last[1], c->last[2], c->last[3], c->last[4], c->last[5]);
    c->sent++;
}

/* Connect and stream until the connection drops or we are stopped.
 * Returns 0 when stopped, 1 to retry. */
static int run_once(tobii_api_t *api, struct ctx *c) {
    char url[256] = {0};
    if (tobii_enumerate_local_device_urls(api, url_cb, url) || !*url) return 1;
    tobii_device_t *dev;
    tobii_error_t e = tobii_device_create(api, url, &dev);
    if (e) { fprintf(stderr, "connect: %s\n", tobii_error_message(e)); return 1; }
    e = tobii_head_pose_subscribe(dev, head_cb, c);
    if (e) {
        fprintf(stderr, "head pose: %s (is Tobii Engine running?)\n", tobii_error_message(e));
        tobii_device_destroy(dev);
        return 1;
    }
    fprintf(stderr, "streaming head pose to %s:%d\n", inet_ntoa(c->dest.sin_addr), ntohs(c->dest.sin_port));
    while (running) {
        e = tobii_wait_for_callbacks(1, &dev);
        if (e && e != TOBII_ERROR_TIMED_OUT) break;
        e = tobii_device_process_callbacks(dev);
        if (e == TOBII_ERROR_CONNECTION_FAILED) {
            fprintf(stderr, "connection lost, reconnecting\n");
            e = tobii_device_reconnect(dev);
            if (e) break;
        } else if (e) break;
    }
    tobii_head_pose_unsubscribe(dev);
    tobii_device_destroy(dev);
    return running ? 1 : 0;
}

int main(int argc, char **argv) {
    char const *host = "127.0.0.1";
    int port = 4242;
    struct ctx c = { .print = 0 };
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--host") && i + 1 < argc) host = argv[++i];
        else if (!strcmp(argv[i], "--port") && i + 1 < argc) port = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--print")) c.print = 1;
        else { fprintf(stderr, "usage: %s [--host H] [--port P] [--print]\n", argv[0]); return 2; }
    }
    c.sock = socket(AF_INET, SOCK_DGRAM, 0);
    c.dest.sin_family = AF_INET;
    c.dest.sin_port = htons(port);
    if (c.sock < 0 || inet_pton(AF_INET, host, &c.dest.sin_addr) != 1) { fprintf(stderr, "bad host %s\n", host); return 2; }
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    tobii_api_t *api;
    if (tobii_api_create(&api, NULL, NULL)) { fprintf(stderr, "could not start Stream Engine\n"); return 1; }
    while (run_once(api, &c) && running) sleep(1);
    tobii_api_destroy(api);
    close(c.sock);
    return 0;
}
