
#include "bflb_adc.h"
#include "bflb_mtimer.h"
#include "board.h"


#define DBG_TAG "ADC"
#include "log.h"


// ---------------------------------------------------

extern int32_t STORG_adc0Val;
extern uint8_t STORG_adc0Cha;

// ---------------------------------------------------








static int32_t justify_millivolt_lightness=2000;
static int32_t justify_millivolt_temperature=2000;
static int32_t justify_millivolt_flame=2000;

struct bflb_device_s *adc;


// 现在只开放adc_ch0
// adc_ch0 io20
#define CHANNELS_COUNT 1
struct bflb_adc_channel_s chan[] = {
    { .pos_chan = ADC_CHANNEL_0,
      .neg_chan = ADC_CHANNEL_GND }
};


void config_adc(void)
{
    adc = bflb_device_get_by_name("adc");

    /* adc clock = XCLK / 2 / 32 */
    struct bflb_adc_config_s cfg;
    cfg.clk_div = ADC_CLK_DIV_32;
    cfg.scan_conv_mode = true;
    cfg.continuous_conv_mode = false;
    cfg.differential_mode = false;
    cfg.resolution = ADC_RESOLUTION_16B;
    cfg.vref = ADC_VREF_3P2V;

    bflb_adc_init(adc, &cfg);
    bflb_adc_channel_config(adc, chan, CHANNELS_COUNT);
}


void start_adc(void)
{

    //get all channels data;
    bflb_adc_start_conversion(adc);
    // 
    while (bflb_adc_get_count(adc) < CHANNELS_COUNT) {
        bflb_mtimer_delay_ms(1);
    }
    
    for (size_t j = 0; j < CHANNELS_COUNT; j++) {
        struct bflb_adc_result_s result;
        uint32_t raw_data = bflb_adc_read_raw(adc);
        bflb_adc_parse_result(adc, &raw_data, &result, 1);
        
        // LOG_I("raw data:%08x\r\n", raw_data);
        LOG_I("pos chan %d,%d mv \r\n", result.pos_chan, result.millivolt);
        // LOG_W("one channel is read over\r\n");
        

        STORG_adc0Cha = result.pos_chan;
        STORG_adc0Val = result.millivolt;
    }
    // 
    bflb_adc_stop_conversion(adc);
}