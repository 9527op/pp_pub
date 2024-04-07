
#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef USER_FLASH_H
#define USER_FLASH_H

// the ssid and pass for toLink 
#define SSID_KEY "SSID"
#define PASS_KEY "PASS"
#define SSID_DVALUE "_nmd"
#define PASS_DVALUE "xinhuaxian_g"

#define TRY_TIMES "TRY_TIMES"

#define MQTT_SERVER "MQTT_SERVER"
#define MQTT_NAME "MQTT_N"
#define MQTT_PASS "MQTT_P"


// void flash_init(void);
// void flash_erase_set(char* key, char* value);
// char* flash_get_data(char* key, int len);

#endif

#ifdef __cplusplus
}
#endif

#endif