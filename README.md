# wifi6 tcp case


## Support CHIP

|      CHIP        | Remark |
|:----------------:|:------:|
|BL616/BL618       |        |

## Compile

- BL616/BL618

```bash
make CHIP=bl616 BOARD=bl616dk
```

## Flash

```bash
make flash COMX=xxx ## xxx is your com name
```

## 这里才是真正的常规信息

### $$ 目前外设支持 $$ ###
#### $$当前外设控制或输出变量，统一由在main.c文件中定义的、对应STORG\\_开头的变量存储$$ ####
```
控制(switch类)

风扇                sensors/dig.c IO18  5v小风扇
灯                  light/light.c IO12,14,15 开发板自带
舵机                pwm_2/pwm_2.c IO24
```

```
传感器(输出类)

温湿度传感器        sensors/dht11.c IO23  型号dht11
```


## 语音控制端
```c
修改main.c中的
    CONTROLLER为1
打开main函数中
    OLED_Init();
	e2prom_i2cMsgs_init();
打开create_server_task中相应任务，如
    xTaskCreate(e2prom_task, (char*)"e2prom_task", E2PROM_STACK_SIZE, NULL, E2PROM_PRIORITY, &e2prom_task_hd);  //从e2prom中读取数据
    xTaskCreate(oldeDisplay_task, (char*)"oldedisplay_proc_task", OLED_STACK_SIZE, NULL, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);  //视情况的屏幕显示
    xTaskCreate(oldeDisplay_ap_task, (char*)"oldedisplay_proc_task", OLED_STACK_SIZE, NULL, OLED_DISPLAY_PRIORITY, &oldeDisplay_task_hd);   //视情况的屏幕显示
    视情况调整
```

## 家具设备端
```c
修改main.c中的
    CONTROLLER为0
    调整以下变量为相应值
    IN_WHERE 0
    IN_WHERE_STR "live"
关闭main函数中
    OLED_Init();    //有屏幕就打开
	e2prom_i2cMsgs_init();
打开create_server_task中相应任务，如
    xTaskCreate(switch_devices_task, (char*)"switch_devices_task", SWITCH_DEVICES_STACK_SIZE, NULL, SWITCH_DEVICES_PRIORITY, &switch_devices_task_hd);  //设备状态切换
    xTaskCreate(mqttP_task, (char*)"mqttP_task", MQTT_P_STACK_SIZE, NULL, MQTT_P_PRIORITY, &mqttP_task_hd); //传感器数据上传

    视情况调整
```