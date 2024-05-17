#include "FreeRTOS_POSIX.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <lwip/errno.h>
#include <netdb.h>

#include <string.h>

#include "utils_getopt.h"

#include "mqtt.h"
#include "shell.h"

#define DBG_TAG "MQTT_S"
#include "log.h"


#include "storage.h"
#include "mqtt_s.h"


uint8_t sendbuf_sub[2048]; /* sendbuf_sub should be large enough to hold multiple whole mqtt messages */
uint8_t recvbuf_sub[1024]; /* recvbuf_sub should be large enough any whole mqtt message expected to be received */

// mqtt server address
const char* addr=UMQTT_TOSUB_ADDR_TEST;
const char* port=UMQTT_TOSUB_PORT_TEST;

const char* mqtts_name=UMQTT_TOSUB_NAME_TEST;
const char* mqtts_pass=UMQTT_TOSUB_PASS_TEST;

char *firstUp_ptr = NULL;

shell_sig_func_ptr abort_exec_sub;

static TaskHandle_t client_daemon_sub;
struct mqtt_client client_sub;
int the_sockfd_sub = -1;

// ---------------------------------------------------
// ---------------------------------------------------
extern void sub_logic_func(char* topic_key,char* topic_val);

// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------





/*
    A template for opening a non-blocking POSIX socket.
*/
static int open_nb_socket(const char* addr, const char* port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if(rv != 0) {
        LOG_W("Failed to open socket (getaddrinfo): %s\r\n", rv);
        // vTaskDelay(200/portTICK_PERIOD_MS);
        GLB_SW_System_Reset();
        return -1;
    }

    /* open the first possible socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        /* connect to server */
        rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if(rv == -1) {
          close(sockfd);
          sockfd = -1;
          continue;
        }
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
    if (sockfd != -1) {
        int iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);
    }

    return sockfd;
}

/**
 * @brief The function will be called whenever a PUBLISH message is received.
 */
static void publish_callback_1(void** unused, struct mqtt_response_publish *published)
{
    /* not used in this example */
    // LOG_I("publish_callback_1 is using...\r\n");
    /* note that published->topic_name is NOT null-terminated (here we'll change it to a c-string) */
    char* topic_name = (char*) malloc(published->topic_name_size + 1);
    memcpy(topic_name, published->topic_name, published->topic_name_size);
    topic_name[published->topic_name_size] = '\0';

    char* topic_msg = (char*) malloc(published->application_message_size + 1);
    memcpy(topic_msg, published->application_message, published->application_message_size);
    topic_msg[published->application_message_size] = '\0';

    // LOG_I("Received publish('%s'): %s\r\n", topic_name, topic_msg);


    sub_logic_func(topic_name,topic_msg);
    // 
    // 
    // 在sub_logic_func中free
    // if(topic_name!=NULL)
    // {
    //     free(topic_name);
    // }
    // if(topic_msg!=NULL)
    // {
    //     free(topic_msg);
    // }
    // free(topic_name);
    // free(topic_msg);
}

/**
 * @brief The client_sub's refresher. This function triggers back-end routines to
 *        handle ingress/egress traffic to the broker.
 *
 * @note All this function needs to do is call \ref __mqtt_recv and
 *       \ref __mqtt_send every so often. I've picked 100 ms meaning that
 *       client_sub ingress/egress traffic will be handled every 100 ms.
 */
