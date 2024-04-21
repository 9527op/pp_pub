/****************************************************************************
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

 /****************************************************************************
  * Included Files
  ****************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "bl_fw_api.h"
#include "wifi_mgmr_ext.h"
#include "wifi_mgmr.h"

#include "bflb_irq.h"
#include "bflb_clock.h"
#include "bflb_uart.h"
#include "bflb_i2c.h"
#include "bflb_gpio.h"


#include "bl616_glb.h"
#include "rfparam_adapter.h"

#include "board.h"

#define DBG_TAG "MAIN"
#include "log.h"




// 全部
// 设置为0,语音控制端设置为1
// 最终影响sub_logic_func函数中是否逻辑处理，switch_device分支（家具端使用，非switch_device分支（语音控制端使用
#define CONTROLLER 0
#define JUST_USING_I2C0 0

// live 0
// room0 1
// door 2
#define IN_WHERE 1
#define IN_WHERE_STR "room0"

// legal mqtt_s topic HEAD (using in sub_logic_func
#define LEAGAL_SUB_TOPIC_HEAD "mytopicLegal"
// legal mqtt_s user (using in sub_logic_func
#define LEAGAL_SUB_TOPIC_USER "lzbUser"
// legal mqtt_s topic HEAD (using in sub_logic_func
#define LEAGAL_PUB_TOPIC_HEAD "mytopicLegal"
// legal mqtt_s user (using in sub_logic_func
#define LEAGAL_PUB_TOPIC_USER "lzbUser"


//mqttP_task using for Sensors
// 
volatile char *CONRRECT_MQTT_TOPIC = NULL;
uint8_t dig_read16_use = 0;
uint8_t dig_read17_use = 0;
uint8_t adc0_use = 0;
uint8_t dht11_use = 1;
// for switch_devices_task and mqttP_task using
uint8_t inDebug = 0;

// -------------------------------------------------------------------------------
// -----------------------------状态变量-------------------------------------------
// -------------------------------------------------------------------------------

// e2prom read
// switch devices
// 
volatile uint8_t STORG_servo0State = 1;     //卧室窗帘  room0
volatile uint8_t STORG_fan0State = 0;       //卧室风扇  room0
volatile uint8_t STORG_fan1State = 0;       //客厅风扇  live
volatile uint8_t STORG_light0State = 0;     //卧室灯 room0
volatile uint8_t STORG_light1State = 0;     //客厅灯 live

// adc
volatile uint8_t STORG_adc0Cha = 0;
volatile int32_t STORG_adc0Val = 0;
volatile int32_t STORG_switchLight = 1;

// dht11
volatile uint8_t STORG_temperature = 0xff;       //温度      ------------------>temperature_integer
volatile uint8_t STORG_humidity = 0xff;          //湿度      ------------------>humidity_integer
volatile uint8_t STORG_temperature_decimal = 0xff;       //温度      ------------------>temperature_decimal
volatile uint8_t STORG_humidity_decimal = 0xff;          //湿度      ------------------>humidity_decimal


//dig_read IO16,17
volatile uint8_t STORG_IO16RDig = 0x1f;
volatile uint8_t STORG_IO17RDig = 0x1f;

// fingerprint
volatile uint8_t STORG_openFingerprint = 0; // 通过mqtt发送
volatile uint8_t TOActionFingerprint = 0;
volatile uint16_t fingerID_END = 0;
volatile uint16_t fingerID_Unlock = 0xff;
#define fingerID_END_STR "fingerID_END"


volatile struct tm *local_time;
volatile uint8_t local_init = 0;

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------




/****************************************************************************
 * user Definitions
 ****************************************************************************/


#include "storage.h"
// #include "wifi_event.h"
// #include "mqtt_p.h"
// #include "mqtt_s.h"
#include "OLED.h"       //oldeDisplay_task

#include "fpm383.h"



/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


#define WIFI_STACK_SIZE  (1536)
#define TASK_PRIORITY_FW (16)


 /****************************************************************************
  * Private Types
  ****************************************************************************/

  /****************************************************************************
   * Private Data
   ****************************************************************************/

static struct bflb_device_s *uart0;
struct bflb_device_s *i2c0 = NULL;
struct bflb_device_s *i2c1 = NULL;

// extern void shell_init_with_task(struct bflb_device_s* shell);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

