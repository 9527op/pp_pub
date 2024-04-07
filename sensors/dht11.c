// #include "FreeRTOS.h"
#include "bflb_gpio.h"
#include "bflb_mtimer.h"


#define DBG_TAG "DHT11"
#include "log.h"

// just adjust to 23, old is 33
#define DHT_PIN 23

volatile float dht11_humidity =0;
volatile float dht11_temperature =0;


// -------------------------------------------------
extern uint8_t STORG_temperature;
extern uint8_t STORG_humidity; 
//
// -------------------------------------------------
 

void DHT_START()
{
    STORG_temperature = 0xff;
    STORG_humidity = 0xff;

    struct bflb_device_s *gpio;
    gpio = bflb_device_get_by_name("gpio");

    
    // LOG_I("A dht read\r\n");
    // 初始化GPIO引脚
    bflb_gpio_init(gpio, DHT_PIN, GPIO_OUTPUT | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    // 发送开始信号
    bflb_gpio_set(gpio, DHT_PIN);
    bflb_mtimer_delay_ms(50);
    bflb_gpio_reset(gpio, DHT_PIN);
    bflb_mtimer_delay_ms(20);
    bflb_gpio_set(gpio, DHT_PIN);
    bflb_mtimer_delay_us(30);

    // 切换为输入模式并等待DHT11的响应
    bflb_gpio_init(gpio, DHT_PIN, GPIO_INPUT | GPIO_PULLUP | GPIO_SMT_EN);

    // 判断是否响应
    bflb_mtimer_delay_us(10);
    if(bflb_gpio_read(gpio, DHT_PIN) == 1)
    {
        // 没响应
        LOG_I("没有响应 in IO-%d\r\n",DHT_PIN);
        return;
    }
    // 读取DHT11的响应
    while (bflb_gpio_read(gpio, DHT_PIN) == 0);
    while (bflb_gpio_read(gpio, DHT_PIN) == 1);

    // 读取温湿度数据 共 40bits
    // 8bit湿度整数数据+8bit湿度小数数据
    // +8bi温度整数数据+8bit温度小数数据
    // +8bit校验和

    uint8_t data[5] = {0};
    for (int i = 0; i < 5; i++)
    {
        data[i] = 0;
        for (int j = 0; j < 8; j++)
        {
            while (bflb_gpio_read(gpio, DHT_PIN) == 0);
            bflb_mtimer_delay_us(30);
            if (bflb_gpio_read(gpio, DHT_PIN) == 1)
            {
                data[i] |= (1 << (7 - j));
                while (bflb_gpio_read(gpio, DHT_PIN) == 1);
            }
        }
    }

    // 关闭GPIO
    bflb_gpio_deinit(gpio, DHT_PIN);

    // 解析数据
    uint8_t humidity_integer = data[0];
    uint8_t humidity_decimal = data[1];
    uint8_t temperature_integer = data[2];
    uint8_t temperature_decimal = data[3];

    // 计算小数部分
    dht11_humidity = humidity_integer + (humidity_decimal / 10.0);
    dht11_temperature = temperature_integer + (temperature_decimal / 10.0);

    // 数据有效
    // LOG_I("Humidity: %.1f%%\n", dht11_humidity);
    // LOG_I("Temperature: %.1fC\n", dht11_temperature);

    STORG_temperature=temperature_integer;
    STORG_humidity=humidity_integer;
}

// void dht_cycle_read(void)
// {
//     while(1)
//     {
//         // 挂起其它任务
//         vTaskSuspendAll();
//         DHT_START();
//         // 恢复其它任务
//         xTaskResumeAll();
//         LOG_I("DHT读取一次\r\n");
//         vTaskDelay(1000/portTICK_PERIOD_MS);
//     }
// }
