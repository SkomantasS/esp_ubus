#include "ubus_functions.h"
#include <libserialport.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

struct ubus_context *ctx;

int main(int argc, char **argv) {
    const char *ubus_socket = NULL;
    int ch;

    while ((ch = getopt(argc, argv, "cs:")) != -1) {
        switch (ch) {
        case 's':
            ubus_socket = optarg;
            break;
        default:
            break;
        }
    }

    argc -= optind;
    argv += optind;

    uloop_init();

    ctx = ubus_connect(ubus_socket);
    if (!ctx) {
        return -1;
    }

    ubus_add_uloop(ctx);
    server_main(ctx);

    ubus_free(ctx);
    uloop_done();
    return 0;
}