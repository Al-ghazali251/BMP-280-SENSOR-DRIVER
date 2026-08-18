#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

#define BMP280_I2C_BUS  "/dev/i2c-1"
#define BMP280_I2C_ADDR 0x76
#define BMP280_CALIB_START 0x88
#define BMP280_CALIB_LENGTH 24
#define BMP280_REG_ID   0xD0
#define BMP280_CHIP_ID  0x58

struct bmp280 {
    int fd;
    uint8_t address;
};

int bmp280_read_reg(struct bmp280 *dev,
                    uint8_t reg,
                    uint8_t *value);

#endif
