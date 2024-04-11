#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bflb_mtimer.h"
#include "bflb_dma.h"
#include "bflb_uart.h"
#include "bflb_gpio.h"

#include "shell.h"

#define DBG_TAG "FPM383C"
#include "log.h"

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

extern uint8_t TOActionFingerprint;
extern uint16_t fingerID_END;
extern uint16_t fingerID_Unlock;
#define fingerID_END_STR "fingerID_END"                 //保证和main.c文件中的同名预定义的值相同



// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

// 控制模块LED灯颜色
uint8_t PS_BlueLEDBuf[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x03, 0x01, 0x01, 0x00, 0x00, 0x49};
uint8_t PS_RedLEDBuf[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x02, 0x04, 0x04, 0x02, 0x00, 0x50};
uint8_t PS_GreenLEDBuf[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x02, 0x02, 0x02, 0x02, 0x00, 0x4C};

// 休眠指令-设置传感器进入休眠模式
uint8_t PS_SleepBuf[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x33, 0x00, 0x37};

// 清空指纹库-删除 flash 数据库中所有指纹模板。
uint8_t PS_EmptyBuf[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11};

// 取消指令-取消自动注册模板和自动验证指纹。如表 2-1 中加密等级设置为 0 或 1 情况下支持此功能
uint8_t PS_CancelBuf[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x30, 0x00, 0x34};

// 自动注册模板-一站式注册指纹，包含采集指纹、生成特征、组合模板、存储模板等功能。加密等级设置为 0 或 1 情况下支持此功能。
uint8_t PS_AutoEnrollBuf[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x31, '\0', '\0', 0x04, 0x00, 0x16, '\0', '\0'};

// 验证用获取图像-验证指纹时，探测手指，探测到后录入指纹图像存于图像缓冲区。返回确认码表示：录入成功、无手指等。
uint8_t PS_GetImageBuf[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};

// 生成特征值-将图像缓冲区中的原始图像生成指纹特征文件存于模板缓冲区
uint8_t PS_GetCharBuf[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};

// 搜索指纹-以模板缓冲区中的特征文件搜索整个或部分指纹库。若搜索到，则返回页码。加密等级设置为 0 或 1 情况下支持
uint8_t PS_SearchBuf[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x0C};

// 删除模板-删除 flash 数据库中指定 ID 号开始的N 个指纹模板
uint8_t PS_DeleteBuf[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x0C, '\0', '\0', 0x00, 0x01, '\0', '\0'};

// 设置名为fpm383c_uart的外设句柄,用来执行串口指令的发送
struct bflb_device_s *fpm383c_uart;

// USART串口接收长度以及标志位
uint8_t USART2_STA = 0;

// 指纹ID和验证指纹的分数
uint16_t pageID, score;

// USART串口接收缓冲数组
uint8_t USART_ReceiveBuf[30];

// 主循环状态标志位
uint8_t ScanStatus = 0;

/// @brief uart中断触发方法
/// @param irq
/// @param arg
void uart_isr(int irq, void *arg)
{
    uint32_t intstatus = bflb_uart_get_intstatus(fpm383c_uart);
    // 接收超时中断，当一段时间内数据没有接收后便会停止，在触发中断后
    uint8_t i = 0;
    if (intstatus & UART_INTSTS_RTO)
    {
        // 检测缓冲区是否有数据
        while (bflb_uart_rxavailable(fpm383c_uart))
        {
            // 轮询fpm383c_uart接收到的字符
            uint8_t data = bflb_uart_getchar(fpm383c_uart);
            // 获取数据并放入缓冲数组
            USART_ReceiveBuf[i++] = data;
        }
        // 停止接收后清空中断标志等待下一次发送
        bflb_uart_int_clear(fpm383c_uart, UART_INTCLR_RTO);
        USART2_STA = 1;
    }
}

