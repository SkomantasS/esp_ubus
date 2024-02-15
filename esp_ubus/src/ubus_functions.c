#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <libserialport.h>

#include <libubox/uloop.h>
#include <libubox/blobmsg_json.h>
#include "ubus_functions.h"
#include "serialport_functions.h"

static struct ubus_subscriber esp_serial_event;
static struct blob_buf b;

enum {
	DEVICES_PORT,
	DEVICES_VENDOR_ID,
    DEVICES_PRODUCT_ID,
	__DEVICES_MAX
};
enum {
	OFF_PORT,
	OFF_PIN,
	__OFF_MAX
};
enum {
	ON_PORT,
	ON_PIN,
	__ON_MAX
};

static const struct blobmsg_policy devices_policy[] = {
	[DEVICES_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[DEVICES_VENDOR_ID] = { .name = "vendor", .type = BLOBMSG_TYPE_STRING },
    [DEVICES_PRODUCT_ID] = { .name = "product", .type = BLOBMSG_TYPE_STRING },
};
static const struct blobmsg_policy off_policy[] = {
	[OFF_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[OFF_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};
static const struct blobmsg_policy on_policy[] = {
	[ON_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[ON_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

struct devices_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	struct ubus_context *ctx;
	char data[];
};
struct off_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	struct ubus_context *ctx;
	char data[];
};
struct on_request {
	struct ubus_request_data req;
	struct uloop_timeout timeout;
	struct ubus_context *ctx;
	char data[];
};

void devices_reply(struct uloop_timeout *t)
{
	fprintf(stderr, "devices_reply Start\n");

	struct port_data esp_data[10];
	int count = get_esp_devices(&esp_data[0]);
	struct devices_request *req = container_of(t, struct devices_request, timeout);
	void *c, *d;

	blob_buf_init(&b, 0);
	c = blobmsg_open_array(&b, "devices");
	for (int i = 0; i<count; i++) {
		d = blobmsg_open_table(&b,"NULL");
		blobmsg_add_string(&b,"port", esp_data[i].port_name);
		if (strstr(esp_data[i].port_name, "USB") != NULL) {
			blobmsg_add_u64(&b,"vendor", esp_data[i].usb_vid);
			blobmsg_add_u64(&b,"product", esp_data[i].usb_pid);
			if (esp_data[i].usb_vid==4292 && esp_data[i].usb_pid==60000){
				blobmsg_add_u64(&b,"esp", 1);
			}
		}
		blobmsg_close_table(&b, d);
		free(esp_data[i].port_name);
	}
	blobmsg_close_array(&b,c);
	
	ubus_send_reply(req->ctx, &req->req, b.head);
	ubus_complete_deferred_request(req->ctx, &req->req, 0);
	free(req);
	blob_buf_free(&b);
	fprintf(stderr, "devices_reply End\n");
}
void off_reply(struct uloop_timeout *t)
{
	int size = 46;
	fprintf(stderr, "off_reply Start\n");

	struct off_request *req = container_of(t, struct off_request, timeout);
	char *port = NULL;
	char *data = NULL;
	char *error_info = NULL;
	char *response = NULL;
	port = (char *) malloc(sizeof(char)*25);
	data = (char *) malloc(sizeof(char)*50);
	error_info = (char *) malloc(sizeof(char)*55);
	response = (char *) malloc(sizeof(char)*size);

	int pin = -1;
	sscanf(req->data,"%s%i",port,&pin);
	sprintf(data,"{\"action\": \"off\", \"pin\": %i}",pin);

	connect_to_port_and_send_data(port,data,size,error_info,response);

	blob_buf_init(&b, 0);
	blobmsg_add_json_from_string(&b,data);
	blobmsg_add_json_from_string(&b,response);
	if (strstr(error_info, "failed to ") != NULL) {
		blobmsg_add_string(&b, "error",error_info);
	}
	ubus_send_reply(req->ctx, &req->req, b.head);
	ubus_complete_deferred_request(req->ctx, &req->req, 0);

	free(req);
	free(port);
	free(data);
	free(error_info);
	free(response);
	blob_buf_free(&b);
	fprintf(stderr, "off_reply End\n");
}
void on_reply(struct uloop_timeout *t)
{
	int size = 44;
	fprintf(stderr, "on_reply Start\n");

	struct on_request *req = container_of(t, struct on_request, timeout);
	char *port = NULL;
	char *data = NULL;
	char *error_info = NULL;
	char *response = NULL;
	port = (char *) malloc(sizeof(char)*25);
	data = (char *) malloc(sizeof(char)*50);
	error_info = (char *) malloc(sizeof(char)*55);
	response = (char *) malloc(sizeof(char)*size);

	int pin = -1;
	sscanf(req->data,"%s%i",port,&pin);
	sprintf(data,"{\"action\": \"on\", \"pin\": %i}",pin);

	connect_to_port_and_send_data(port,data,size,error_info,response);

	blob_buf_init(&b, 0);
	blobmsg_add_json_from_string(&b,data);
	blobmsg_add_json_from_string(&b,response);
	if (strstr(error_info, "failed to ") != NULL) {
		blobmsg_add_string(&b, "error",error_info);
	}
	ubus_send_reply(req->ctx, &req->req, b.head);
	ubus_complete_deferred_request(req->ctx, &req->req, 0);

	free(req);
	free(port);
	free(data);
	free(error_info);
	free(response);
	blob_buf_free(&b);
	fprintf(stderr, "on_reply End\n");
}

int devices(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	struct devices_request *hreq;
	struct blob_attr *tb[__DEVICES_MAX];
	const char *format = "[{'port':'%s'}]";
	const char *msgstr = "(unknown)";

	fprintf(stderr, "devices Start\n");
	blobmsg_parse(devices_policy, ARRAY_SIZE(devices_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[DEVICES_PORT])
		msgstr = blobmsg_data(tb[DEVICES_PORT]);

	hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(msgstr) + 1);

	hreq->ctx = ctx;

	sprintf(hreq->data, format, msgstr);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = devices_reply;
	uloop_timeout_set(&hreq->timeout, 1000);

	fprintf(stderr, "devices End\n");
	return 0;
}
int off(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	struct devices_request *hreq;
	struct blob_attr *tb[__OFF_MAX];
	const char *format = "%s %i";
	const char *msgport = "(unknown)";
	int msgpin = -1;

	fprintf(stderr, "off Start\n");
	blobmsg_parse(off_policy, ARRAY_SIZE(off_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[OFF_PORT])
		msgport = blobmsg_data(tb[OFF_PORT]);
	if (tb[OFF_PIN])
		msgpin = blobmsg_get_u32(tb[OFF_PIN]);

	hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(msgport) + sizeof(int) + 1);

	hreq->ctx = ctx;

	sprintf(hreq->data, format, msgport, msgpin);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = off_reply;
	uloop_timeout_set(&hreq->timeout, 1000);

	fprintf(stderr, "off End\n");
	return 0;
}
int on(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	struct devices_request *hreq;
	struct blob_attr *tb[__ON_MAX];
	const char *format = "%s %i";
	const char *msgport = "(unknown)";
	int msgpin = -1;

	fprintf(stderr, "on Start\n");
	blobmsg_parse(on_policy, ARRAY_SIZE(on_policy), tb, blob_data(msg), blob_len(msg));

	if (tb[ON_PORT])
		msgport = blobmsg_data(tb[ON_PORT]);
	if (tb[ON_PIN])
		msgpin = blobmsg_get_u32(tb[ON_PIN]);

	hreq = calloc(1, sizeof(*hreq) + strlen(format) + strlen(msgport) + sizeof(int) + 1);

	hreq->ctx = ctx;

	sprintf(hreq->data, format, msgport, msgpin);
	ubus_defer_request(ctx, req, &hreq->req);
	hreq->timeout.cb = on_reply;
	uloop_timeout_set(&hreq->timeout, 1000);

	fprintf(stderr, "on End\n");
	return 0;
}

void esp_serial_handle_remove(struct ubus_context *ctx, struct ubus_subscriber *s, uint32_t id)
{
	fprintf(stderr, "esp_serial_handle_remove Start\n");
	fprintf(stderr, "Object %08x went away\n", id);
	fprintf(stderr, "esp_serial_handle_remove End\n");
}

/*
	When a method is called, displays method and params
*/
int test_notify(struct ubus_context *ctx, struct ubus_object *obj,struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
#if 0
	char *str;

	str = blobmsg_format_json(msg, true);
	fprintf(stderr, "Received notification '%s': %s\n", method, str);
	free(str);
#endif
	fprintf(stderr, "test_notify Start\n");
	fprintf(stderr, "test_notify End\n");
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

void server_main(struct ubus_context *ctx)
{
	fprintf(stderr, "server_main Start\n");
	int ret;

	ret = ubus_add_object(ctx, &esp_serial_object);
	if (ret)
		fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));

	ret = ubus_register_subscriber(ctx, &esp_serial_event);
	if (ret)
		fprintf(stderr, "Failed to add on handler: %s\n", ubus_strerror(ret));

	uloop_run();
	fprintf(stderr, "server_main End\n");
}
