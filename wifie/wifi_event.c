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
#include "bflb_uart.h"
#include "bflb_l1c.h"
#include "bflb_mtimer.h"

#include "bl616_glb.h"
#include "rfparam_adapter.h"

#include "board.h"

#define DBG_TAG "WIFI_EVENT"
#include "log.h"
//---------------------------------------------


#include "storage.h"
#include "wifi_event.h"
#include "mqtt_s.h"

#define WIFI_STACK_SIZE     (1024*4)
#define TASK_PRIORITY_FW    (16)


// wifi fw
static wifi_conf_t conf =
{
    .country_code = "CN",
};
static TaskHandle_t wifi_fw_task;
static uint32_t sta_ConnectStatus = 0;
volatile uint8_t wifi_state = 0;

xQueueHandle queue;


// init wifi fw
int wifi_start_firmware_task(void)
{
    LOG_I("Starting wifi ...");

    /* enable wifi clock */

    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_IP_WIFI_PHY | GLB_AHB_CLOCK_IP_WIFI_MAC_PHY | GLB_AHB_CLOCK_IP_WIFI_PLATFORM);
    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_WIFI);

    /* set ble controller EM Size */

    GLB_Set_EM_Sel(GLB_WRAM160KB_EM0KB);

    if (0 != rfparam_init(0, NULL, 0)) {
        LOG_I("PHY RF init failed!");
        return 0;
    }

    LOG_I("PHY RF init success!");

    /* Enable wifi irq */

    extern void interrupt0_handler(void);
    bflb_irq_attach(WIFI_IRQn, (irq_callback)interrupt0_handler, NULL);
    bflb_irq_enable(WIFI_IRQn);

    xTaskCreate(wifi_main, (char*)"fw", WIFI_STACK_SIZE, NULL, TASK_PRIORITY_FW, &wifi_fw_task);

    return 0;
}




