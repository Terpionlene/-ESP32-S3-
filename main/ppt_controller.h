#ifndef PPT_CONTROLLER_H
#define PPT_CONTROLLER_H

#include <esp_err.h>
#include <stdbool.h>

typedef enum {
    PPT_ACTION_NEXT = 0,
    PPT_ACTION_PREV,
    PPT_ACTION_VOLUME_UP,
    PPT_ACTION_VOLUME_DOWN,
} ppt_action_t;

typedef void (*ppt_exit_cb_t)(void);

void ppt_controller_init(const char* server_ip, int server_port);
void ppt_controller_set_server(const char* server_ip, int server_port);
void ppt_controller_get_server(char* server_ip, int* server_port);
esp_err_t ppt_controller_send_action(ppt_action_t action);
void ppt_controller_start(void);
void ppt_controller_stop(void);
bool ppt_controller_is_running(void);
void ppt_controller_set_exit_callback(ppt_exit_cb_t cb);

#endif
