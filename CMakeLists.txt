cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_add_include_directories(.)
sdk_add_include_directories(pwm_2)
sdk_add_include_directories(storage)
sdk_add_include_directories(mhttp)
sdk_add_include_directories(wifie)

sdk_add_include_directories(mqttp)
sdk_add_include_directories(mqtts)

sdk_add_include_directories(oled)
sdk_add_include_directories(e2prom)




# User
sdk_add_compile_definitions(-DCONFIG_CLI_CMD_ENABLE)
# sdk_add_compile_options(-DMQTT_USE_CUSTOM_SOCKET_HANDLE)
# sdk_add_compile_definitions(-DMBEDTLS_PK_PARSE_C)
# sdk_add_compile_definitions(-DBL616_DHCP_DEBUG)

target_sources(app PRIVATE
                   pwm_2/pwm_2.c
                   storage/storage.c
                   wifie/wifi_event.c
                   mhttp/mlwip_http.c
                   mqtts/mqtt_s.c
                   mqttp/mqtt_p.c
                   oled/OLED_Data.c
                   oled/OLED.c
                   e2prom/e2prom.c
                   sensors/adc.c
                   sensors/dht11.c
                   sensors/dig.c
                   light/light.c)



sdk_set_main_file(main.c)

project(wifi_mqtt_publisher)
