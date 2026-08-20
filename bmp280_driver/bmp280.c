#include "bmp280.h"

#include <unistd.h>

int bmp280_read_reg(struct bmp280 *dev,
                    uint8_t reg,
                    uint8_t *value)
{
    /* Send the register address we want to read */
    if (write(dev->fd, &reg, 1) != 1)
        return -1;

    /* Read one byte from that register */
    if (read(dev->fd, value, 1) != 1)
        return -1;

    return 0;
}

int bmp280_read_regs(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t *buffer,
                     uint8_t length)
{
    if (write(dev->fd, &reg, 1) != 1)
        return -1;

    if (read(dev->fd, buffer, length) != length)
        return -1;

    return 0;
}


int bmp280_read_calibration(struct bmp280 *dev)
{
    uint8_t calib_data[24];

    /* Read calibration registers 0x88 to 0x9F */
    if (bmp280_read_regs(dev, 0x88, calib_data, 24) < 0)
        return -1;

    /* Temperature calibration values */
    dev->calib.dig_T1 =
        (uint16_t)calib_data[1] << 8 |
        calib_data[0];

    dev->calib.dig_T2 =
        (int16_t)((uint16_t)calib_data[3] << 8 |
                  calib_data[2]);

    dev->calib.dig_T3 =
        (int16_t)((uint16_t)calib_data[5] << 8 |
                  calib_data[4]);

    /* Pressure calibration values */
    dev->calib.dig_P1 =
        (uint16_t)calib_data[7] << 8 |
        calib_data[6];

    dev->calib.dig_P2 =
        (int16_t)((uint16_t)calib_data[9] << 8 |
                  calib_data[8]);

    dev->calib.dig_P3 =
        (int16_t)((uint16_t)calib_data[11] << 8 |
                  calib_data[10]);

    dev->calib.dig_P4 =
        (int16_t)((uint16_t)calib_data[13] << 8 |
                  calib_data[12]);

    dev->calib.dig_P5 =
        (int16_t)((uint16_t)calib_data[15] << 8 |
                  calib_data[14]);

    dev->calib.dig_P6 =
        (int16_t)((uint16_t)calib_data[17] << 8 |
                  calib_data[16]);

    dev->calib.dig_P7 =
        (int16_t)((uint16_t)calib_data[19] << 8 |
                  calib_data[18]);

    dev->calib.dig_P8 =
        (int16_t)((uint16_t)calib_data[21] << 8 |
                  calib_data[20]);

    dev->calib.dig_P9 =
        (int16_t)((uint16_t)calib_data[23] << 8 |
                  calib_data[22]);

    return 0;
}
