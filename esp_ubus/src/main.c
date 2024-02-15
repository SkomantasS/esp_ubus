
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <libserialport.h>

#include <libubox/uloop.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "ubus_functions.h"
/* Example of how to get a list of serial ports on the system.
 *
 * This example file is released to the public domain. */

struct ubus_context *ctx;

// jmp_buf buffer1;

// static void sig_handler()
// {
// 	// longjmp(buffer1,1);
// 	printf("gavau sigabrt :(");
// }

int main(int argc, char **argv)
{
	// signal(SIGABRT,sig_handler);

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
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);
	server_main(ctx);

	ubus_free(ctx);
	uloop_done();

	// printf("Freeing port list.\n");
	// sp_free_port_list(port_list); //serial
	return 0;
}