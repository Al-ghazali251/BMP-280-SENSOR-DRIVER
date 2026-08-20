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
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_MEAS_LENGTH   6



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
    int32_t t_fine;
    struct bmp280_calib calib;
};

int bmp280_read_reg(struct bmp280 *dev,
                    uint8_t reg,
                    uint8_t *value);


int bmp280_compensate_pressure(struct bmp280 *dev,
                               uint32_t raw_pressure,
                               uint32_t *pressure);



int bmp280_read_regs(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t *buffer,
                     uint8_t length);



int bmp280_read_measurements(struct bmp280 *dev,
                             uint32_t *raw_pressure,
                             uint32_t *raw_temperature);





int bmp280_read_calibration(struct bmp280 *dev);


int bmp280_read_measurements(struct bmp280 *dev,
                             uint32_t *raw_pressure,
                             uint32_t *raw_temperature);


int bmp280_write_reg(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t value);





int bmp280_configure(struct bmp280 *dev);

int bmp280_configure_filter(struct bmp280 *dev);

int bmp280_compensate_temperature(struct bmp280 *dev,
                                  uint32_t raw_temperature,
                                  int32_t *temperature);


#endif
