#include "FreeRTOS.h"
#include "bflb_i2c.h"
#include "board.h"

#define DBG_TAG "E2PROM"
#include "log.h"

#include "e2prom.h"
#include "mqtt_p.h"


// for conrrect mqtt pub
// 
// legal mqtt_s topic HEAD (using in sub_logic_func
#define LEAGAL_PUB_TOPIC_HEAD "mytopicLegal"
// legal mqtt_s user (using in sub_logic_func
#define LEAGAL_PUB_TOPIC_USER "lzbUser"

int16_t first_read = 1;

// ------------------------------------------
extern struct bflb_device_s *i2c1;

// connect or?
extern uint8_t wifi_state;


// e2prom read
extern uint8_t STORG_servo0State;   //卧室窗帘
extern uint8_t STORG_fan0State;     //卧室风扇
extern uint8_t STORG_fan1State;     //客厅风扇
extern uint8_t STORG_light0State;   //卧室灯
extern uint8_t STORG_light1State;   //客厅灯

// adc
extern uint8_t STORG_adc0Cha;
extern int32_t STORG_adc0Val;

// 温湿度写入
extern uint8_t STORG_temperature;       //温度      ------------------>dht11_temperature
extern uint8_t STORG_humidity;          //湿度      ------------------>dht11_humidity
extern uint8_t STORG_temperature_decimal;       //温度      ------------------>temperature_decimal
extern uint8_t STORG_humidity_decimal;          //湿度      ------------------>humidity_decimal

// ------------------------------------------

// 指纹fingerprint
extern uint8_t STORG_openFingerprint; // 通过mqtt发送


// -------------------------------------------------------

struct bflb_i2c_msg_s e2prom_msgs[2];

volatile uint8_t e2prom_recData[16];
volatile uint8_t e2prom_senData[16];

//
uint8_t e2prom_i2c_addr = EEPROM_I2C_ADDRESS1;

// 初始msg 指向e2prom_i2c_addr 但两个buffer指向null(0)，e2prom_msgs[1].flags默认为0
void e2prom_i2cMsgs_init(void)
{
    LOG_I("in e2prom_i2cMsgs_init\r\n");
    // init data array
    for(uint8_t i=0;i<16;i++)
    {
        e2prom_recData[i]=0;
        e2prom_senData[i]=0;
    }
    
    e2prom_msgs[0].addr = e2prom_i2c_addr;
    e2prom_msgs[0].flags = I2C_M_NOSTOP;
    // e2prom_msgs[0].buffer = &the_page0;
    e2prom_msgs[0].buffer = 0;
    e2prom_msgs[0].length = EEPROM_SUBADDR_LEN;
    
    e2prom_msgs[1].addr = e2prom_i2c_addr;
    e2prom_msgs[1].flags = 0;
    // e2prom_msgs[1].buffer = wdata;
    e2prom_msgs[1].buffer = 0;
    e2prom_msgs[1].length = EEPROM_DATA_BUF_LEN;

}


// 指定写入页的位置，发送e2prom_senData数据
void write_page(uint8_t page)
{    
    e2prom_msgs[0].buffer = &page;
    e2prom_msgs[1].buffer = e2prom_senData; 
    e2prom_msgs[1].length = EEPROM_DATA_BUF_LEN;
    e2prom_msgs[1].flags = 0;

    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(100/portTICK_PERIOD_MS);
}

// 指定写读取页的位置，数据存入e2prom_recData
void read_page(uint8_t page)
{
    // LOG_I("in read_page\r\n");
    e2prom_msgs[0].buffer = &page;
    e2prom_msgs[1].buffer = e2prom_recData;
    e2prom_msgs[1].length = EEPROM_DATA_BUF_LEN;
    e2prom_msgs[1].flags = I2C_M_READ;
    
    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(100/portTICK_PERIOD_MS);
}


uint8_t read_byte(uint8_t location)
{
    uint8_t temp;
    e2prom_msgs[0].buffer = &location;
    // 
    e2prom_msgs[1].buffer = &temp;
    e2prom_msgs[1].length = 1;
    e2prom_msgs[1].flags = I2C_M_READ;
    
    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(10/portTICK_PERIOD_MS);

    return temp;
}

