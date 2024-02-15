#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <libserialport.h>

#include "serialport_functions.h"

int get_esp_devices(struct port_data *esp_data)
{
	enum sp_return retUSB;

	struct sp_port **port_list;
	printf("Getting port list.\n");
	enum sp_return result = sp_list_ports(&port_list);
	if (result != SP_OK) {
		printf("sp_list_ports() failed!\n");
		return -1;
	}
	int i;
	for (i = 0; port_list[i] != NULL; i++) {
		struct sp_port *port = port_list[i];
		char *port_name = sp_get_port_name(port);
		printf("Found port: %s\n", port_name);
		esp_data[i].port_name = (char *) malloc(sizeof(char)*25);
		sprintf(esp_data[i].port_name,"%s",port_name);
		printf("testing: %s\n", port_name);
		if (strstr(port_name, "USB") != NULL) {
			int usb_vid = 0;
			int usb_pid = 0;
			retUSB = sp_get_port_usb_vid_pid(port, &usb_vid, &usb_pid);
            if (retUSB == 0) {
                esp_data[i].usb_vid = usb_vid;
                esp_data[i].usb_pid = usb_pid;
            }
			// if (usb_vid==4292 && usb_pid==60000) {
			
			// }
		}
	}
	printf("Found %d ports.\n", i);
	printf("Freeing port list.\n");
	sp_free_port_list(port_list);
	return i;
}

void connect_to_port_and_send_data(char *port,char *data, int size_r, char *error_info, char *response)
{
	char *buf = malloc(size_r + 1);

	printf("Looking for port %s.\n", port);
	struct sp_port *ports;
	if(sp_get_port_by_name(port, &ports)) {
		sprintf(error_info,"failed to find port.");
		printf("failed to find port.\n");
		goto cleanup;
	}
	// checking if port is esp
	enum sp_return retUSB;
	if (strstr(port, "USB") == NULL) {
		sprintf(error_info,"failed to identify the port as an USB.");
		printf("failed to identify the port as an USB.\n");
		goto cleanup;
	} else {
		int usb_vid = 0;
		int usb_pid = 0;
		retUSB = sp_get_port_usb_vid_pid(ports, &usb_vid, &usb_pid);
		if (retUSB == 0) {
			if (usb_vid != 4292 || usb_pid != 60000) {
				sprintf(error_info,"failed to identify the port as an esp.");
				printf("failed to identify the port as an esp.\n");
				goto cleanup;
			}
		}
	}

	printf("Opening port.\n");
	if(sp_open(ports, SP_MODE_READ_WRITE),error_info) {
		// sprintf(error_info,"failed to open port.");
		// printf("failed to open port.\n");
		// return;
	}
	printf("Setting port to 9600 8N1, no flow control.\n");
	if(sp_set_baudrate(ports, 9600),error_info) {
		// sprintf(error_info,"failed to set port to 9600 8N1, no flow control.");
		// printf("failed to set port to 9600 8N1, no flow control.\n");
		// return;
	}
	if(sp_set_bits(ports, 8),error_info) {
		// sprintf(error_info,"failed to set_bits.");
		// printf("failed to set_bits.\n");
		// return;
	}
	if(sp_set_parity(ports, SP_PARITY_NONE),error_info) {
		// sprintf(error_info,"failed to set parity.");
		// printf("failed to set parity.\n");
		// return;
	}
	if(sp_set_stopbits(ports, 1),error_info) {
		// sprintf(error_info,"failed to set stop bits.");
		// printf("failed to set stop bits.\n");
		// return;
	}
	if(sp_set_flowcontrol(ports, SP_FLOWCONTROL_NONE),error_info) {
		// sprintf(error_info,"failed to set flow control.");
		// printf("failed to set flow control.\n");
		// return;
	}
	
	// sending
	struct sp_port *tx_port = ports;
	struct sp_port *rx_port = ports;
	int size = strlen(data);
	unsigned int timeout = 10000;
	int result;
	printf("Sending '%s' (%d bytes) on port %s.\n", data, size, sp_get_port_name(tx_port));
	result = check(sp_nonblocking_write(tx_port, data, size),error_info);
	if (result == size)
		printf("Sent %d bytes successfully.\n", size);
	else
		printf("Timed out, %d/%d bytes sent.\n", result, size);
	
	printf("Receiving %d bytes on port %s.\n", size_r, sp_get_port_name(rx_port));
	result = check(sp_blocking_read(rx_port, buf, size_r, timeout),error_info);

	if (result == size_r)
		printf("Received %d bytes successfully.\n", size_r);
	else
		printf("Timed out, %d/%d bytes received.\n", result, size_r);

	buf[result] = '\0';
	printf("Received '%s'.\n", buf);
	sprintf(response,"%s",buf);

	cleanup:
	printf("cleaning up\n");
	free(buf);
	sp_close(ports);
	sp_free_port(ports);
	return;
}

/* Helper function for error handling. */
int check(enum sp_return result, char *error_info)
{
	/* For this example we'll just exit on any error by calling abort(). */
	char *error_message;

	switch (result) {
	case SP_ERR_ARG:
		printf("Error: Invalid argument.\n");
		sprintf(error_info,"Error: Invalid argument.\n");
		// abort();
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		printf("Error: Failed: %s\n", error_message);
		sprintf(error_info,"Error: Failed: %s\n", error_message);
		sp_free_error_message(error_message);
		// abort();
	case SP_ERR_SUPP:
		printf("Error: Not supported.\n");
		sprintf(error_info,"Error: Not supported.\n");
		// abort();
	case SP_ERR_MEM:
		printf("Error: Couldn't allocate memory.\n");
		sprintf(error_info,"Error: Couldn't allocate memory.\n");
		// abort();
	case SP_OK:
	default:
		return result;
	}
}