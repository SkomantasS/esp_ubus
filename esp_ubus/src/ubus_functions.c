#include "ubus_functions.h"
#include "serialport_functions.h"
#include <libserialport.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BETWEEN(value, min, max) (value <= max && value >= min)

static struct ubus_subscriber esp_serial_event;
static struct blob_buf b;

enum { DEVICES_PORT, DEVICES_VENDOR_ID, DEVICES_PRODUCT_ID, __DEVICES_MAX };
enum { OFF_PORT, OFF_PIN, __OFF_MAX };
enum { ON_PORT, ON_PIN, __ON_MAX };

static const struct blobmsg_policy devices_policy[] = {};
static const struct blobmsg_policy off_policy[] = {
    [OFF_PORT] = {.name = "port", .type = BLOBMSG_TYPE_STRING},
    [OFF_PIN] = {.name = "pin", .type = BLOBMSG_TYPE_INT32},
};
static const struct blobmsg_policy on_policy[] = {
    [ON_PORT] = {.name = "port", .type = BLOBMSG_TYPE_STRING},
    [ON_PIN] = {.name = "pin", .type = BLOBMSG_TYPE_INT32},
};

struct devices_request {
    struct ubus_request_data req;
    struct uloop_timeout timeout;
    struct ubus_context *ctx;
    int pin;
    int on_or_off;
    int size;
    char data[];
};

struct on_or_off_request {
    struct ubus_request_data req;
    struct uloop_timeout timeout;
    struct ubus_context *ctx;
    int pin;
    int on_or_off;
    int size;
    char data[];
};

void devices_reply(struct uloop_timeout *t) {
    struct port_data esp_data[10];
    int count = get_esp_devices(&esp_data[0]);
    struct devices_request *req =
        container_of(t, struct devices_request, timeout);
    void *c, *d;
    char vendor[20];
    char product[20];
    blob_buf_init(&b, 0);
    c = blobmsg_open_array(&b, "devices");
    for (int i = 0; i < count; i++) {
        d = blobmsg_open_table(&b, "NULL");
        blobmsg_add_string(&b, "port", esp_data[i].port_name);
        if (strstr(esp_data[i].port_name, "USB") != NULL) {
            sprintf(vendor, "%x", esp_data[i].usb_vid);
            sprintf(product, "%x", esp_data[i].usb_pid);
            blobmsg_add_string(&b, "vendor", vendor);
            blobmsg_add_string(&b, "product", product);
            if (esp_data[i].usb_vid == ESP_VID &&
                esp_data[i].usb_pid == ESP_PID) {
                blobmsg_add_u64(&b, "esp", 1);
            }
        }
        blobmsg_close_table(&b, d);
        free(esp_data[i].port_name);
    }
    blobmsg_close_array(&b, c);

    ubus_send_reply(req->ctx, &req->req, b.head);
    ubus_complete_deferred_request(req->ctx, &req->req, 0);
    free(req);
    blob_buf_free(&b);
}

void on_or_off_reply(struct uloop_timeout *t) {
    struct on_or_off_request *req =
        container_of(t, struct on_or_off_request, timeout);
    int rc = 0;
    int pin = req->pin;
    int size = req->size;
    char error_info[50] = "no error";

    char port[25] = "(unknown)";
    sscanf(req->data, "%24s", port);

    char on_or_off[4] = "off";
    if (req->on_or_off)
        sprintf(on_or_off, "on");

    char *response = NULL;
    if (!(pin == 0 || pin == 2 || BETWEEN(pin, 4, 5) || BETWEEN(pin, 12, 16)))
        size = 44;
    response = (char *) calloc(1, size + 1);

    char *data = NULL;
    size_t required_size =
        snprintf(NULL, 0, "{\"action\": \"%s\", \"pin\": %i}", on_or_off, pin) +
        1;
    data = calloc(1, required_size);
    sprintf(data, "{\"action\": \"%s\", \"pin\": %i}", on_or_off, pin);

    struct sp_port *ports = NULL;
    rc = open_port_to_esp(port, &ports);
    send_to_esp(data, &ports);
    read_from_esp(response, size, &ports);
    close_port_to_esp(&ports);

    switch (rc) {
    case -1:
        sprintf(error_info, "failed to find port.");
        break;
    case -2:
        sprintf(error_info, "failed to identify the port as an USB.");
        break;
    case -3:
        sprintf(error_info, "failed to identify the port as an esp.");
        break;
    case 0:
        sprintf(error_info, "no error");
        break;
    }

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "port", port);
    blobmsg_add_json_from_string(&b, data);
    blobmsg_add_json_from_string(&b, response);
    if (strcmp(error_info, "no error"))
        blobmsg_add_string(&b, "error", error_info);
    ubus_send_reply(req->ctx, &req->req, b.head);
    ubus_complete_deferred_request(req->ctx, &req->req, 0);
    blob_buf_free(&b);

    free(req);
    free(data);
    free(response);
}

