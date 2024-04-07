#ifndef __MLWIP_HTTP_H_
#define __MLWIP_HTTP_H_
// #include "wm_include.h"
// #include "wm_psram.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include <string.h>
#include "task.h"
#include "semphr.h"
#include <sys/socket.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
typedef void *at_os_mutex_t;

typedef struct myparm
{
    int sc;
    u8 *buf;
}MYPARM;

void mhttp_server_init();

#endif
