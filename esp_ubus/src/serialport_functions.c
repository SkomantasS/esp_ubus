#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libserialport.h>

#include "serialport_functions.h"

int
get_esp_devices(struct port_data *esp_data) {
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
        esp_data[i].port_name = (char *) malloc(sizeof(char) * 25);
        sprintf(esp_data[i].port_name, "%s", port_name);
        printf("testing: %s\n", port_name);
        if (strstr(port_name, "USB") != NULL) {
            int usb_vid = 0;
            int usb_pid = 0;
            retUSB = sp_get_port_usb_vid_pid(port, &usb_vid, &usb_pid);
            if (retUSB == 0) {
                esp_data[i].usb_vid = usb_vid;
                esp_data[i].usb_pid = usb_pid;
            }
        }
    }
    printf("Found %d ports.\n", i);
    printf("Freeing port list.\n");
    sp_free_port_list(port_list);
    return i;
}

void
connect_to_port_and_send_data(char *port, char *data, int size_r,
                              char *error_info, char *response) {
    char *buf = malloc(size_r + 1);

    printf("Looking for port %s.\n", port);
    struct sp_port *ports;
    if (sp_get_port_by_name(port, &ports)) {
        sprintf(error_info, "failed to find port.");
        printf("failed to find port.\n");
        goto cleanup;
    }
    // checking if port is esp
    if (strstr(port, "USB") == NULL) {
        sprintf(error_info, "failed to identify the port as an USB.");
        printf("failed to identify the port as an USB.\n");
        goto cleanup;
    } else {
        int usb_vid = 0;
        int usb_pid = 0;
        if (!sp_get_port_usb_vid_pid(ports, &usb_vid, &usb_pid) &&
            (usb_vid != 4292 || usb_pid != 60000)) {
            sprintf(error_info, "failed to identify the port as an esp.");
            printf("failed to identify the port as an esp.\n");
            goto cleanup;
        }
    }
    printf("Opening port.\n");
    sp_open(ports, SP_MODE_READ_WRITE);

    printf("Setting port to 9600 8N1, no flow control.\n");
    sp_set_baudrate(ports, 9600);
    sp_set_bits(ports, 8);
    sp_set_parity(ports, SP_PARITY_NONE);
    sp_set_stopbits(ports, 1);
    sp_set_flowcontrol(ports, SP_FLOWCONTROL_NONE);

    // sending
    struct sp_port *tx_port = ports;
    struct sp_port *rx_port = ports;
    int size = strlen(data);
    unsigned int timeout = 10000;
    int result;
    printf("Sending '%s' (%d bytes) on port %s.\n", data, size,
           sp_get_port_name(tx_port));
    result = sp_nonblocking_write(tx_port, data, size);
    if (result == size)
        printf("Sent %d bytes successfully.\n", size);
    else
        printf("Timed out, %d/%d bytes sent.\n", result, size);

    printf("Receiving %d bytes on port %s.\n", size_r,
           sp_get_port_name(rx_port));
    result = sp_blocking_read(rx_port, buf, size_r, timeout);

    if (result == size_r)
        printf("Received %d bytes successfully.\n", size_r);
    else
        printf("Timed out, %d/%d bytes received.\n", result, size_r);

    buf[result] = '\0';
    printf("Received '%s'.\n", buf);
    sprintf(response, "%s", buf);

cleanup:
    printf("cleaning up\n");
    free(buf);
    sp_close(ports);
    sp_free_port(ports);
    return;
}