void write_byte(uint8_t val, uint8_t location)
{
    e2prom_msgs[0].buffer = &location;
    // 
    e2prom_msgs[1].buffer = &val;
    e2prom_msgs[1].length = 1;
    e2prom_msgs[1].flags = 0;

    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(100/portTICK_PERIOD_MS);
}


// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------
// 

// 
// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------




// page 4 7 9
void e2prom_write_test(void)
{
    for(uint8_t i=0;i<16;i++)
    {
        e2prom_senData[i]=(0x20+i);
        LOG_I("page4 e2prom_senData[%d]:%02x\r\n",i,e2prom_senData[i]);
    }
    write_page(EEPROM_SELECT_PAGE4);
    for(uint8_t i=0;i<16;i++)
    {
        e2prom_senData[i]=(0x60+i);
        LOG_I("page7 e2prom_senData[%d]:%02x\r\n",i+16,e2prom_senData[i]);
    }
    write_page(EEPROM_SELECT_PAGE7);
    for(uint8_t i=0;i<16;i++)
    {
        e2prom_senData[i]=(0x40+i);
        LOG_I("page9 e2prom_senData[%d]:%02x\r\n",i+16,e2prom_senData[i]);
    }
    write_page(EEPROM_SELECT_PAGE9);
}



// page 7 8 9
void e2prom_read_test(void)
{
    read_page(EEPROM_SELECT_PAGE7);
    for(uint8_t i=0;i<16;i++)
    {
        LOG_I("page7 e2prom_recData[%d]:%02x\r\n",i,e2prom_recData[i]);
    }
    read_page(EEPROM_SELECT_PAGE8);
    for(uint8_t i=0;i<16;i++)
    {
        LOG_I("page8 e2prom_recData[%d]:%02x\r\n",i+16,e2prom_recData[i]);
    }
    read_page(EEPROM_SELECT_PAGE9);
    for(uint8_t i=0;i<16;i++)
    {
        LOG_I("page9 e2prom_recData[%d]:%02x\r\n",i+16,e2prom_recData[i]);
    }
}


void e2prom_read_test_01(void)
{
    read_page(EEPROM_SELECT_PAGE1);
    LOG_I("page1 readed\r\n");
    for(uint8_t i=0;i<16;i++)
    {
        // LOG_I("page1 e2prom_recData[%d]:%02x\r\n",i,e2prom_recData[i]);
    }

    STORG_servo0State = e2prom_recData[6];
}


// test 写和读前两页
void e2prom_man_test(void)
{
    uint8_t the_page0=(0 << 4);
    uint8_t the_page1=(1 << 4);

    uint8_t wdata[32]={0};
    uint8_t rdata[32]={0};
    
    uint8_t subaddr[2] = {0x00,the_page0};

    for(uint8_t i=0; i<32;i++)
    {
        // tdata[i]=data[i];
        // LOG_W("in write_page: %02x\r\n",tdata[i]);
        wdata[i]=i;
        rdata[i]=0;
        
        LOG_I("wdata[%d]: %02x\r\n",i,wdata[i]);
    }

    e2prom_msgs[0].addr = EEPROM_I2C_ADDRESS1;
    e2prom_msgs[0].flags = I2C_M_NOSTOP;
    e2prom_msgs[0].buffer = &the_page0;
    e2prom_msgs[0].length = 1;
    e2prom_msgs[1].addr = EEPROM_I2C_ADDRESS1;
    e2prom_msgs[1].flags = 0;
    e2prom_msgs[1].buffer = wdata;
    e2prom_msgs[1].length = 16;


    // page 0
    // bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(200/portTICK_PERIOD_MS);

    // 
    // 
    // 
    subaddr[0] = the_page0;
    subaddr[1] = the_page1;
    e2prom_msgs[0].buffer = &the_page1;
    e2prom_msgs[1].addr = EEPROM_I2C_ADDRESS1;
    e2prom_msgs[1].flags = 0;
    e2prom_msgs[1].buffer = &(wdata[16]);
    e2prom_msgs[1].length = 16;
    // page 1
    // bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    vTaskDelay(200/portTICK_PERIOD_MS);



    LOG_W("Start reading...\r\n");


    // 
    // 
    // page 0
    subaddr[1] = 0x00;
    subaddr[1] = the_page0;
    e2prom_msgs[0].buffer = &the_page0;
    e2prom_msgs[1].addr = 0x50;
    e2prom_msgs[1].flags = I2C_M_READ;
    e2prom_msgs[1].buffer = rdata;
    e2prom_msgs[1].length = 16;
    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    printf("read page0 over\r\n");
    vTaskDelay(200/portTICK_PERIOD_MS);
    // page 1
    subaddr[0] = the_page0;
    subaddr[1] = the_page1;
    e2prom_msgs[0].buffer = &the_page1;
    e2prom_msgs[1].addr = 0x50;
    e2prom_msgs[1].flags = I2C_M_READ;
    e2prom_msgs[1].buffer = &(rdata[16]);
    e2prom_msgs[1].length = 16;
    bflb_i2c_transfer(i2c1, e2prom_msgs, 2);
    printf("read page1 over\r\n");
    vTaskDelay(200/portTICK_PERIOD_MS);

    for(uint8_t i=0; i<32;i++)
    {
        printf("rdata[%d]:%02x\r\n",i,rdata[i]);
    }

}







