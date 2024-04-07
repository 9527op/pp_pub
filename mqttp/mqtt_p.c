#include "FreeRTOS_POSIX.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <lwip/errno.h>
#include <netdb.h>

#include "utils_getopt.h"

#include "mqtt.h"
#include "shell.h"

#include "bflb_rtc.h"

#define DBG_TAG "MQTT_P"
#include "log.h"


#include "storage.h"
#include "mqtt_p.h"

uint8_t sendbuf_pub[2048]; /* sendbuf_pub should be large enough to hold multiple whole mqtt messages */
uint8_t recvbuf_pub[1024]; /* recvbuf_pub should be large enough any whole mqtt message expected to be received */

// mqtt server address
const char* addr_pub=UMQTT_TOPUB_ADDR_TEST;
const char* port_pub=UMQTT_TOPUB_PORT_TEST;

// 
const char* mqttp_pass=UMQTT_TOPUB_PASS_TEST;
const char* mqttp_name=UMQTT_TOPUB_NAME_TEST;



shell_sig_func_ptr abort_exec_pub;

static TaskHandle_t client_daemon_pub;
struct mqtt_client client_pub;
int test_sockfd_pub;
int the_sockfd_pub = -1;


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
    LOG_I("publish_callback_1 is using...\r\n");
}

/**
 * @brief The client_pub's refresher. This function triggers back-end routines to
 *        handle ingress/egress traffic to the broker.
 *
 * @note All this function needs to do is call \ref __mqtt_recv and
 *       \ref __mqtt_send every so often. I've picked 100 ms meaning that
 *       client_pub ingress/egress traffic will be handled every 100 ms.
 */
static void client_refresher(void* client_pub)
{
    while(1)
    {
        mqtt_sync((struct mqtt_client*) client_pub);
        vTaskDelay(100);
    }

}

/**
 * @brief Safelty closes the \p sockfd and cancels the \p client_daemon_pub before \c exit.
 */
static void test_close(int sig)
{
    if (test_sockfd_pub)
    {
        close(test_sockfd_pub);
    }
    // -------------------------------------------------------
    if (the_sockfd_pub)
    {
        close(the_sockfd_pub);
    }
    LOG_W("mqtt_pub stop publish to %s\r\n", addr_pub);

    abort_exec_pub(sig);


    the_sockfd_pub=-1;
    vTaskDelete(client_daemon_pub);
}


// --------------------------------------------------------------

void mqtt_publier_init()
{
    LOG_I("in mqtt_publier_init\r\n");

    int ret = 0;

    char* addr_tmp=flash_get_data(MQTT_SERVER,64);
    char* name_tmp=flash_get_data(MQTT_NAME,32);
    char* pass_tmp=flash_get_data(MQTT_PASS,32);

    if(addr_tmp!=NULL && strlen(addr_tmp)>0)
    {
        addr_pub=addr_tmp;
    }

    if(name_tmp!=NULL && strlen(addr_tmp)>0)
    {
        mqttp_name=name_tmp;
    }

    if(pass_tmp!=NULL && strlen(addr_tmp)>0)
    {
        mqttp_pass=pass_tmp;
    }




    
    // 绑定信号处理函数 shell.h
    abort_exec_pub = shell_signal(SHELL_SIGINT, test_close);
    /* open the non-blocking TCP socket (connecting to the broker) */
    the_sockfd_pub = open_nb_socket(addr_pub, port_pub);

    if (the_sockfd_pub < 0) {
        LOG_W("Failed to open socket: %d\r\n", the_sockfd_pub);
        test_close(SHELL_SIGINT);
    }

    mqtt_init(&client_pub, the_sockfd_pub, sendbuf_pub, sizeof(sendbuf_pub), recvbuf_pub, sizeof(recvbuf_pub), publish_callback_1);
    /* Create an anonymous session */
    const char* client_id = NULL;
    /* Ensure we have a clean session */
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    /* Send connection request to the broker. */
    ret = mqtt_connect(&client_pub, client_id, NULL, NULL, 0, mqttp_name, mqttp_pass, connect_flags, 200);

    if (ret != MQTT_OK)
    {
        LOG_W("fail \r\n");
    }
    /* check that we don't have any errors */
    if (client_pub.error != MQTT_OK) {
        LOG_W("error: %s\r\n", mqtt_error_str(client_pub.error));
        test_close(SHELL_SIGINT);
    }else
    {
        /* start a thread to refresh the client_pub (handle egress and ingree client_pub traffic) */
        xTaskCreate(client_refresher, (char*)"client_ref", 1024,  &client_pub, 10, &client_daemon_pub);
        LOG_I("mqtt_publier_init over\r\n");
    }
}



// rooms:"theRoom","room0","live","dropback"
// 
void mqtt_publier_a_time(char *topic, char *message)
{
    LOG_I("in mqtt_publier_a_time\r\n");

    if (the_sockfd_pub < 0) {
        LOG_I("to init socket of mqtt\r\n");
        mqtt_publier_init();
    }

    if (the_sockfd_pub < 0) {
        LOG_W("can't open mqtt server\r\n");
        return;
    }


    if(strlen(topic)<=0 || strlen(message)<=0)
    {
        LOG_W("topic or msg null\r\n");
        return;
    }

    /* start publishing the time */
    LOG_I("mqtt_p is ready to begin publishing the time.\r\n");

    /* publish the msg */
    mqtt_publish(&client_pub, topic, message, strlen(message) + 1, MQTT_PUBLISH_QOS_0);
    LOG_I("mqtt_p published : \"%s\"--\"%s\"\r\n", topic, message);

    /* check for errors */
    if (client_pub.error != MQTT_OK) {
        LOG_I("error: %s\r\n", mqtt_error_str(client_pub.error));
    }
    
    vTaskDelay(400/portTICK_PERIOD_MS);

    // close socket
    // test_close(SHELL_SIGINT);
}










#ifdef CONFIG_SHELL
#include <shell.h>

extern uint32_t wifi_state;
static int check_wifi_state(void)
{
    if (wifi_state == 1)
    {
        return 0;
    } else {
        return 1;
    }
}

int cmd_mqtt_publisher_onlyO(int argc, const char **argv)
{
    uint32_t ret = 0;

    ret = check_wifi_state();
    if (ret != 0) {
        LOG_W("your wifi not connected!\r\n");
        return 0;
    }

    // xTaskCreate(example_mqtt,(char*)"test_mqtt", 8192, argv, 10, NULL);
    mqtt_publier_a_time(UMQTT_TOPUB_TOPIC_TEST,UMQTT_TOPUB_MESSAGE_TEST);

    return 0;
}

SHELL_CMD_EXPORT_ALIAS(cmd_mqtt_publisher_onlyO, mqtt_pub_only, mqtt publisher just one);
#endif
