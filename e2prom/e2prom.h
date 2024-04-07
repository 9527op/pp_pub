// eeprom 
// target: at24c08 4_block 16_page 16-byte_page
// with dma or no
// 
// 

#ifndef E2PROM_P_H
#define E2PROM_P_H

// 16
#define EEPROM_SELECT_PAGE0     (0 << 4)
#define EEPROM_SELECT_PAGE1     (1 << 4)
#define EEPROM_SELECT_PAGE2     (2 << 4)
#define EEPROM_SELECT_PAGE3     (3 << 4)
#define EEPROM_SELECT_PAGE4     (4 << 4)
#define EEPROM_SELECT_PAGE5     (5 << 4)
#define EEPROM_SELECT_PAGE6     (6 << 4)
#define EEPROM_SELECT_PAGE7     (7 << 4)
#define EEPROM_SELECT_PAGE8     (8 << 4)
#define EEPROM_SELECT_PAGE9     (9 << 4)
#define EEPROM_SELECT_PAGE10    (10 << 4)
#define EEPROM_SELECT_PAGE11    (11 << 4)
#define EEPROM_SELECT_PAGE12    (12 << 4)
#define EEPROM_SELECT_PAGE13    (13 << 4)
#define EEPROM_SELECT_PAGE14    (14 << 4)
#define EEPROM_SELECT_PAGE15    (15 << 4)

#define EEPROM_SUBADDR_LEN      1
#define EEPROM_DATA_BUF_LEN     16

#define EEPROM_inPAGE_bit0     0x00    // ?
#define EEPROM_inPAGE_bit1     0x01
#define EEPROM_inPAGE_bit2     0x02
#define EEPROM_inPAGE_bit3     0x03
#define EEPROM_inPAGE_bit4     0x04
#define EEPROM_inPAGE_bit5     0x05
#define EEPROM_inPAGE_bit6     0x06
#define EEPROM_inPAGE_bit7     0x07
#define EEPROM_inPAGE_bit8     0x08
#define EEPROM_inPAGE_bit9     0x09
#define EEPROM_inPAGE_bit10    0x0A
#define EEPROM_inPAGE_bit11    0x0B
#define EEPROM_inPAGE_bit12    0x0C
#define EEPROM_inPAGE_bit13    0x0D
#define EEPROM_inPAGE_bit14    0x0E
#define EEPROM_inPAGE_bit15    0x0F



#define EEPROM_I2C_ADDRESS1  0x50 // 1010 X00X 高七位
#define EEPROM_I2C_ADDRESS2  0x51 // 1010 X01X 高七位
#define EEPROM_I2C_ADDRESS3  0x52 // 1010 X10X 高七位
#define EEPROM_I2C_ADDRESS4  0x53 // 1010 X11X 高七位





void e2prom_read_test(void);


#endif