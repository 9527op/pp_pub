#include "shell.h"

#include "bflb_gpio.h"
#include "board.h"

#define DBG_TAG "DIG_get_set"
#include "log.h"


extern uint8_t STORG_IO16RDig;
extern uint8_t STORG_IO17RDig;

// USING IO16,IO17,IO18

struct bflb_device_s *gpio_dig=NULL;
struct bflb_device_s *gpio_fan=NULL;


void init_dig(void)
{
    gpio_dig = bflb_device_get_by_name("gpio");
    gpio_fan = bflb_device_get_by_name("gpio");

    // input 16,17  read
    // 
    bflb_gpio_init(gpio_dig, GPIO_PIN_16, GPIO_INPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_0);
    bflb_gpio_init(gpio_dig, GPIO_PIN_17, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_0);

    // output 18    fan
    // 
    bflb_gpio_init(gpio_fan, GPIO_PIN_18, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3);
    //
    //add IO11 IO13 motor
    bflb_gpio_init(gpio_fan, GPIO_PIN_11, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3);
    bflb_gpio_init(gpio_fan, GPIO_PIN_13, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3);
    // 
}



// for IO16,IO17    read
void read_dig(void)
{
    if (gpio_dig != NULL)
    {
        if (bflb_gpio_read(gpio_dig, GPIO_PIN_16))
        {
            STORG_IO16RDig = 1;
        }
        else
        {
            STORG_IO16RDig = 0;
        }

        if (bflb_gpio_read(gpio_dig, GPIO_PIN_17))
        {
            STORG_IO17RDig = 1;
        }
        else
        {
            STORG_IO17RDig = 0;
        }
        LOG_I("GPIO_PIN_16=%02x, GPIO_PIN_17=%02x\r\n", STORG_IO16RDig, STORG_IO17RDig);
    }
}



// for IO18 IO19    fan
void start_dig(void)
{
    // 输出高电平
    if(gpio_dig!=NULL)
    {
        LOG_I("start dig\r\n");
        bflb_gpio_set(gpio_fan, GPIO_PIN_18);
    }  
    else
    {
        LOG_I("gpio_dig is NULL(dig\r\n");
    }
}

void end_dig(void)
{
    // 输出低电平
    if(gpio_dig!=NULL)
    {
        LOG_I("end dig\r\n");
        bflb_gpio_reset(gpio_fan, GPIO_PIN_18);
    }
    else
    {
        LOG_I("gpio_dig is NULL(dig\r\n");
    }
}


// for IO11 IO13    motor

void end_motor(void)
{
    // 输出低电平
    if (gpio_dig != NULL)
    {
        LOG_I("end dig\r\n");
        bflb_gpio_reset(gpio_fan, GPIO_PIN_11);
        bflb_gpio_reset(gpio_fan, GPIO_PIN_13);
    }
    else
    {
        LOG_I("gpio_dig is NULL(motor\r\n");
    }
}

void start_motor_clockwise(void)
{
    // IO11输出高电平
    // IO13输出低电平
    if (gpio_dig != NULL)
    {
        LOG_I("end dig\r\n");
        bflb_gpio_set(gpio_fan, GPIO_PIN_11);
        bflb_gpio_reset(gpio_fan, GPIO_PIN_13);

        // stop
        // bflb_mtimer_delay_ms(3000);
        // end_motor();
    }
    else
    {
        LOG_I("gpio_dig is NULL(motor\r\n");
    }
}

void start_motor_cclockwise(void)
{
    
    // IO11输出低电平
    // IO13输出高电平

    if (gpio_dig != NULL)
    {
        LOG_I("end dig\r\n");
        bflb_gpio_set(gpio_fan, GPIO_PIN_13);
        bflb_gpio_reset(gpio_fan, GPIO_PIN_11);

        // stop
        // bflb_mtimer_delay_ms(3000);
        // end_motor();
    }
    else
    {
        LOG_I("gpio_dig is NULL(motor\r\n");
    }
}




#ifdef CONFIG_SHELL
#include <shell.h>

SHELL_CMD_EXPORT_ALIAS(read_dig,read_dig,read the dig);


SHELL_CMD_EXPORT_ALIAS(start_dig,start_dig,start the dig);
SHELL_CMD_EXPORT_ALIAS(end_dig,end_dig,end the dig);
// 
// add IO11 IO13
SHELL_CMD_EXPORT_ALIAS(start_motor_clockwise,start_motor_clockwise,start clockwise the motor);
SHELL_CMD_EXPORT_ALIAS(start_motor_cclockwise,start_motor_cclockwise,start cclockwise the motor);
SHELL_CMD_EXPORT_ALIAS(end_motor,end_motor,end the motor);


#endif