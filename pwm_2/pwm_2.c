#include "bflb_mtimer.h"
#include "bflb_pwm_v2.h"
#include "bflb_clock.h"
#include "bflb_gpio.h"
#include "board.h"

#include "shell.h"


#define DBG_TAG "PWM_2"
#include "log.h"

#include "pwm_2.h"



// struct bflb_device_s *gpio;
// gpio = bflb_device_get_by_name("gpio");


// struct bflb_device_s *pwm;
// struct bflb_pwm_v2_config_s pwm_cfg = {
//     .clk_source = BFLB_SYSTEM_XCLK,
//     .clk_div = 40,
//     .period = 1000,
// };


void pwm_2_init(void)
{
    pwm = bflb_device_get_by_name("pwm_v2_0");   
    pwm_cfg.clk_source = BFLB_SYSTEM_XCLK;
    pwm_cfg.clk_div = 40;
    pwm_cfg.period = 20000;
    // 200 =1% ?


    board_pwm_gpio_init();  //in board.h    24-31
    
    bflb_pwm_v2_init(pwm, &pwm_cfg);
    bflb_pwm_v2_start(pwm);
    
}



void diy_channel(void)
{
    struct bflb_device_s *gpio;
    
    gpio = bflb_device_get_by_name("gpio");

    bflb_gpio_init(gpio, GPIO_PIN_12, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3); //12管脚对应着CH0通道
    bflb_gpio_init(gpio, GPIO_PIN_14, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3); //14管脚对应着CH2管道
    bflb_gpio_init(gpio, GPIO_PIN_15, GPIO_FUNC_PWM0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3); //15管脚对应着CH3管道

    bflb_pwm_v2_init(pwm, &pwm_cfg);
    bflb_pwm_v2_start(pwm);
    
    
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 80, 100);

    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH2);
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH2, 70, 100);

    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH3);
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH3, 60, 100);
    
}

void light_white(void)
{
    // but control yellow
    // light 29
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH3, 100, 900); /* duty = (900-100)/50 = 4%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH3);
}

void light_yellow(void)
{
    // but control white
    // light 27
    LOG_I("in light_yellow\r\n");
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH1, 100, 500); /* duty = (500-100)/50 = 2%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH1);
}


// -------------------------------------------GPIO24---------------------------------------------------
// sg90
// 20ms 50hz
// 0 -- 2.5%
// 45 -- 5%
// 90 -- 7.5%
// 135 -- 10%
// 180 -- 12.5%

// 200 = 1% ?
// 200 = 1% ?
// 200 = 1% ?

void pwm_sg90_turn_0(void)
{
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 2500, 3000); /* duty = (3000-2500)/50 = 2.5%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
}

void pwm_sg90_turn_45(void)
{
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 2000, 3000); /* duty = (3000-2000)/50 = 5%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
}

void pwm_sg90_turn_90(void)
{
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 1500, 3000); /* duty = (3000-1500)/50 = 7.5%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
}

void pwm_sg90_turn_135(void)
{
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 1000, 3000); /* duty = (3000-1000)/50 = 10%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
}

void pwm_sg90_turn_180(void)
{
    bflb_pwm_v2_channel_set_threshold(pwm, PWM_CH0, 500, 3000); /* duty = (3000-500)/50 = 12.5%? */ 
    bflb_pwm_v2_channel_positive_start(pwm, PWM_CH0);
}







#ifdef CONFIG_SHELL
#include <shell.h>

SHELL_CMD_EXPORT_ALIAS(pwm_sg90_turn_180,pwm_180,pwm_sg90_180);
SHELL_CMD_EXPORT_ALIAS(pwm_sg90_turn_0,pwm_0,pwm_sg90_180);

#endif