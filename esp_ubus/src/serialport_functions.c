#include "serialport_functions.h"
#include <libserialport.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int get_esp_devices(struct port_data *esp_data) {
    int i;
    enum sp_return retUSB, result;
    struct sp_port **port_list;

    result = sp_list_ports(&port_list);
    if (result != SP_OK) {
        return -1;
    }
    for (i = 0; port_list[i] != NULL; i++) {
        struct sp_port *port = port_list[i];
        char *port_name = sp_get_port_name(port);
        esp_data[i].port_name = (char *) malloc(sizeof(char) * 25);
        sprintf(esp_data[i].port_name, "%s", port_name);
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
    sp_free_port_list(port_list);
    return i;
}

int open_port_to_esp(char *port, struct sp_port **ports) {
    int rc = 0;
    if (sp_get_port_by_name(port, ports)) {
        return rc = -1;
    }
    // checking if port is esp
    if (strstr(port, "USB") == NULL) {
        return rc = -2;
    } else {
        int usb_vid = 0;
        int usb_pid = 0;
        if (!sp_get_port_usb_vid_pid(*ports, &usb_vid, &usb_pid) &&
            (usb_vid != ESP_VID || usb_pid != ESP_PID)) {
            return rc = -3;
        }
    }
    rc = sp_open(*ports, SP_MODE_READ_WRITE);
    sp_set_baudrate(*ports, 9600);
    sp_set_bits(*ports, 8);
    sp_set_parity(*ports, SP_PARITY_NONE);
    sp_set_stopbits(*ports, 1);
    sp_set_flowcontrol(*ports, SP_FLOWCONTROL_NONE);
    return rc;
}
int send_to_esp(char *data, struct sp_port **tx_port) {
    int rc = 0;
    int size_s = strlen(data);
    printf("%s", data);
    rc = sp_nonblocking_write(*tx_port, data, size_s);
    return rc;
}
int read_from_esp(char *response, int size_r, struct sp_port **rx_port) {
    int rc = 0;
    char *buf = calloc(1, size_r + 1);
    unsigned int timeout = 10000;
    rc = sp_blocking_read(*rx_port, buf, size_r, timeout);
    sprintf(response, "%s", buf);
    free(buf);
    return rc;
}
int close_port_to_esp(struct sp_port **ports) {
    int rc = 0;
    rc = sp_close(*ports);
    sp_free_port(*ports);
    return rc;
}