// 
// 
// 

// page 10 0xA0
void e2prom_read_0xA0(void)
{
    // 设置watch_dog的值，如果读取一致，则直接返回。
    uint8_t watch_dog_0xA0 = 0xdd;
    // 

    read_page(EEPROM_SELECT_PAGE10);
    // LOG_I("page 0xA0 readed\r\n");

    // for(uint8_t i=0;i<16;i++)
    // {
    //     LOG_W("page10 e2prom_recData[%d]:%02x\r\n",i,e2prom_recData[i]);
    // }

    if (first_read && wifi_state)
    {
        first_read = 0;
    }
    else
    {
        if (e2prom_recData[0] == watch_dog_0xA0)
        {
            // LOG_W("WARNNING NO PUB_e2prom_recData[0]:%x\r\n", e2prom_recData[0]);
            return;
        }
    }

    char correct_pub_topic[128];
    char temp_pub_topic[128];
    memset(correct_pub_topic, '\0', sizeof(correct_pub_topic) / sizeof(correct_pub_topic[0]));
    memset(correct_pub_topic, '\0', sizeof(temp_pub_topic) / sizeof(temp_pub_topic[0]));

    strcpy(correct_pub_topic, LEAGAL_PUB_TOPIC_HEAD);
    strcat(correct_pub_topic, "/");
    strcat(correct_pub_topic, LEAGAL_PUB_TOPIC_USER);

    // ------------------------------------------------------
    // rooms:"theRoom","room0","live","dropback"
    //
    // 0xA1    卧室风扇
    // 0xA2    卧室灯
    // 0xA3    客厅灯
    // 0xA4    卧室窗帘
    // 0xAB    客厅风扇

    LOG_W("STORG_fan0State:%02x\r\n", STORG_fan0State);
    LOG_W("STORG_fan1State:%02x\r\n", STORG_fan1State);
    LOG_W("STORG_light0State:%02x\r\n", STORG_light0State);
    LOG_W("STORG_light1State:%02x\r\n", STORG_light1State);
    LOG_W("STORG_servo0State:%02x\r\n", STORG_servo0State);

    if (e2prom_recData[1] == 0 || e2prom_recData[1] == 1)
    {
        if (wifi_state)
        {
            STORG_fan0State = e2prom_recData[1];
            strcpy(temp_pub_topic, correct_pub_topic);
            strcat(temp_pub_topic, "/live/fan0");

            if (STORG_fan0State)
            {
                mqtt_publier_a_time(temp_pub_topic, "1");
            }
            else
            {
                mqtt_publier_a_time(temp_pub_topic, "0");
            }
        }
    }

    if (e2prom_recData[2] == 0 || e2prom_recData[2] == 1)
    {
        if (wifi_state)
        {
            STORG_light0State = e2prom_recData[2];
            strcpy(temp_pub_topic, correct_pub_topic);
            strcat(temp_pub_topic, "/room0/light0");

            if (STORG_light0State)
            {
                mqtt_publier_a_time(temp_pub_topic, "1");
            }
            else
            {
                mqtt_publier_a_time(temp_pub_topic, "0");
            }
        }
    }

    if (e2prom_recData[3] == 0 || e2prom_recData[3] == 1)
    {
        if (wifi_state)
        {
            STORG_light1State = e2prom_recData[3];
            strcpy(temp_pub_topic, correct_pub_topic);
            strcat(temp_pub_topic, "/live/light1");

            if (STORG_light1State)
            {
                mqtt_publier_a_time(temp_pub_topic, "1");
            }
            else
            {
                mqtt_publier_a_time(temp_pub_topic, "0");
            }
        }
    }

    if (e2prom_recData[4] == 0 || e2prom_recData[4] == 1)
    {
        if (wifi_state)
        {
            STORG_servo0State = e2prom_recData[4];
            strcpy(temp_pub_topic, correct_pub_topic);
            strcat(temp_pub_topic, "/room0/servo0");

            if (STORG_servo0State)
            {
                mqtt_publier_a_time(temp_pub_topic, "1");
            }
            else
            {
                mqtt_publier_a_time(temp_pub_topic, "0");
            }
        }
    }

    if (e2prom_recData[11] == 0 || e2prom_recData[11] == 1)
    {
        if (wifi_state)
        {
            STORG_fan1State = e2prom_recData[11];
            strcpy(temp_pub_topic, correct_pub_topic);
            strcat(temp_pub_topic, "/live/fan1");

            if (STORG_fan1State)
            {
                mqtt_publier_a_time(temp_pub_topic, "1");
            }
            else
            {
                mqtt_publier_a_time(temp_pub_topic, "0");
            }
        }
    }

    // set the 0xA0 to 0xff
    if (e2prom_recData[0] != watch_dog_0xA0)
    {
        write_byte(watch_dog_0xA0, 0xA0);
    }
    LOG_W("set 0xA0 to %02x\r\n",watch_dog_0xA0);
}


