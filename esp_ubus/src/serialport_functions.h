#include <libubus.h>

struct port_data {
    int usb_vid;
    int usb_pid;
    char *port_name;
};

int get_esp_devices(struct port_data *esp_data);
void connect_to_port_and_send_data(char *port, char *data, int size_r,
                                   char *error_info, char *response);