int devices(struct ubus_context *ctx, struct ubus_object *obj,
            struct ubus_request_data *req, const char *method,
            struct blob_attr *msg) {
    struct devices_request *hreq;
    struct blob_attr *tb[__DEVICES_MAX];

    hreq = calloc(1, sizeof(*hreq) + 1);
    hreq->ctx = ctx;

    ubus_defer_request(ctx, req, &hreq->req);
    hreq->timeout.cb = devices_reply;
    uloop_timeout_set(&hreq->timeout, 100);
    return 0;
}

int off(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg) {
    struct devices_request *hreq;
    struct blob_attr *tb[__OFF_MAX];
    const char *format = "%.24s";
    const char *msgport = "(unknown)";
    int on_or_off = 0;
    int msgpin = -1;
    int size = 46;

    blobmsg_parse(off_policy, ARRAY_SIZE(off_policy), tb, blob_data(msg),
                  blob_len(msg));

    if (tb[OFF_PORT])
        msgport = blobmsg_data(tb[OFF_PORT]);
    if (tb[OFF_PIN])
        msgpin = blobmsg_get_u32(tb[OFF_PIN]);

    size_t required_size =
        snprintf(NULL, 0, format, msgport, on_or_off, msgpin, size) + 1;
    hreq = calloc(1, sizeof(*hreq) + required_size);

    hreq->ctx = ctx;
    hreq->on_or_off = on_or_off;
    hreq->pin = msgpin;
    hreq->size = size;

    sprintf(hreq->data, format, msgport);
    ubus_defer_request(ctx, req, &hreq->req);
    hreq->timeout.cb = on_or_off_reply;
    uloop_timeout_set(&hreq->timeout, 100);
    return 0;
}

int on(struct ubus_context *ctx, struct ubus_object *obj,
       struct ubus_request_data *req, const char *method,
       struct blob_attr *msg) {
    struct devices_request *hreq;
    struct blob_attr *tb[__ON_MAX];
    const char *format = "%.24s";
    const char *msgport = "(unknown)";
    int on_or_off = 1;
    int msgpin = -1;
    int size = 45;

    blobmsg_parse(on_policy, ARRAY_SIZE(on_policy), tb, blob_data(msg),
                  blob_len(msg));

    if (tb[ON_PORT])
        msgport = blobmsg_data(tb[ON_PORT]);
    if (tb[ON_PIN])
        msgpin = blobmsg_get_u32(tb[ON_PIN]);

    size_t required_size =
        snprintf(NULL, 0, format, msgport, on_or_off, msgpin, size) + 1;
    hreq = calloc(1, sizeof(*hreq) + required_size);

    hreq->ctx = ctx;
    hreq->on_or_off = on_or_off;
    hreq->pin = msgpin;
    hreq->size = size;

    sprintf(hreq->data, format, msgport);
    ubus_defer_request(ctx, req, &hreq->req);
    hreq->timeout.cb = on_or_off_reply;
    uloop_timeout_set(&hreq->timeout, 100);
    return 0;
}

static const struct ubus_method esp_serial_methods[] = {
    UBUS_METHOD("devices", devices, devices_policy),
    UBUS_METHOD("off", off, off_policy),
    UBUS_METHOD("on", on, on_policy),
};

static struct ubus_object_type esp_serial_object_type =
    UBUS_OBJECT_TYPE("esp_serial", esp_serial_methods);

static struct ubus_object esp_serial_object = {
    .name = "esp_serial",
    .type = &esp_serial_object_type,
    .methods = esp_serial_methods,
    .n_methods = ARRAY_SIZE(esp_serial_methods),
};

void server_main(struct ubus_context *ctx) {
    ubus_add_object(ctx, &esp_serial_object);
    ubus_register_subscriber(ctx, &esp_serial_event);
    uloop_run();
}