// page 11 0xB0
void e2prom_write_0xB0(void)
{
    // 0xB1 temperature
    // 0xB2 humidity
    // 0xB3 temperature_decimal
    // 0xB4 humidity_decimal
    // 
    // 0xBE STORG_openFingerprint

    // empty
    for(uint8_t i=0;i<16;i++)
    {
        e2prom_senData[i]=0xFF;
    }
    // --------------------------------------------------------------------------------

    // LOG_I("STORG_temperature_decimal is %d;;;STORG_humidity_decimal is %d\r\n", STORG_temperature_decimal, STORG_humidity_decimal);

    // 0xB1
    if(STORG_temperature!=0&&STORG_temperature<220)
    {
        e2prom_senData[1]=STORG_temperature;
        // STORG_temperature = 0xFF;
    }

    // 0xB2
    if (STORG_humidity != 0 && STORG_humidity < 220)
    {
        e2prom_senData[2] = STORG_humidity;
        // STORG_humidity = 0xFF;
    }

    // 0xB3
    if (STORG_temperature_decimal < 220)
    {
        e2prom_senData[3] = STORG_temperature_decimal;
        // STORG_temperature_decimal = 0xFF;
    }

    // 0xB4
    if (STORG_humidity_decimal < 220)
    {
        e2prom_senData[4] = STORG_humidity_decimal;
        // STORG_humidity_decimal = 0xFF;
    }

    // 0xBE
    if(STORG_openFingerprint!=0&&STORG_openFingerprint<220)
    {
        e2prom_senData[14]=STORG_openFingerprint;
        // STORG_openFingerprint = 0xFF;
    }

    // --------------------------------------------------------------------------------

    for(uint8_t i=0;i<16;i++)
    {
        if (e2prom_senData[i] != 0xFF)
        {
            // for (uint8_t j = 0; j < 16; j++)
            // {
            //     LOG_W("set EEPROM_SELECT_PAGE11-----------------%d to %02x\r\n", j, e2prom_senData[j]);
            // }
            
            write_page(EEPROM_SELECT_PAGE11);
            break;
        }
    }


}




// --------------------------------------------------------------------------
// -------------------------debugger-----------------------------------------

#ifdef CONFIG_SHELL
#include <shell.h>

void eat_dag(void)
{
    uint8_t watch_dog_0xA0 = 0xbb;
    uint8_t dog_0xA0 = 0xA0;

    write_byte(watch_dog_0xA0, dog_0xA0);
    LOG_W("set 0xA0 to %02x\r\n", watch_dog_0xA0);
}



SHELL_CMD_EXPORT_ALIAS(eat_dag,eat_dag,eat dag);

#endif