// handle wifi event
void wifi_event_handler(uint32_t code)
{

    sta_ConnectStatus = code;
    BaseType_t xHigherPriorityTaskWoken;

    switch (code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_INIT_DONE\r\n", __func__);
            wifi_mgmr_init(&conf);
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_MGMR_DONE\r\n", __func__);

        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            // char* scan_msg = pvPortMalloc(128);
            char scan_msg[128];
            memset(scan_msg, 0, 128);
            wifi_mgmr_sta_scanlist();
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_SCAN_DONE SSID numbles:%d\r\n", __func__, wifi_mgmr_sta_scanlist_nums_get());
            sprintf(scan_msg, "{\"wifi_scan\":{\"status\":0}}");
            // xQueueSend(queue, scan_msg, );
            if (wifi_mgmr_sta_scanlist_nums_get()>0) {
                xQueueSendFromISR(queue, scan_msg, &xHigherPriorityTaskWoken);
                if (xHigherPriorityTaskWoken) {
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }

            // vPortFree(scan_msg);
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_CONNECTED\r\n", __func__);
            void mm_sec_keydump();
            mm_sec_keydump();
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {

            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_GOT_IP\r\n", __func__);
            wifi_state=1;
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_DISCONNECT\r\n", __func__);
            wifi_state=0;
            // flash_erase_set(PASS_KEY, "00");
            // GLB_SW_System_Reset();

            // char* queue_buff = pvPortMalloc(128);
            // char queue_buff[128];
            // memset(queue_buff, 0, 128);
            // sprintf(queue_buff, "{\"wifi_disconnect\":true}");
            // xQueueSendFromISR(queue, queue_buff, pdTRUE);
            // vPortFree(queue_buff);

        }
        break;
        case CODE_WIFI_ON_AP_STARTED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_AP_STARTED\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STOPPED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_AP_STOPPED\r\n", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STA_ADD:
        {
            LOG_I("[APP] [EVT] [AP] [ADD] %lld\r\n", xTaskGetTickCount());
        }
        break;
        case CODE_WIFI_ON_AP_STA_DEL:
        {
            LOG_I("[APP] [EVT] [AP] [DEL] %lld\r\n", xTaskGetTickCount());
        }
        break;
        default:
        {
            LOG_I("[APP] [EVT] Unknown code %u \r\n", code);
        }
    }
}

// --------------------------------------------------------------



uint8_t wifi_connect(char* ssid, char* passwd)
{
    int ret = 255;
    // struct fhost_vif_ip_addr_cfg ip_cfg = { 0 };
    uint32_t ipv4_addr = 0;
    // char* queue_buff = pvPortMalloc(128);
    char queue_buff[128];
    memset(queue_buff, 0, 128);
    // 
    char* try_times;
    try_times=flash_get_data(TRY_TIMES,2);

    if (NULL==ssid || 0==strlen(ssid)) {
        return 1;
    }

    // ap stop
    if (wifi_mgmr_ap_state_get() == 1) {
        wifi_mgmr_ap_stop();
    }
    // sta stop
    if (wifi_mgmr_sta_state_get() == 1) {
        wifi_sta_disconnect();
    }
    
    // success 0
    // fail -1
    if (wifi_sta_connect(ssid, passwd, NULL, NULL, 0, 0, 0, 1)) {
        return 4;
    }
    LOG_I("Wating wifi connet\r\n");
    // 等待连接成功
    sta_ConnectStatus = 0;
    // for (int i = 0;i<10*30;i++) 
    while (1) {
        vTaskDelay(100/portTICK_PERIOD_MS);
        switch (sta_ConnectStatus) {
            case CODE_WIFI_ON_MGMR_DONE:
                // vTaskDelay(2000);
                // LOG_I("wifi_mgmr_sta_scan:%d\r\n", wifi_mgmr_sta_scan(wifi_scan_config));
                // vPortFree(queue_buff);
                return 3;
            case CODE_WIFI_ON_SCAN_DONE:

                // LOG_I("WIFI STA SCAN DONE %s\r\n", wifi_scan_config[0].ssid_array);

                // vPortFree(queue_buff);
                return 2;
            case CODE_WIFI_ON_DISCONNECT:	//连接失败（超过了重连次数还没有连接成功的状态）
                LOG_I("Wating wifi connet Faild\r\n");
                // wifi_sta_disconnect();

                if(try_times!=NULL && try_times[0]=='3')
                {
                    flash_erase_set(TRY_TIMES, "2");
                    vTaskDelay(100/portTICK_PERIOD_MS);
                    LOG_I("wifi connet Faild %s\r\n",try_times);
                }
                else if(try_times!=NULL && try_times[0]=='2')
                {
                    flash_erase_set(TRY_TIMES, "1");
                    vTaskDelay(100/portTICK_PERIOD_MS);

                    LOG_I("wifi connet Faild %s\r\n",try_times);
                }
                else
                {
                    flash_erase_set(SSID_KEY, "null");
                    flash_erase_set(PASS_KEY, "null");
                    flash_erase_set(MQTT_SERVER, "");
                    flash_erase_set(MQTT_NAME, "");
                    flash_erase_set(MQTT_PASS, "");
                    vTaskDelay(100/portTICK_PERIOD_MS);

                    LOG_I("wifi connet Faild %s\r\n",try_times);
                }
                
                vTaskDelay(1000/portTICK_PERIOD_MS);
                
                // 重启
                GLB_SW_System_Reset();

                return 4;
                
            case CODE_WIFI_ON_CONNECTED:	//连接成功(表示wifi sta状态的时候表示同时获取IP(DHCP)成功，或者使用静态IP)
                LOG_I("Wating wifi connet OK \r\n");
                // break;
            case CODE_WIFI_ON_GOT_IP:
                wifi_sta_ip4_addr_get(&ipv4_addr, NULL, NULL, NULL);
                LOG_I("wifi connened %s,IP:%s\r\n", ssid, inet_ntoa(ipv4_addr));
                sprintf(queue_buff, "{\"ip\":{\"IP\":\"%s\"}}", inet_ntoa(ipv4_addr));

                flash_erase_set(SSID_KEY, ssid);
                flash_erase_set(PASS_KEY, passwd);
                xQueueSend(queue, queue_buff, portMAX_DELAY);
                LOG_I("Wating wifi connet OK and get ip OK\r\n");
                // vPortFree(queue_buff);
            
                return 0;
            default:
                //等待连接成功
                break;
        }

    }
    // vPortFree(queue_buff);
    return 14; //连接超时
}

void start_ap(void)
{
    wifi_mgmr_ap_params_t config = { 0 };

    config.channel = 3;
    config.key = USER_AP_PASSWORD;
    config.ssid = USER_AP_NAME;
    config.use_dhcpd = 1;

    if (wifi_mgmr_conf_max_sta(2) != 0) {
        return 5;
    }
    if (wifi_mgmr_ap_start(&config) == 0) {
        return 0;
    }
}

void toLink(void)
{
    char* ss_id;
    char* pa_ss;


    ss_id=flash_get_data(SSID_KEY,64);
    pa_ss=flash_get_data(PASS_KEY,64);


    for (uint8_t i=0;i<30;i++) {
        if (sta_ConnectStatus == CODE_WIFI_ON_MGMR_DONE || sta_ConnectStatus == CODE_WIFI_ON_DISCONNECT) { // WIFI管理初始化完毕
            LOG_I("start toLink again\r\n");
            wifi_connect(ss_id, pa_ss);
        }        
        vTaskDelay(2300/portTICK_PERIOD_MS);
    }
} 
