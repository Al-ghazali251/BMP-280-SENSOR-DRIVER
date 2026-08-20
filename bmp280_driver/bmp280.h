#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#define BMP280_I2C_BUS  "/dev/i2c-1"
#define BMP280_I2C_ADDR 0x76
#define BMP280_CALIB_START 0x88
#define BMP280_CALIB_LENGTH 24
#define BMP280_REG_ID   0xD0
#define BMP280_CHIP_ID  0x58
#define BMP280_REG_CTRL_MEAS 0xF4

struct bmp280_calib {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
};



struct bmp280 {
    int fd;
    uint8_t address;
    struct bmp280_calib calib;
};

int bmp280_read_reg(struct bmp280 *dev,
                    uint8_t reg,
                    uint8_t *value);


int bmp280_read_regs(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t *buffer,
                     uint8_t length);






int bmp280_read_calibration(struct bmp280 *dev);

int bmp280_write_reg(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t value);





int bmp280_configure(struct bmp280 *dev);


#endif