static void client_refresher(void* client)
{
    while(1)
    {
        mqtt_sync((struct mqtt_client*) client);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

}

/**
 * @brief Safelty closes the \p sockfd and cancels the \p client_daemon_sub before \c exit.
 */
static void test_close(int sig)
{
    // -------------------------------------------------------
    if (the_sockfd_sub)
    {
        close(the_sockfd_sub);
    }
    LOG_W("mqtt_pub stop publish to %s\r\n", addr);

    abort_exec_sub(sig);


    the_sockfd_sub=-1;
    vTaskDelete(client_daemon_sub);
}


// --------------------------------------------------------------

void mqtt_subscriber_init()
{
    LOG_I("in mqtt_subscriber_init\r\n");

    int ret = 0;
    
    
    char* addr_tmp=flash_get_data(MQTT_SERVER,64);
    char* name_tmp=flash_get_data(MQTT_NAME,32);
    char* pass_tmp=flash_get_data(MQTT_PASS,32);

    if(addr_tmp!=NULL && strlen(addr_tmp)>0)
    {
        addr=addr_tmp;
    }

    if(name_tmp!=NULL && strlen(name_tmp)>0)
    {
        mqtts_name=name_tmp;
    }

    if(pass_tmp!=NULL && strlen(pass_tmp)>0)
    {
        mqtts_pass=pass_tmp;
    }




    // 绑定信号处理函数 shell.h
    abort_exec_sub = shell_signal(SHELL_SIGINT, test_close);
    /* open the non-blocking TCP socket (connecting to the broker) */
    the_sockfd_sub = open_nb_socket(addr, port);

    if (the_sockfd_sub < 0) {
        LOG_W("Failed to open socket: %d\r\n", the_sockfd_sub);
        test_close(SHELL_SIGINT);
    }

    mqtt_init(&client_sub, the_sockfd_sub, sendbuf_sub, sizeof(sendbuf_sub), recvbuf_sub, sizeof(recvbuf_sub), publish_callback_1);
    /* Create an anonymous session */
    const char* client_id = NULL;
    /* Ensure we have a clean session */
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    /* Send connection request to the broker. */
    ret = mqtt_connect(&client_sub, client_id, NULL, NULL, 0, mqtts_name, mqtts_pass, connect_flags, 200);

    if (ret != MQTT_OK)
    {
        LOG_W("fail \r\n");
    }
    /* check that we don't have any errors */
    if (client_sub.error != MQTT_OK) {
        LOG_W("error: %s\r\n", mqtt_error_str(client_sub.error));
        test_close(SHELL_SIGINT);
    }else
    {
        /* start a thread to refresh the client_sub (handle egress and ingree client_sub traffic) */
        xTaskCreate(client_refresher, (char*)"client_ref", 1024*4,  &client_sub, 10, &client_daemon_sub);
        LOG_I("mqtt_publier_init over\r\n");
    }
}


static void mqtt_subscriber_a_topic(char *topic)
{
    LOG_I("in mqtt_subscriber_a_topic\r\n");

    if(strlen(topic)<=0)
    {
        LOG_W("topic or msg null\r\n");
        return;
    }


    if (the_sockfd_sub < 0) {
        LOG_I("to init socket of mqtt\r\n");
        mqtt_subscriber_init();
    }else{
        LOG_W("had socket open, maybe the topic is ready listening\r\n");
        // return;  ?
    }

    if (the_sockfd_sub < 0) {
        LOG_W("can't open mqtt server\r\n");
        return;
    }


    /* start publishing the time */
    LOG_I("mqtt_p is ready to begin subscribing the time.\r\n");

    
    /* subscribe */
    mqtt_subscribe(&client_sub, topic, 0);
    LOG_I("mqtt_p subscribed : \"%s\"\r\n", topic);

    /* check for errors */
    if (client_sub.error != MQTT_OK) {
        LOG_I("error: %s\r\n", mqtt_error_str(client_sub.error));
    }
    

    //
    // up msg send
    if (firstUp_ptr != NULL)
    {
        mqtt_publier_a_time(firstUp_ptr, "1");
    }
    
    while(1)
    {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    
    // close socket
    test_close(SHELL_SIGINT);
}





// mqtt_subscriber_a_topic(UMQTT_TOSUB_TOPIC_TEST);

void mqtt_sub_start(char *toSubTopic, char *firstUp)
{

    //
    // 复制up信息主题
    firstUp_ptr = firstUp;

    // test
    // mqtt_subscriber_a_topic(UMQTT_TOSUB_TOPIC_TEST);

    if(toSubTopic == NULL || strlen(toSubTopic)<=0)
    {
        return;
    }
    mqtt_subscriber_a_topic(toSubTopic);
}

#ifdef CONFIG_SHELL
#include <shell.h>



extern uint8_t wifi_state;
static int check_wifi_state(void)
{
    if (wifi_state == 1)
    {
        return 0;
    } else {
        return 1;
    }
}


int cmd_mqtt_subscriber_onlyO(int argc, const char **argv)
{
    uint8_t ret = 0;

    ret = check_wifi_state();
    if (ret != 0) {
        LOG_W("your wifi not connected!\r\n");
        return 0;
    }
    
    mqtt_subscriber_a_topic(UMQTT_TOSUB_TOPIC_TEST);

    return 0;
}

SHELL_CMD_EXPORT_ALIAS(cmd_mqtt_subscriber_onlyO, mqtt_sub, mqtt subscribe);
#endif
