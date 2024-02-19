#include <libserialport.h>
#include <libubus.h>

#define ESP_VID 0x10C4
#define ESP_PID 0xEA60

struct port_data {
    int usb_vid;
    int usb_pid;
    char *port_name;
};

int get_esp_devices(struct port_data *esp_data);
int open_port_to_esp(char *port, struct sp_port **ports);
int send_to_esp(char *data, struct sp_port **tx_port);
int read_from_esp(char *response, int size_r, struct sp_port **rx_port);
int close_port_to_esp(struct sp_port **ports);