/// @brief 初始化FPM383C指纹模块
void FPM383C_Init()
{
    // 声明 gpio句柄
    struct bflb_device_s *gpio;
    // FPM383C 模块默认波特率为 57600
    struct bflb_uart_config_s cfg = {
        .baudrate = 57600,
        .data_bits = UART_DATA_BITS_8,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .flow_ctrl = 0,
        .tx_fifo_threshold = 4,
        .rx_fifo_threshold = 4,
    };

    // 初始化FPM383C指纹模块UART
    gpio = bflb_device_get_by_name("gpio");
    fpm383c_uart = bflb_device_get_by_name("uart1");

    // 将GPIO_23和GPIO_24设置为TX和RX
    bflb_gpio_uart_init(gpio, GPIO_PIN_23, GPIO_UART_FUNC_UART1_TX);
    bflb_gpio_uart_init(gpio, GPIO_PIN_24, GPIO_UART_FUNC_UART1_RX);

    bflb_uart_init(fpm383c_uart, &cfg);

    // uart tx fifo 阈值中断屏蔽开关，开启后超过设定阈值则触发中断。
    bflb_uart_txint_mask(fpm383c_uart, true);
    // uart rx fifo 阈值中断和超时屏蔽开关这里用不上关闭即可。
    bflb_uart_rxint_mask(fpm383c_uart, false);
    // 注册中断入口函数
    bflb_irq_attach(fpm383c_uart->irq_num, uart_isr, NULL);
    //  使能 GPIO 中断
    bflb_irq_enable(fpm383c_uart->irq_num);
}

/// @brief USART串口发送数据
/// @param length 发送数组长度
/// @param FPM383C_DataBuf 需要发送的功能数组
void FPM383C_SendData(int length, uint8_t FPM383C_DataBuf[])
{
    LOG_I("FPM383C_DataBuf[0]:%02x\r\n", FPM383C_DataBuf[0]);
    for (int i = 0; i < length; i++)
    {
        bflb_uart_put(fpm383c_uart, (uint8_t *)&FPM383C_DataBuf[i], 1);
    }
    USART2_STA = 0;
}

/// @brief 发送休眠指令 确认码=00H 表示休眠设置成功。确认码=01H 表示休眠设置失败。
/// @param
void FPM383C_Sleep(void)
{
    FPM383C_SendData(12, PS_SleepBuf);
}

