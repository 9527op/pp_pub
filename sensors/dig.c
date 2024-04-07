#include "shell.h"

#include "bflb_gpio.h"
#include "board.h"

#define DBG_TAG "DIG_get_set"
#include "log.h"


// USING IO16,IO17,IO18

struct bflb_device_s *gpio_dig=NULL;


void init_dig(void)
{
    gpio_dig = bflb_device_get_by_name("gpio");

    // input 16,17
    // 
    bflb_gpio_init(gpio_dig, GPIO_PIN_16, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);

    bflb_gpio_init(gpio_dig, GPIO_PIN_17, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);

    // output 18
    // 
    bflb_gpio_init(gpio_dig, GPIO_PIN_18, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3);

}



// for IO16,IO17
void read_dig(void)
{
    if(gpio_dig!=NULL)
    {
        printf("GPIO_PIN_16=%x\r\nGPIO_PIN_17=%x\r\n", bflb_gpio_read(gpio_dig, GPIO_PIN_16), bflb_gpio_read(gpio_dig, GPIO_PIN_17));
    }
}



// for IO18
void start_dig(void)
{
    // 输出高电平
    if(gpio_dig!=NULL)
    {
        LOG_I("start dig\r\n");
        bflb_gpio_set(gpio_dig, GPIO_PIN_18);
    }  
}

void end_dig(void)
{
    // 输出低电平
    if(gpio_dig!=NULL)
    {
        LOG_I("end dig\r\n");
        bflb_gpio_reset(gpio_dig, GPIO_PIN_18);
    }  
}


#ifdef CONFIG_SHELL
#include <shell.h>

SHELL_CMD_EXPORT_ALIAS(read_dig,read_dig,read the dig);


SHELL_CMD_EXPORT_ALIAS(start_dig,start_dig,start the dig);
SHELL_CMD_EXPORT_ALIAS(end_dig,end_dig,end the dig);

#endif