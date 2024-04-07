// 15 B
// 12 R
// 14 G

#include "shell.h"

#include "bflb_gpio.h"
#include "board.h"

#define DBG_TAG "LIGHT_RGB"
#include "log.h"

struct bflb_device_s *gpio_rgb=NULL;

void init_rgb(void)
{
    gpio_rgb = bflb_device_get_by_name("gpio");

    // just green
    bflb_gpio_init(gpio_rgb, GPIO_PIN_14, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
    // and red
    bflb_gpio_init(gpio_rgb, GPIO_PIN_12, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);
}


void start_rgb(void)
{
    // return;
    if(gpio_rgb!=NULL)
    {
        LOG_I("start rgb\r\n");
        bflb_gpio_set(gpio_rgb, GPIO_PIN_12);  
        bflb_gpio_set(gpio_rgb, GPIO_PIN_14);  
    }  
}


void end_rgb(void)
{
    if(gpio_rgb!=NULL)
    {
        LOG_I("end rgb\r\n");
        bflb_gpio_reset(gpio_rgb, GPIO_PIN_14);
        bflb_gpio_reset(gpio_rgb, GPIO_PIN_12);  
    }  
}


#ifdef CONFIG_SHELL
#include <shell.h>



SHELL_CMD_EXPORT_ALIAS(start_rgb,start_rgb,start the rgb);
SHELL_CMD_EXPORT_ALIAS(end_rgb,end_rgb,end the rgb);

#endif