/// @brief 验证用获取图像
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_GetImage(uint32_t timeout)
{
    uint8_t tmp;
    FPM383C_SendData(12, PS_GetImageBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    tmp = (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
    memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
    return tmp;
}

/// @brief 将图像缓冲区中的原始图像生成指纹特征文件存于模板缓冲区。
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_GenChar(uint32_t timeout)
{
    uint8_t tmp;
    FPM383C_SendData(13, PS_GetCharBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    tmp = (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
    memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
    return tmp;
}

/// @brief 发送搜索指纹指令
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_Search(uint32_t timeout)
{
    FPM383C_SendData(17, PS_SearchBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    return (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
}

/// @brief 删除指定指纹指令
/// @param pageID 需要删除的指纹ID号
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_Delete(uint16_t pageID, uint32_t timeout)
{
    uint8_t tmp;
    PS_DeleteBuf[10] = (pageID >> 8);
    PS_DeleteBuf[11] = (pageID);
    PS_DeleteBuf[14] = (0x15 + PS_DeleteBuf[10] + PS_DeleteBuf[11]) >> 8;
    PS_DeleteBuf[15] = (0x15 + PS_DeleteBuf[10] + PS_DeleteBuf[11]);
    FPM383C_SendData(16, PS_DeleteBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    tmp = (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
    memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
    return tmp;
}

/// @brief 清空指纹库
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_Empty(uint32_t timeout)
{
    uint8_t tmp;
    FPM383C_SendData(12, PS_EmptyBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    tmp = (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
    memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
    return tmp;
}

/// @brief 发送控制灯光指令
/// @param PS_ControlLEDBuf 不同颜色的协议数据
/// @param timeout 接收数据的超时时间
/// @return 确认码
uint8_t FPM383C_ControlLED(uint8_t PS_ControlLEDBuf[], uint32_t timeout)
{
    uint8_t tmp;
    FPM383C_SendData(16, PS_ControlLEDBuf);
    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    tmp = (USART_ReceiveBuf[6] == 0x07 ? USART_ReceiveBuf[9] : 0xFF);
    memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
    return tmp;
}

/// @brief 验证指纹是否注册
/// @param
void FPM383C_Identify(void)
{
    if (FPM383C_GetImage(2000) == 0x00)
    {
        if (FPM383C_GenChar(2000) == 0x00)
        {
            struct bflb_device_s *led = bflb_device_get_by_name("gpio");
            if (FPM383C_Search(2000) == 0x00)
            {
                score = (int)((USART_ReceiveBuf[10] << 8) + USART_ReceiveBuf[11]);
                fingerID_Unlock = score;
                LOG_E("识别成功 指纹ID：%d\r\n", fingerID_Unlock);

                FPM383C_ControlLED(PS_GreenLEDBuf, 1000);

                // 控制板上led灯
                // bflb_gpio_init(led, GPIO_PIN_14, GPIO_OUTPUT);
                // bflb_gpio_set(led, GPIO_PIN_14);
                // bflb_mtimer_delay_ms(1000);
                // bflb_gpio_reset(led, GPIO_PIN_14);

                // 重置接收数据缓存
                memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
                return;
            }
            else
            {
                LOG_E("识别失败\r\n");

                FPM383C_ControlLED(PS_RedLEDBuf, 1000);

                // bflb_gpio_init(led, GPIO_PIN_12, GPIO_OUTPUT);
                // bflb_gpio_set(led, GPIO_PIN_12);
                // bflb_mtimer_delay_ms(1000);
                // bflb_gpio_reset(led, GPIO_PIN_12);

                // 重置接收数据缓存
                memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
                return;
            }
        }
    }
}

/// @brief 自动注册
/// @param pageID 输入需要注册的指纹ID号，取值范围0—59
/// @param timeout 设置注册指纹超时时间，因为需要按压四次手指，建议大于10000（即10s）
void FPM383C_Enroll(uint16_t pageID, uint16_t timeout)
{
    char fingerID_END_ptr[5];
    char fingerID_END_ptr_R[5];
    memset(fingerID_END_ptr, 0, 5);
    memset(fingerID_END_ptr_R, 0, 5);


    LOG_E("注册指纹ID: %d\r\n", pageID);
    // uint8_t PS_AutoEnrollBuf[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x31,'\0','\0',0x04,0x00,0x16,'\0','\0'};
    PS_AutoEnrollBuf[10] = (pageID >> 8);
    PS_AutoEnrollBuf[11] = (pageID);
    PS_AutoEnrollBuf[15] = (0x54 + PS_AutoEnrollBuf[10] + PS_AutoEnrollBuf[11]) >> 8;
    PS_AutoEnrollBuf[16] = (0x54 + PS_AutoEnrollBuf[10] + PS_AutoEnrollBuf[11]);
    FPM383C_SendData(17, PS_AutoEnrollBuf);



    while (!USART2_STA && (--timeout))
    {
        bflb_mtimer_delay_ms(1);
    }
    if (USART_ReceiveBuf[9] == 0x00)
    {
        LOG_E("指纹注册完成\r\n");
        // 亮绿灯2秒
        FPM383C_ControlLED(PS_GreenLEDBuf, 2000);
        // 重置接收数据缓存
        memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));


        // ------------------------
        // flash_erase_set
        // pageID

        // intToStr
        for (int i = 0; i < 5; i++)
        {
            if (pageID == 0)
            {
                if (i == 0)
                {
                    fingerID_END_ptr[i] = '0';
                }
                for (int j = 0; j < i; j++)
                {
                    fingerID_END_ptr[j] = fingerID_END_ptr_R[i - 1 - j];
                }

                break;
            }
            fingerID_END_ptr_R[i] = (pageID % 10) + 48;
            pageID /= 10;
        }

        LOG_D("fingerID_END_ptr:%d,fingerID_END_ptr_R:%d\r\n", fingerID_END_ptr, fingerID_END_ptr_R);
        flash_erase_set(fingerID_END_STR, fingerID_END_ptr);

        return;
    }
    else if (timeout == 0)
    {
        // 超时取消注册
        FPM383C_SendData(12, PS_CancelBuf);
        bflb_mtimer_delay_ms(50);
        // 重置接收数据缓存
        memset(USART_ReceiveBuf, 0xFF, sizeof(USART_ReceiveBuf));
        // 亮红灯2秒
        FPM383C_ControlLED(PS_RedLEDBuf, 2000);
    }
}













#ifdef CONFIG_SHELL
#include <shell.h>


void register_fingerPrint(void)
{
    TOActionFingerprint = 1;
}

void delete_fingerPrint(void)
{
    TOActionFingerprint = 2;
}

void empty_fingerPrint(void)
{
    FPM383C_Empty(2000);
}

SHELL_CMD_EXPORT_ALIAS(register_fingerPrint,register_fingerPrint,toRegisterAfingerPrint);
SHELL_CMD_EXPORT_ALIAS(delete_fingerPrint,delete_fingerPrint,toDeleteAfingerPrint);

SHELL_CMD_EXPORT_ALIAS(empty_fingerPrint,empty_fingerPrint,toEmptyAllfingerPrint);

#endif
