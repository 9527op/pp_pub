#include "stdio.h"
#include "string.h"
#include "storage.h"
#include "easyflash.h"

#include "log.h"
#define DBG_TAG "STORAGE"


/**
 * init
*/
void flash_init(void)
{
    //init easyflash
    bflb_mtd_init();
    easyflash_init();
}

/**
 * set the value of key
*/
void flash_erase_set(char* key, char* value)
{
    size_t len = 0;
    int value_len = strlen(value);
    ef_set_and_save_env(key, value);

    memset(value, 0, strlen(value));
    ef_get_env_blob(key, value, value_len, &len);
    LOG_W("flash_erase_set %s: %s\r\n", key, value);
}

/**
 * get
*/
// static char flash_data[128];
char* flash_get_data(char* key, int len)
{
    
    static char* flash_data = NULL;
    flash_data = pvPortMalloc(len);
    memset(flash_data, 0, len);

    ef_get_env_blob(key, flash_data, len, (size_t)&len);
    LOG_W("flash_get_data %s: %s, len:%d\r\n", key, flash_data, strlen(flash_data));
    return flash_data;
}