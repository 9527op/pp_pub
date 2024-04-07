// publiser 
// 
// 
// 

#ifndef MQTT_P_H
#define MQTT_P_H


#include "stdint.h"

#define UMQTT_TOPUB_ADDR_TEST "192.168.6.1"
#define UMQTT_TOPUB_PORT_TEST "1883"

#define UMQTT_TOPUB_NAME_TEST "lzb"
#define UMQTT_TOPUB_PASS_TEST "lzb"


#define UMQTT_TOPUB_TOPIC_TEST "mytopic/test"
#define UMQTT_TOPUB_MESSAGE_TEST "just A msg"

void mqtt_publier_a_time(char *topic, char *message);

#endif