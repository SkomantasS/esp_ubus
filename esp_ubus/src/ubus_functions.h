#include <libubox/uloop.h>
#include <libubus.h>

void devices_reply(struct uloop_timeout *t);
void on_or_off_reply(struct uloop_timeout *t);
int devices(struct ubus_context *ctx, struct ubus_object *obj,
            struct ubus_request_data *req, const char *method,
            struct blob_attr *msg);
int off(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);
int on(struct ubus_context *ctx, struct ubus_object *obj,
       struct ubus_request_data *req, const char *method,
       struct blob_attr *msg);
void test_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s,
                        uint32_t id);
int test_notify(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg);
int test_watch(struct ubus_context *ctx, struct ubus_object *obj,
               struct ubus_request_data *req, const char *method,
               struct blob_attr *msg);
void server_main(struct ubus_context *ctx);