void i2c1_init(void)
{

    if (JUST_USING_I2C0 && i2c0 != NULL)
    {
        i2c1 = i2c0;
        LOG_I("JUST_USING_I2C0 is true, then i2c1 is i2c0\r\n");
        return;
    }

    struct bflb_device_s* gpio;

    gpio = bflb_device_get_by_name("gpio");
    /* I2C0_SCL */
    bflb_gpio_init(gpio, GPIO_PIN_0, GPIO_FUNC_I2C1 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* I2C0_SDA */
	bflb_gpio_init(gpio, GPIO_PIN_1, GPIO_FUNC_I2C1 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    i2c1 = bflb_device_get_by_name("i2c1");

    bflb_i2c_init(i2c1, 400000);
}

void i2c0_init(void)
{
    struct bflb_device_s* gpio;

    gpio = bflb_device_get_by_name("gpio");
    /* I2C0_SCL */
    bflb_gpio_init(gpio, GPIO_PIN_32, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    /* I2C0_SDA */
	bflb_gpio_init(gpio, GPIO_PIN_33, GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    i2c0 = bflb_device_get_by_name("i2c0");

    bflb_i2c_init(i2c0, 400000);
}

/****************************************************************************
 * Functions
 ****************************************************************************/

#define WIFI_HTTP_SERVER_STACK_SIZE  (1024 * 4)
#define WIFI_HTTP_SERVERTASK_PRIORITY (15)
// 
#define OLED_STACK_SIZE  (1024*3)
#define OLED_DISPLAY_PRIORITY (8)
// 
#define TOLINK_STACK_SIZE  (1024*5)
#define TOLINK_PRIORITY (2)
// 
#define MQTT_S_STACK_SIZE  (1024*2)
#define MQTT_S_PRIORITY (6)
// 
#define MQTT_P_STACK_SIZE  (1024*2)
#define MQTT_P_PRIORITY (6)
// 
#define E2PROM_STACK_SIZE  (1024*2)
#define E2PROM_PRIORITY (6)
// 
#define DHT11_STACK_SIZE  (1024)
#define DHT11_PRIORITY (3)
// 
#define ADC_STACK_SIZE  (1024)
#define ADC_PRIORITY (3)
// 
#define DIG_READ_STACK_SIZE  (1024)
#define DIG_READ_PRIORITY (3)
// 
#define SWITCH_DEVICES_STACK_SIZE  (1024*2)
#define SWITCH_DEVICES_PRIORITY (4)
// 
#define FINGERPRINT_STACK_SIZE  (1024*2)
#define FINGERPRINT_PRIORITY (4)
// 
#define TEST_STACK_SIZE  (1024*2)
#define TEST_PRIORITY (2)



static TaskHandle_t server_task_hd;
static TaskHandle_t oldeDisplay_task_hd;
static TaskHandle_t toLink_task_hd;
static TaskHandle_t mqttS_task_hd;
static TaskHandle_t mqttP_task_hd;
static TaskHandle_t e2prom_task_hd;
static TaskHandle_t servo_task_hd;
static TaskHandle_t dht11_task_hd;
static TaskHandle_t adc_task_hd;
static TaskHandle_t digRead_task_hd;
static TaskHandle_t switch_devices_task_hd;
static TaskHandle_t fingerprint_task_hd;
static TaskHandle_t test_task_hd;

void* MuxSem_Handle = NULL;


// -----------------------extern variants-------------------------------------------

extern void toLink(void);


extern uint8_t wifi_state;

// -----------------------extern variants end-------------------------------------------

// ---------------------------------------------------
// --------------use in mqtt_s callback---------------
uint32_t charToInt(char *oval)
{
    // used in sub_logic_func
    //
    uint32_t tmp = 0;

    for (uint8_t i = 0; i < 10; i++)
    {
        if (oval[i] == '\0')
        {
            break;
        }
        if (i != 0)
        {
            tmp *= 10;
        }

        tmp += (oval[i] - 48); //'0'
    }

    return tmp;
}

// set dropback in controller ; theRoom is debugger use
// const char* sub_rooms[]={"theRoom","room0","live","dropback"};
const char *sub_switch_devices[] = {"aDevice", "light0", "fan0", "servo0", "light1","fan1","switchLight"};

void sub_logic_func(char* topic_key,char* topic_val)
{
    if(topic_key==NULL || topic_val==NULL)
    {
        LOG_I("sub_logic_func rec null vals\r\n");
        return;
    }

    char *key_path[20];
    uint32_t senesor_tmpValue = 0;
    uint8_t i_path = 0;
    uint8_t val = 0;

    // init ptr
    for(i_path=0;i_path<20;i_path++)
    {
        key_path[i_path]=NULL;
    }
    
    // split topic 
    i_path=0;
    key_path[i_path]=strtok(topic_key,"/");
    while(key_path[i_path]!=NULL)
    {
        key_path[++i_path]=strtok(NULL,"/");
    }

    // no conrrect number of splitTopic
    if (i_path != 4)
    {
        LOG_I("the number is diff (sub_logic_func -------------i_path:%d\r\n", i_path);
        return;
    }

    // ---------------------------------------------------------------
    // ------------------------logic handle---------------------------
    // ---------------------------------------------------------------
    //
    
    // topic path
    // LOG_I("key_path[0]:%s---------------sub_logic_func\r\n", key_path[0]);
    // LOG_I("key_path[1]:%s---------------sub_logic_func\r\n", key_path[1]);
    // LOG_I("key_path[2]:%s---------------sub_logic_func\r\n", key_path[2]);
    // LOG_I("key_path[i_path - 1]:%s---------------sub_logic_func\r\n", key_path[i_path - 1]);

    if (!strcmp(key_path[0], LEAGAL_SUB_TOPIC_HEAD) && !strcmp(key_path[1], LEAGAL_SUB_TOPIC_USER))
    {
        //
        // the val is 1 or 0 switch device
        if (strlen(topic_val) == 1 && strstr(key_path[i_path - 1], "sensor") == NULL)
        {
            // non controller using
            //
            if(CONTROLLER)
            {
                goto sub_logic_func_end;
            }

            LOG_I("in %s(%s val:%d---------------sub_logic_func_forSwitchs\r\n", key_path[i_path - 2], key_path[i_path - 1], senesor_tmpValue);

            val = topic_val[0] - '0';

            uint8_t j = sizeof(sub_switch_devices) / sizeof(sub_switch_devices[0]);
            for (uint8_t i = 0; i < j; i++)
            {
                if (!(strcmp(key_path[i_path - 1], sub_switch_devices[i])))
                {
                    LOG_I("%d:%s(val:%d)------------------- in(sub_switch_devices) all:%d\r\n", i, key_path[i_path - 1], val, j);

                    // ----------------------------------------------------------------
                    // ----------------------val set: STORG_*--------------------------
                    // sub_rooms:{"theRoom","room0","live","dropback"};

                    switch (i)
                    {
                        // justice which deviceName
                    case 0:
                        /* aDevice */
                        // just all set
                        STORG_servo0State = val;
                        STORG_fan0State = val;
                        STORG_light0State = val;
                        STORG_light1State = val;
                        break;
                    case 1:
                        /* light0 */
                        STORG_light0State = val;
                        break;
                    case 2:
                        /* fan0 */
                        STORG_fan0State = val;
                        break;
                    case 3:
                        /* servo0 */
                        STORG_servo0State = val;
                        break;
                    case 4:
                        /* light1 */
                        STORG_light1State = val;
                        break;
                    case 5:
                        /* fan1 */
                        STORG_fan1State = val;
                        break;
                    case 6:
                        /* switchLight */
                        STORG_switchLight = val;
                        break;
                    case 7:
                        /* null */
                        break;
                    default:
                        break;
                    }

                    // --------------------val set: STORG_* end------------------------
                    break;
                }
            }
        }
        else
        {
            senesor_tmpValue = charToInt(topic_val);
            LOG_W("%s val:%d---------------sub_logic_func_forSensors(in %s)\r\n", key_path[i_path - 1], senesor_tmpValue, key_path[2]);
            // controller using
            //
            if(!CONTROLLER)
            {
                goto sub_logic_func_end;
            }
            LOG_I("controller using{sub_logic_func}\r\n");

            if(!strcmp(key_path[i_path - 1], "sensor0_IO16RDig"))
            {
                // IO16RDig
                STORG_IO16RDig = senesor_tmpValue;
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_IO17RDig"))
            {
                // IO17RDig
                STORG_IO17RDig = senesor_tmpValue;

            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_adc0Val"))
            {
                // adc0Val
                STORG_adc0Val = senesor_tmpValue;
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_temperature") && !strcmp(key_path[2], "live"))
            {
                // dht11_temperature
                STORG_temperature = senesor_tmpValue;
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_humidity")&& !strcmp(key_path[2], "live"))
            {
                // dht11_humidity
                STORG_humidity = senesor_tmpValue;
                
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_temperature_decimal")&& !strcmp(key_path[2], "live"))
            {
                // dht11_humidity
                STORG_temperature_decimal = senesor_tmpValue;
                LOG_W("%d---------------(in %d)\r\n", !strcmp(key_path[i_path - 1], "sensor0_temperature_decimal"), !strcmp(key_path[2], "live"));
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_humidity_decimal")&& !strcmp(key_path[2], "live"))
            {
                // dht11_humidity
                STORG_humidity_decimal = senesor_tmpValue;
                
            }
            else if(!strcmp(key_path[i_path - 1], "sensor0_openFingerprint")&& !strcmp(key_path[2], "door"))
            {
                // fpm383_fingperPrint
                // just 1
                STORG_openFingerprint = senesor_tmpValue;
            }

            // test
            // LOG_W("%d---------------(in %d)\r\n", !strcmp(key_path[i_path - 1], "sensor0_temperature_decimal"), !strcmp(key_path[2], "live"));
        }
    }
    

    // ---------------------------------------------------------------
    // ----------------------logic handle end-------------------------
    // ---------------------------------------------------------------

sub_logic_func_end:

    // switch device state
    // LOG_W("sub_logic_func STORG_fan0State:%02x\r\n",STORG_fan0State);
    // LOG_W("sub_logic_func STORG_light0State:%02x\r\n",STORG_light0State);
    // LOG_W("sub_logic_func STORG_light1State:%02x\r\n",STORG_light1State);
    // LOG_W("sub_logic_func STORG_servo0State:%02x\r\n",STORG_servo0State);


    LOG_I("--------------------------------sub_logic_func end---------------------------------------\r\n\r\n");


    free(topic_key);
    free(topic_val);
    return;
}
// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
char* intToChar(uint64_t oval)
{
    uint8_t gen=0;
    uint8_t nvaln=0;
    char* nval = (char*) malloc(50);
    char* rnval = (char*) malloc(50);

    memset(nval,'\0',50);
    memset(rnval,'\0',50);

    LOG_I("intToChar oval:%d\r\n",oval);

    // just 0
    if (oval == 0)
    {
        rnval[0] = 48; //'0'
        goto intToChar_end;
    }

    while (oval % 10 != 0 || oval > 0)
    {
        gen = oval % 10;
        *(nval + nvaln) = (char)(gen + 48); //'0' --ascii值 48
        nvaln++;
        oval /= 10;
    }

    // reverse
    for (uint8_t i = 0; i < nvaln; i++)
    {
        *(rnval + i) = *(nval + nvaln - i - 1);
    }
    
intToChar_end:

    free(nval);
    nval = NULL;

    LOG_I("intToChar rnval:%s\r\n", rnval);

    return rnval;
}
// ----------------------------------------------------

void server_task(void* param)
{
    LOG_I("server_task isend\r\n");
    //AP running
    start_ap();
    // 启动http服务，也就是响应网页的服务
    mhttp_server_init();
    while (1) {
        // printf("http");
        LOG_I("server_task\r\n");
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

void toLink_task(void* param)
{
    LOG_I("toLink_task isend\r\n");
    toLink();
}

void mqttS_task(void* param)
{
    // LOG_I("mqttS_task in\r\n");
    uint8_t had_init_mqtt_sub=0;
    char topic_target[128];
    memset(topic_target,'\0',sizeof(topic_target)/sizeof(topic_target[0]));
    strcpy(topic_target,LEAGAL_SUB_TOPIC_HEAD);
    strcat(topic_target,"/#");

    char up_pub_topic[128];
    memset(up_pub_topic, '\0', sizeof(up_pub_topic) / sizeof(up_pub_topic[0]));
    strcpy(up_pub_topic, LEAGAL_PUB_TOPIC_HEAD);
    strcat(up_pub_topic, "/");
    strcat(up_pub_topic, LEAGAL_PUB_TOPIC_USER);
    strcat(up_pub_topic, "/");
    strcat(up_pub_topic, IN_WHERE_STR);
    strcat(up_pub_topic, "/");
    strcat(up_pub_topic, "up");


    while(1)
    {
        LOG_I("to mqttS_task in %d\r\n", wifi_state);
        if(wifi_state == 1 && had_init_mqtt_sub == 0)
        {
            light_yellow();

            mqtt_sub_start(topic_target);
            had_init_mqtt_sub = 1;
            LOG_I("to init_mqtt_subttS_task\r\n");

            //
            // up msg send
            mqtt_publier_a_time(up_pub_topic, "1");
        }
        else
        {
            LOG_I("to_init_mqtt_sub\r\n");
        }


        // maybe useful delete
        if(had_init_mqtt_sub == 1)
        {
            vTaskDelete(mqttS_task);
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);   
    }

}

void mqttP_task(void* param)
{
    char correct_pub_topic[128];
    char tmp_pub_topic[128];
    memset(correct_pub_topic,'\0',sizeof(correct_pub_topic)/sizeof(correct_pub_topic[0]));
    memset(tmp_pub_topic,'\0',sizeof(tmp_pub_topic)/sizeof(tmp_pub_topic[0]));
    strcpy(correct_pub_topic, LEAGAL_PUB_TOPIC_HEAD);
    strcat(correct_pub_topic, "/");
    strcat(correct_pub_topic, LEAGAL_PUB_TOPIC_USER);
    strcat(correct_pub_topic, "/");
    strcat(correct_pub_topic, IN_WHERE_STR);

    // save in 全局
    CONRRECT_MQTT_TOPIC = malloc(50);
    strcpy(CONRRECT_MQTT_TOPIC, correct_pub_topic);

    int32_t STORG_adc0Val_old = -1;           // adc0
    uint8_t STORG_IO17RDig_old = 2;           // dig_io17
    uint8_t STORG_IO16RDig_old = 2;           // dig_io16
    uint8_t STORG_temperature_old = 0xff;     // 温度
    uint8_t STORG_humidity_old = 0xff;        // 湿度
    uint8_t STORG_temperature_decimal_old = 0xff;        // 温度小
    uint8_t STORG_humidity_decimal_old = 0xff;        // 湿度小
    char *val_ptr = NULL;

    // --------------------------------
    // -----74行变量——使能发送mqtt信息---------
    // --------------------------------

    while(1)
    {
        if(!wifi_state)
        {
            LOG_I("wifi未连接 mqttP_task suspend 2s\r\n");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        // 在FPM383C_Identify发送
        // fingperrint  
        // if (STORG_openFingerprint != 0 || inDebug)
        // {
        //     LOG_I("fingperprint state send\r\n");

        //     strcpy(tmp_pub_topic, correct_pub_topic);
        //     strcat(tmp_pub_topic, "/sensor0_openFingerprint");
        //     val_ptr = intToChar(STORG_openFingerprint);

        //     mqtt_publier_a_time(tmp_pub_topic, val_ptr);

        //     STORG_openFingerprint = 0;
        //     free(val_ptr);
        //     val_ptr = NULL;
        //     vTaskDelay(100 / portTICK_PERIOD_MS);
        // }

        // dig_read
        //
        if (STORG_IO17RDig_old != STORG_IO17RDig && dig_read17_use)
        {
            strcpy(tmp_pub_topic, correct_pub_topic);
            strcat(tmp_pub_topic, "/sensor0_IO17RDig");

            val_ptr = intToChar(STORG_IO17RDig);

            mqtt_publier_a_time(tmp_pub_topic, val_ptr);
            STORG_IO17RDig_old = STORG_IO17RDig;

            free(val_ptr);
            val_ptr = NULL;
            // vTaskDelay(100/portTICK_PERIOD_MS);
        }

        if (STORG_IO16RDig_old != STORG_IO16RDig && dig_read16_use)
        {
            strcpy(tmp_pub_topic, correct_pub_topic);
            strcat(tmp_pub_topic, "/sensor0_IO16RDig");

            val_ptr = intToChar(STORG_IO16RDig);

            mqtt_publier_a_time(tmp_pub_topic, val_ptr);
            STORG_IO16RDig_old = STORG_IO16RDig;

            free(val_ptr);
            val_ptr = NULL;
            vTaskDelay(100/portTICK_PERIOD_MS);
        }

        // adc0
        if (STORG_adc0Val_old != STORG_adc0Val && adc0_use)
        {
            strcpy(tmp_pub_topic, correct_pub_topic);
            strcat(tmp_pub_topic, "/sensor0_adc0Val");

            val_ptr = intToChar(STORG_adc0Val);
            mqtt_publier_a_time(tmp_pub_topic, val_ptr);
            STORG_adc0Val_old = STORG_adc0Val;

            free(val_ptr);
            val_ptr = NULL;
            vTaskDelay(100/portTICK_PERIOD_MS);
        }

        // dht11
        if (STORG_temperature != 0xff && STORG_humidity != 0xff && dht11_use) 
        {
            

            LOG_I("DHT data mqtt pub start\r\n");
            
            
            // pub dht11 datas
            strcpy(tmp_pub_topic, correct_pub_topic);
            if (STORG_temperature_old != STORG_temperature || inDebug)
            {
                strcat(tmp_pub_topic, "/sensor0_temperature");

                val_ptr = intToChar(STORG_temperature);
                mqtt_publier_a_time(tmp_pub_topic, val_ptr);
                STORG_temperature_old = STORG_temperature;

                free(val_ptr);
                val_ptr = NULL;
            }

            strcpy(tmp_pub_topic, correct_pub_topic);
            if (STORG_humidity_old != STORG_humidity || inDebug)
            {
                strcat(tmp_pub_topic, "/sensor0_humidity");

                val_ptr = intToChar(STORG_humidity);
                mqtt_publier_a_time(tmp_pub_topic, val_ptr);
                STORG_humidity_old = STORG_humidity;

                free(val_ptr);
                val_ptr = NULL;
            }

            // STORG_temperature_decimal
            strcpy(tmp_pub_topic, correct_pub_topic);
            if (STORG_temperature_decimal_old != STORG_temperature_decimal || inDebug)
            {
                strcat(tmp_pub_topic, "/sensor0_temperature_decimal");

                val_ptr = intToChar(STORG_temperature_decimal);
                mqtt_publier_a_time(tmp_pub_topic, val_ptr);
                STORG_temperature_decimal_old = STORG_temperature_decimal;

                free(val_ptr);
                val_ptr = NULL;
            }
            // STORG_humidity_decimal
            strcpy(tmp_pub_topic, correct_pub_topic);
            if (STORG_humidity_decimal_old != STORG_humidity_decimal || inDebug)
            {
                strcat(tmp_pub_topic, "/sensor0_humidity_decimal");

                val_ptr = intToChar(STORG_humidity_decimal);
                mqtt_publier_a_time(tmp_pub_topic, val_ptr);
                STORG_humidity_decimal_old = STORG_humidity_decimal;

                free(val_ptr);
                val_ptr = NULL;
            }
        }
        

        vTaskDelay(1200/portTICK_PERIOD_MS);
    }

}

void e2prom_task(void* param)
{
    
    // e2prom_write_test();

    while(1)
    {
        // e2prom_read_test_01();

        e2prom_read_0xA0();
        e2prom_write_0xB0();

        // LOG_I("e2prom_task one finish\r\n");
        vTaskDelay(300/portTICK_PERIOD_MS);
    }
}

void servo_task(void* param)
{
    while(1)
    {
        pwm_sg90_turn_0();
        vTaskDelay(800/portTICK_PERIOD_MS);
        pwm_sg90_turn_45();
        vTaskDelay(800/portTICK_PERIOD_MS);
        pwm_sg90_turn_90();
        vTaskDelay(800/portTICK_PERIOD_MS);
        pwm_sg90_turn_135();
        vTaskDelay(800/portTICK_PERIOD_MS);
        pwm_sg90_turn_180();
        vTaskDelay(800/portTICK_PERIOD_MS);
        

        LOG_W("a sg90 circle over\r\n");

    }
}

void dht11_task(void* param)
{

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    LOG_I("DHT读取 Start\r\n");

    while(1)
    {
        STORG_temperature = 0xff;
        STORG_humidity = 0xff;

        vTaskSuspendAll();

        DHT_START();
        LOG_I("DHT读取一次\r\n");

        if (!xTaskResumeAll())
        {
            // LOG_I("taskYIELD\r\n");
            taskYIELD();
        }
        
        

        vTaskDelay(1400/portTICK_PERIOD_MS);
    }
}

void adc_task(void* param)
{
    uint32_t watchDog_adc = 1500;

    struct bflb_device_s* gpio;
    gpio = bflb_device_get_by_name("gpio");
    /* ADC_CH0 */
    bflb_gpio_init(gpio, GPIO_PIN_20, GPIO_ANALOG | GPIO_SMT_EN | GPIO_DRV_0);

    config_adc();

    while(1)
    {
        vTaskSuspendAll();

        start_adc();

        

        if (!xTaskResumeAll())
        {
            // LOG_I("taskYIELD\r\n");
            taskYIELD();
        }


        if(STORG_switchLight)
        {
            LOG_I("just light in adc_task\r\n");
            // 判断是否启动灯光
            switch (IN_WHERE)
            {
                // live 0
                // room0 1
                // STORG_light0State = 0;     //卧室灯 room0
                // STORG_light1State = 0;     // 客厅灯 live
            case 0:
                if (STORG_adc0Val < watchDog_adc)
                {
                    STORG_light1State = 0;
                    // end_rgb();
                }
                else
                {
                    STORG_light1State = 1;
                    // start_rgb();
                }
                break;

            case 1:
                if (STORG_adc0Val < watchDog_adc)
                {
                    STORG_light0State = 0;
                    // end_rgb();
                }
                else
                {
                    STORG_light0State = 1;
                    // start_rgb();
                }
                break;

            default:
                break;
            }
        }
       

        vTaskDelay(400/portTICK_PERIOD_MS);
    }
}

void digRead_task(void* param)
{
    while(1)
    {
        //get IO16,IO17 data
        read_dig();

        vTaskDelay(800 / portTICK_PERIOD_MS);
    }
}

void switch_devices_task(void* param)
{
    uint8_t STORG_servo0State_old = 2;  //卧室窗帘 room0
    uint8_t STORG_fan0State_old = 2;    //卧室风扇 room0
    uint8_t STORG_fan1State_old = 2;    //客厅风扇 live
    uint8_t STORG_light0State_old = 2; // 卧室灯 room0
    uint8_t STORG_light1State_old = 2; // 客厅灯 live



    // -------------------------------------

    while (1)
    {
        // LOG_W("STORG_servo0State{switch_devices_task}:%d\r\n", STORG_servo0State);
        // LOG_W("STORG_fan0State{switch_devices_task}:%d\r\n", STORG_fan0State);
        // LOG_W("STORG_fan1State{switch_devices_task}:%d\r\n", STORG_fan1State_old);
        // LOG_W("STORG_light0State{switch_devices_task}:%d\r\n", STORG_light0State);
        // LOG_W("STORG_light1State{switch_devices_task}:%d\r\n", STORG_light1State);
        switch (IN_WHERE)
        {
        case 0:
            /* live */
            if (STORG_light1State_old != STORG_light1State || inDebug)
            {
                if(STORG_light1State)
                {
                    start_rgb();
                }
                else
                {
                    end_rgb();
                }
                STORG_light1State_old = STORG_light1State;
            }
            if (STORG_fan1State_old != STORG_fan1State || inDebug)
            {
                if(STORG_fan1State)
                {
                    start_dig();
                }
                else
                {
                    end_dig();
                }
                STORG_fan1State_old = STORG_fan1State;
            }
            // dht11放着
            break;
        case 1:
            /* room0 */
            if (STORG_light0State_old != STORG_light0State || inDebug)
            {
                if(STORG_light0State)
                {
                    start_rgb();
                }
                else
                {
                    end_rgb();
                }
                STORG_light0State_old = STORG_light0State;
            }
            if (STORG_servo0State_old != STORG_servo0State)
            {
                if(STORG_servo0State_old!=2)
                {
                    if (STORG_servo0State)
                    {
                        // pwm_sg90_turn_180();
                        // 外设更换为motor
                        start_motor_clockwise();
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        end_motor();

                    }
                    else
                    {
                        // pwm_sg90_turn_0();
                        // 外设更换为motor
                        start_motor_cclockwise();
                        vTaskDelay(2000 / portTICK_PERIOD_MS);
                        end_motor();
                    }
                }
                
                STORG_servo0State_old = STORG_servo0State;
                // STORG_servo0State = 2;
            }
            if (STORG_fan0State_old != STORG_fan0State || inDebug)
            {
                if(STORG_fan0State)
                {
                    start_dig();
                }
                else
                {
                    end_dig();
                }
                STORG_fan0State_old = STORG_fan0State;
            }
            break;
        case 2:
            /* door */
            /* null */
            break;
        }
        vTaskDelay(400/portTICK_PERIOD_MS);
    }
}


// fingerprint
void fingerprint_task(void* param)
{

    uint8_t delete_resultCode = 0;
    
    // 初始化FPM383C指纹模块
    FPM383C_Init();

    // 清空指纹库
    // FPM383C_Empty(2000);

    // 随机id   WARNNING:未有保存在flash
    // srand((unsigned int)time(NULL));
    // fingerID_END = rand() % 59 + 1;
    // 不随机，保持ID为0

    char *fingerID_END_ptr = flash_get_data(fingerID_END_STR, 5);
    if (strlen(fingerID_END_ptr) > 0 && fingerID_END_ptr[0] != '\0')
    {
        fingerID_END=0;
        for (int i=0;i<5;i++)
        {
            if (fingerID_END_ptr[i] == '\0')
            {
                break;
            }

            if(i!=0)
            {
                fingerID_END *= 10;
            }
            fingerID_END += (uint8_t)(fingerID_END_ptr[i] - 48); //"0" 48
        }
    }
    LOG_I("fingerID_END is:%d\r\n", fingerID_END);
    // uint16_t

    // 门锁初始化
    pwm_sg90_turn_0();

    while (1)
    {

        LOG_I("TOActionFingerprint is:%d\r\n", TOActionFingerprint);

        if (TOActionFingerprint == 1)
        {
            // 注册分支
            // 注册ID为：fingerID_END + 1
            // 
            

            // 开启注册指纹，指纹ID：0—59， 超时时间尽量在 10秒左右，需要录入四次
            FPM383C_Enroll(fingerID_END + 1, 20000);
            // FPM383C_Enroll_fmanual(10000);

            // 休息600毫秒进行下次注册
            bflb_mtimer_delay_ms(600);
            // 模块休眠一下
            FPM383C_Sleep();

            TOActionFingerprint = 0;
        }
        else
        {
            // 识别分支 & TOActionFingerprint==2 时 删除该指纹信息
            //

            // 开启自动识别
            FPM383C_Identify();

            if(fingerID_Unlock!=0xff)
            {

                if (TOActionFingerprint == 2)
                {
                    delete_resultCode = FPM383C_Delete(fingerID_Unlock, 1200);
                    LOG_W("删除指纹确认码：%02x\r\n", delete_resultCode);
                    if (delete_resultCode == 0)
                    {
                        LOG_W("删除指纹成功\r\n");
                    }
                    TOActionFingerprint = 0;
                }

                fingerID_Unlock = 0xff;
                // vTaskDelay(800 / portTICK_PERIOD_MS);
                // continue;
            }
        }

        vTaskDelay(800/portTICK_PERIOD_MS);
    }
}



// ---------------------------------------------------------------------------------------------
// oled

void odisplay_door(uint8_t semicolon)
{

    char bufTime[50];
    memset(bufTime,0,50);

    OLED_Clear();

    if(local_init)
    {
        if(semicolon)
        {
            sprintf(bufTime, "%02d:%02d", local_time->tm_hour, local_time->tm_min);
        }
        else
        {
            sprintf(bufTime, "%02d %02d", local_time->tm_hour, local_time->tm_min);
        }

        OLED_ShowChinese(1, 0, "状态（门口）");
        OLED_ShowString(97, 8, bufTime, OLED_6X8);
    }
    else
    {
        OLED_ShowChinese(16, 0, "状态（门口）");
    }

    

    
    
    
    OLED_ReverseArea(0, 0, 128, 16);

    OLED_ShowString(0, 16, ">>>>>>>><<<<<<<<", OLED_8X16);
    if (STORG_light0State == 0)
    {
        OLED_ShowChinese(8, 32, "《房门：关闭》");
        OLED_ReverseArea(72, 32, 32, 16);
    }
    else if(STORG_light0State == 1)
    {
        OLED_ShowChinese(8, 32, "《房门：开启》");
        OLED_ReverseArea(72, 32, 32, 16);
    }
    else if(STORG_light0State == 2)
    {
        OLED_ShowChinese(18, 32, "《认证失败》");
    }
    OLED_ShowString(0, 48, "<<<<<<<<>>>>>>>>", OLED_8X16);

    
    
    OLED_Update();
    vTaskDelay(800 / portTICK_PERIOD_MS);
}

void odisplay_room0(uint8_t semicolon)
{
    char bufTime[50];
    memset(bufTime,0,50);

    OLED_Clear();

    if(local_init)
    {
        if(semicolon)
        {
            sprintf(bufTime, "%02d:%02d", local_time->tm_hour, local_time->tm_min);
        }
        else
        {
            sprintf(bufTime, "%02d %02d", local_time->tm_hour, local_time->tm_min);
        }

        OLED_ShowChinese(1, 0, "状态（卧室）");
        OLED_ShowString(97, 8, bufTime, OLED_6X8);
    }
    else
    {
        OLED_ShowChinese(16, 0, "状态（卧室）");
    }

    if (STORG_light0State)
    {
        OLED_ShowChinese(24, 16, "灯光：启动");
    }
    else
    {
        OLED_ShowChinese(24, 16, "灯光：关闭");
    }
    if (STORG_fan0State)
    {
        OLED_ShowChinese(24, 32, "风扇：启动");
    }
    else
    {
        OLED_ShowChinese(24, 32, "风扇：关闭");
    }
    if (STORG_servo0State)
    {
        OLED_ShowChinese(24, 48, "窗帘：开启");
    }
    else
    {
        OLED_ShowChinese(24, 48, "窗帘：关闭");
    }
    OLED_ReverseArea(0, 0, 128, 16);
    OLED_ReverseArea(72, 16, 32, 16);
    OLED_ReverseArea(72, 32, 32, 16);
    OLED_ReverseArea(72, 48, 32, 16);
    OLED_Update();
    vTaskDelay(800 / portTICK_PERIOD_MS);

}

void odisplay_live(uint8_t semicolon)
{
    uint8_t local_temperature = 0xff;
    uint8_t local_humidity = 0xff;
    uint8_t local_temperature_decimal = 0xff;
    uint8_t local_humidity_decimal = 0xff;

    char var_th[20];
    char *var_th_ptr = NULL;

    uint8_t debug = 0;
    
    // --------------------------------------------------

    char bufTime[50];
    memset(bufTime,0,50);

    OLED_Clear();
    


    if(local_init)
    {
        if(semicolon)
        {
            sprintf(bufTime, "%02d:%02d", local_time->tm_hour, local_time->tm_min);
        }
        else
        {
            sprintf(bufTime, "%02d %02d", local_time->tm_hour, local_time->tm_min);
        }

        OLED_ShowChinese(1, 0, "状态（客厅）");
        OLED_ShowString(97, 8, bufTime, OLED_6X8);
    }
    else
    {
        OLED_ShowChinese(16, 0, "状态（客厅）");
    }






    

    if (STORG_light1State)
    {
        OLED_ShowChinese(24, 16, "灯光：启动");
    }
    else
    {
        OLED_ShowChinese(24, 16, "灯光：关闭");
    }
    if (STORG_fan1State)
    {
        OLED_ShowChinese(24, 32, "风扇：启动");
    }
    else
    {
        OLED_ShowChinese(24, 32, "风扇：关闭");
    }
    
    OLED_ReverseArea(0, 0, 128, 16);
    OLED_ReverseArea(72, 16, 32, 16);
    OLED_ReverseArea(72, 32, 32, 16);

  

    // 温湿度传感数据
    OLED_ShowChinese(0, 48, "温湿度：");

    // 
    // 数据更新
    if ((STORG_temperature != 0xff && STORG_humidity != 0xff) || debug)
    {
        local_temperature = STORG_temperature;
        local_humidity = STORG_humidity;
        local_temperature_decimal = STORG_temperature_decimal;
        local_humidity_decimal = STORG_humidity_decimal;
    }
    
    if((local_temperature != 0xff && local_humidity != 0xff) || debug)
    {
        memset(var_th,'\0',20);

        var_th_ptr = intToChar(local_temperature);
        strcat(var_th,var_th_ptr);
        strcat(var_th,".");
        free(var_th_ptr);
        var_th_ptr = intToChar(local_temperature_decimal);
        strcat(var_th,var_th_ptr);
        
        strcat(var_th,"^C");     //实际是°C
        strcat(var_th," ");
        free(var_th_ptr);
        
        var_th_ptr = intToChar(local_humidity);
        strcat(var_th,var_th_ptr);
        strcat(var_th,".");
        free(var_th_ptr);
        var_th_ptr = intToChar(local_humidity_decimal);
        strcat(var_th,var_th_ptr);
        strcat(var_th,"%");

        free(var_th_ptr);
        var_th_ptr = NULL;

        // OLED_ShowString(64, 48, var_th, OLED_6X8);
    }else{
        OLED_ShowChinese(64, 48, "获取失败");
        OLED_Update();
        vTaskDelay(800 / portTICK_PERIOD_MS);
        return;
    }

    LOG_I("odisplay_live温湿度值：%s\r\n", var_th);
    if (strlen(var_th) > 7)
    {
        OLED_Update();
        for (uint8_t i = 0; i < strlen(var_th) - 6; i++)
        {
            var_th_ptr = &var_th[i];
            OLED_ClearArea(64, 48, 64, 16);
            OLED_ShowString(64, 48, var_th_ptr, OLED_8X16);
            OLED_ReverseArea(64, 48, 128, 16);
            // OLED_UpdateArea(64, 48, 64, 16);
            OLED_Update();
            vTaskDelay(600 / portTICK_PERIOD_MS);
        }
        
    }
    else
    {
        OLED_ShowString(64, 48, var_th, OLED_8X16);
        OLED_ReverseArea(64, 48, 8 * strlen(var_th), 16);
        OLED_Update();
    }
    vTaskDelay(800 / portTICK_PERIOD_MS);
}

void odisplay_controller(uint8_t semicolon)
{
    // for controller
    //

    // 无视semicolon，一直显示

    // door
    odisplay_door(1);
    vTaskDelay(400 / portTICK_PERIOD_MS);


    // live
    odisplay_live(1);
    vTaskDelay(400 / portTICK_PERIOD_MS);

    // room0
    odisplay_room0(1);
}

void oledplay_wrap(void* param)
{
    // 时间中的分号
    uint8_t display_semicolon = 1;
    while(1)
    {
        // 执行传入的函数
        ((void (*)())param)(display_semicolon);
        display_semicolon = !display_semicolon;

        vTaskDelay(400 / portTICK_PERIOD_MS);
    }
}

// ----------------------------------

void oldeDisplay_task(void* param)
{

    /*在(0, 0)位置显示字符'A'，字体大小为8*16点阵*/
	OLED_ShowChar(0, 0, 'A', OLED_8X16);
	
	/*在(16, 0)位置显示字符串"Hello World!"，字体大小为8*16点阵*/
	OLED_ShowString(16, 0, "Hello World!", OLED_8X16);
	
	/*在(0, 18)位置显示字符'A'，字体大小为6*8点阵*/
	OLED_ShowChar(0, 18, 'A', OLED_6X8);
	
	/*在(16, 18)位置显示字符串"Hello World!"，字体大小为6*8点阵*/
	OLED_ShowString(16, 18, "Hello World!", OLED_6X8);
	
	/*在(0, 28)位置显示数字12345，长度为5，字体大小为6*8点阵*/
	OLED_ShowNum(0, 28, 12345, 5, OLED_6X8);
	
	/*在(40, 28)位置显示有符号数字-66，长度为2，字体大小为6*8点阵*/
	OLED_ShowSignedNum(40, 28, -66, 2, OLED_6X8);
	
	/*在(70, 28)位置显示十六进制数字0xA5A5，长度为4，字体大小为6*8点阵*/
	OLED_ShowHexNum(70, 28, 0xA5A5, 4, OLED_6X8);
	
	/*在(0, 38)位置显示二进制数字0xA5，长度为8，字体大小为6*8点阵*/
	OLED_ShowBinNum(0, 38, 0xA5, 8, OLED_6X8);
	
	/*在(60, 38)位置显示浮点数字123.45，整数部分长度为3，小数部分长度为2，字体大小为6*8点阵*/
	OLED_ShowFloatNum(60, 38, 123.45, 3, 2, OLED_6X8);
	
	/*在(0, 48)位置显示汉字串"你好，世界。"，字体大小为固定的16*16点阵*/
	OLED_ShowChinese(0, 48, "你好，世界。");
	
	/*在(96, 48)位置显示图像，宽16像素，高16像素，图像数据为Diode数组*/
	OLED_ShowImage(96, 48, 16, 16, Diode);
	
	/*在(96, 18)位置打印格式化字符串，字体大小为6*8点阵，格式化字符串为"[%02d]"*/
	OLED_Printf(96, 18, OLED_6X8, "[%02d]", 6);
	
	/*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
	OLED_Update();


    while (1)
	{
        LOG_I("oldeDisplay_task\r\n");
		for (uint8_t i = 0; i < 4; i ++)
		{
			/*将OLED显存数组部分数据取反，从(0, i * 16)位置开始，宽128像素，高16像素*/
			OLED_ReverseArea(0, i * 16, 128, 16);
			
			/*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
			OLED_Update();
			
			/*延8000ms，观察现象*/
            vTaskDelay(800/portTICK_PERIOD_MS);
            
			/*把取反的内容翻转回来*/
			OLED_ReverseArea(0, i * 16, 128, 16);
		}
		
		/*将OLED显存数组全部数据取反*/
		OLED_Reverse();
		
		/*调用OLED_Update函数，将OLED显存数组的内容更新到OLED硬件进行显示*/
		OLED_Update();
		
		/*延时1000ms，观察现象*/            
        vTaskDelay(1000/portTICK_PERIOD_MS);
	}

    LOG_I("oldeDisplay_task isEnd\r\n");
}

void oldeDisplay_ap_task(void* param)
{
    // OLED_ShowString(0+19, 0, "Ai-HOMO", OLED_8X16);
	// OLED_ShowChinese(58+19, 0, "系统");
	char *show_wifi_name="Ai-HOMO";
	char *show_pass_name="12345678";
	char *show_url_name="http://192.168.169.1";
    OLED_ShowChinese(16, 0, "未连接服务器");

    OLED_ShowString(0, 16, "WiFi", OLED_8X16);
    OLED_ShowChinese(32, 16, "名称：");
    OLED_ShowString(80, 16, show_wifi_name, OLED_8X16);

    OLED_ShowString(0, 32, "WiFi", OLED_8X16);
    OLED_ShowChinese(32, 32, "密码：");
    OLED_ShowString(80, 32, show_pass_name, OLED_8X16);

    OLED_ShowChinese(0, 48, "配置：");
    OLED_ShowString(48, 54, show_url_name, OLED_6X8);

    OLED_Update();

    while (1)
    {   
        
        for(int i=0;i<4;i++)
        {
            OLED_ReverseArea(0, 0, 128, 16);
            OLED_Update();
            vTaskDelay(300/portTICK_PERIOD_MS);
        }

        // wifi名称
        OLED_ReverseArea(0, 16, 64, 16);
        OLED_Update();
        for(int i=0;i<strlen(show_wifi_name)-4;i++)
        {
            OLED_ClearArea(80,16,128,16);
            OLED_ShowString(80, 16, &(show_wifi_name[i]), OLED_8X16);
            
            OLED_Update();
            vTaskDelay(400/portTICK_PERIOD_MS);
        }
        OLED_ShowString(80, 16, show_wifi_name, OLED_8X16);
        OLED_ReverseArea(0, 16, 64, 16);
        OLED_Update();


        // wifi密码
        OLED_ReverseArea(0, 32, 64, 16);
        OLED_Update();
        for(int i=0;i<strlen(show_pass_name)-4;i++)
        {
            OLED_ClearArea(80,32,128,16);
            OLED_ShowString(80, 32, &(show_pass_name[i]), OLED_8X16);

            OLED_Update();
            vTaskDelay(400/portTICK_PERIOD_MS);
        }
        OLED_ShowString(80, 32, show_pass_name, OLED_8X16);
        OLED_ReverseArea(0, 32, 64, 16);
        OLED_Update();


        // 配置地址
        OLED_ReverseArea(0, 48, 32, 16);
        OLED_Update();
        for(int i=0;i<strlen(show_url_name)-10;i++)
        {
            OLED_ClearArea(48,54,128,24);
            OLED_ShowString(48, 54, &(show_url_name[i]), OLED_6X8);

            OLED_Update();
            vTaskDelay(400/portTICK_PERIOD_MS);
        }
        OLED_ShowString(48, 54, show_url_name, OLED_6X8);
        OLED_ReverseArea(0, 48, 32, 16);
        OLED_Update();




		

	}
}

void oledDisplay_test_task(void* param)
{
    OLED_ShowString(0, 0, "data read testing...", OLED_8X16);

    OLED_ShowString(0, 16, "sState:", OLED_8X16);
    // OLED_ShowString(0, 32, "sState:", OLED_8X16);
    // OLED_ShowString(0, 48, "sState:", OLED_8X16);

    while(1)
    {

        OLED_ShowHexNum(64,16,STORG_servo0State,2,OLED_8X16);

        OLED_ShowString(0,32,"chl", OLED_8X16);
        OLED_ShowNum(32,32,STORG_adc0Cha,2,OLED_8X16);
        OLED_ShowNum(64,32,STORG_adc0Val,5,OLED_8X16);
    
    	OLED_Update();
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}


// -----------------------------------

void test_task(void* param)
{
    uint8_t try_times = 10;
    uint16_t upNow_watchdog = 3600;
    uint16_t upNow_flash = 0;

    char* upNow_ptr = NULL;
    time_t now = 0;

    while (start_ntp() != 0)
    {
        vTaskDelay(3000/portTICK_PERIOD_MS);
        if (wifi_state)
        {
            if (--try_times <= 0)
            {
                vTaskDelete(test_task_hd);
            }
        }
    }
    now = getLocalTime_ntp();
    local_time = localtime(&now);
    local_init = 1;

    while(1)
    {
        // 每秒打印
        
        // LOG_W("本地时间：%d年%d月%d日 %d时%d分%d秒\r\n", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
        vTaskDelay(995/portTICK_PERIOD_MS);
        now += 1;
        if(++upNow_flash==upNow_watchdog)
        {
            upNow_flash=0;
            upNow_ptr = intToChar(now + 3600);
            flash_erase_set("THE_NTP", upNow_ptr);

            free(upNow_ptr);
            upNow_ptr=NULL;
        }
        local_time = localtime(&now);
    }








    return;

    uint8_t oled_debug = 1;

    while(1)
    {


        if(wifi_state && !oled_debug)
        {
            mqtt_publier_a_time("mytopicLegal/lzbUser/theRoom/aDevice","1");

            vTaskDelay(1500/portTICK_PERIOD_MS);

            mqtt_publier_a_time("mytopicLegal/lzbUser/theRoom/aDevice","0");

            // LOG_I("test_task a time");  
        }
        else
        {
            odisplay_controller(1);
        }
        
        vTaskDelay(1800/portTICK_PERIOD_MS);
    }
}

// 
// ---------------------------------------------------------------------------
// 
// task start entry 
void create_server_task(void)
{
    MuxSem_Handle = xSemaphoreCreateMutex();
    if (NULL != MuxSem_Handle)
    {
        printf("MuxSem_Handle creat success!\r\n");
    }

    char* ssid=NULL;
    char* pass=NULL;
    ssid=flash_get_data(SSID_KEY,64);
    pass=flash_get_data(PASS_KEY,64);


    // oled & wifi
    if(ssid!=NULL && strlen(pass)>7)
    {
        // 已有wifi记录

        xTaskCreate(toLink_task, (char*)"toLink_queue", TOLINK_STACK_SIZE, NULL, TOLINK_PRIORITY, &toLink_task_hd);
        
        // mqtt subcriber
        xTaskCreate(mqttS_task, (char*)"mqttS_proc_task", MQTT_S_STACK_SIZE, NULL, MQTT_S_PRIORITY, &mqttS_task_hd);

        // mqtt sensors states pub for deives
        // 
        xTaskCreate(mqttP_task, (char *)"mqttP_task", MQTT_P_STACK_SIZE, NULL, MQTT_P_PRIORITY, &mqttP_task_hd);


        // mqtt publisher in e2prom  for controller
        // 
        // 


        // oled
        /*OLED一定要在循环中*/
        /*如果是设备端调用oledplay_wrap，并传入对应函数名为实参*/
        /*参数有：odisplay_controller、odisplay_live、odisplay_door*/
        xTaskCreate(oledplay_wrap, (char*)"oledplay_wrap", OLED_STACK_SIZE, odisplay_room0, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);
        
        // xTaskCreate(odisplay_controller, (char*)"oldedisplay_proc_task", OLED_STACK_SIZE, NULL, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);
    }
    else
    {
        xTaskCreate(server_task, (char*)"fw", WIFI_HTTP_SERVER_STACK_SIZE, NULL, WIFI_HTTP_SERVERTASK_PRIORITY, &server_task_hd);

        // oled
        /*OLED一定要在循环中*/
        xTaskCreate(oldeDisplay_ap_task, (char*)"oldedisplay_proc_task", OLED_STACK_SIZE, NULL, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);

        // 仅测试阶段在这，正式时注视下面，使用上面的
        // xTaskCreate(oledplay_wrap, (char*)"oledplay_wrap", OLED_STACK_SIZE, odisplay_controller, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);


        // 数据读取显示
        // xTaskCreate(oledDisplay_test_task, (char*)"oldedisplay_proc_task", OLED_STACK_SIZE, NULL, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);
    }
    

    // 控制端e2prom数据读取
    // e2prom
    // xTaskCreate(e2prom_task, (char*)"e2prom_task", E2PROM_STACK_SIZE, NULL, E2PROM_PRIORITY, &e2prom_task_hd);

    // 下面默认能打开的是有关于传感器一类

    // dht11
    // xTaskCreate(dht11_task, (char*)"dht11_task", DHT11_STACK_SIZE, NULL, DHT11_PRIORITY, &dht11_task_hd);

    // adc  接光照传感器，模拟值大于1500则打开灯光，否则关闭
    xTaskCreate(adc_task, (char*)"adc_task", ADC_STACK_SIZE, NULL, ADC_PRIORITY, &adc_task_hd);

    // dig_read 人体要4.5v以上
    // xTaskCreate(digRead_task, (char*)"digRead_task", DIG_READ_STACK_SIZE, NULL, DIG_READ_PRIORITY, &digRead_task_hd);

    // --------------------------------

    // servo
    // xTaskCreate(servo_task, (char*)"servo_task", 1024*3, NULL, 4, &servo_task_hd);

    // fingerprint  WARNNING:已知接口有冲突（探明IO23:dht11 ,IO24:sg90      这两fingerprint都要用
    // fingerprint  修改为：IO26:TX ,IO28:RX
    // 
    // xTaskCreate(fingerprint_task, (char*)"fingerprint_task", FINGERPRINT_STACK_SIZE, NULL, FINGERPRINT_PRIORITY, &fingerprint_task_hd);


    //
    // switch devices
    xTaskCreate(switch_devices_task, (char*)"switch_devices_task", SWITCH_DEVICES_STACK_SIZE, NULL, SWITCH_DEVICES_PRIORITY, &switch_devices_task_hd);



    // test
    // 现在给NTP使用
    xTaskCreate(test_task, (char*)"test_task", TEST_STACK_SIZE, NULL, TEST_PRIORITY, &test_task_hd);


}

int main(void)
{
    board_init();

    flash_init();

    pwm_2_init();
    
    i2c0_init();

    i2c1_init();

    init_rgb();

    init_dig();


    // need i2c 没有连接i2c设备，则设备阻塞
	/*OLED初始化 和 e2prom msgs初始化*/

	OLED_Init();
	// e2prom_i2cMsgs_init();


    // 
    light_white();
    // 
    // config_adc();

    // -------------------------------------------------------------------------------------

    // tmp
    // flash_erase_set(SSID_KEY,SSID_DVALUE);
    // flash_erase_set(PASS_KEY,PASS_DVALUE);
    // flash_erase_set(TRY_TIMES,"3");


    // 中断
    bflb_irq_set_nlbits(4);
    bflb_irq_set_priority(37, 3, 0);
    bflb_irq_set_priority(WIFI_IRQn, 1, 0);

    // shell
    uart0 = bflb_device_get_by_name("uart0");
    shell_init_with_task(uart0);


    // wifi
    tcpip_init(NULL, NULL);
    wifi_start_firmware_task();




    create_server_task();

    vTaskStartScheduler();

    while (1) {
        // vTaskDelay(400/portTICK_PERIOD_MS);
    }
}
