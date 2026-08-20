#include "bmp280.h"
#include <stdio.h>
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
    /* Send the starting register address */
    if (write(dev->fd, &reg, 1) != 1)
        return -1;

    /* Read multiple consecutive bytes */
    if (read(dev->fd, buffer, length) != length)
        return -1;

    return 0;
}


int bmp280_read_calibration(struct bmp280 *dev)
{
    uint8_t calib_data[24];

    /* Read calibration registers 0x88 to 0x9F */
    if (bmp280_read_regs(dev, 0x88, calib_data, 24) < 0){
        return -1;
	printf("Calibration bytes:\n");}

for (int i = 0; i < 24; i++)
{
    printf("%02X ", calib_data[i]);

    if ((i + 1) % 8 == 0)
        printf("\n");
}

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


int bmp280_write_reg(struct bmp280 *dev,
                     uint8_t reg,
                     uint8_t value)
{
    uint8_t data[2];

    /* First byte = register address */
    data[0] = reg;

    /* Second byte = value to write */
    data[1] = value;

    /* Send register + value */
    if (write(dev->fd, data, 2) != 2)
        return -1;

    return 0;
}


int bmp280_configure(struct bmp280 *dev)
{
    uint8_t config = 0;

    /* Temperature oversampling x1 */
    config |= (1 << 5);

    /* Pressure oversampling x1 */
    config |= (1 << 2);

    /* Normal mode */
    config |= 3;

    /* Write configuration to CTRL_MEAS register (0xF4) */
    if (bmp280_write_reg(dev,
                         BMP280_REG_CTRL_MEAS,
                         config) < 0)
        return -1;

    return 0;
}



int bmp280_configure_filter(struct bmp280 *dev)
{
    uint8_t config = 0;

    /* Standby time = 0.5 ms */
    config |= (0 << 5);

    /* IIR filter = off */
    config |= (0 << 2);

    /* SPI 3-wire disabled */
    config |= 0;

    if (bmp280_write_reg(dev,
                         BMP280_REG_CONFIG,
                         config) < 0)
        return -1;

    return 0;
}



int bmp280_read_measurements(struct bmp280 *dev,
                             uint32_t *raw_pressure,
                             uint32_t *raw_temperature)
{
    uint8_t data[6];

    /* Read pressure and temperature registers: 0xF7 to 0xFC */
    if (bmp280_read_regs(dev,
                         BMP280_REG_PRESS_MSB,
                         data,
                         6) < 0)
        return -1;
printf("Measurement bytes: ");

for (int i = 0; i < 6; i++)
{
    printf("%02X ", data[i]);
}

printf("\n");



    /* Combine pressure bytes */
    *raw_pressure =
        ((uint32_t)data[0] << 12) |
        ((uint32_t)data[1] << 4) |
        ((uint32_t)data[2] >> 4);

    /* Combine temperature bytes */
    *raw_temperature =
        ((uint32_t)data[3] << 12) |
        ((uint32_t)data[4] << 4) |
        ((uint32_t)data[5] >> 4);

    return 0;
}

int bmp280_compensate_temperature(struct bmp280 *dev,
                                  uint32_t raw_temperature,
                                  int32_t *temperature)
{
    int32_t var1;
    int32_t var2;
    int32_t t_fine;

    var1 = ((((raw_temperature >> 3) -
              ((int32_t)dev->calib.dig_T1 << 1))) *
            ((int32_t)dev->calib.dig_T2)) >> 11;

	int32_t x;

x = (raw_temperature >> 4) -
    (int32_t)dev->calib.dig_T1;

var2 = (((x * x) >> 12) *
        (int32_t)dev->calib.dig_T3) >> 14;



    t_fine = var1 + var2;

    *temperature = (t_fine * 5 + 128) >> 8;
   printf("var1 = %d\n", var1);
printf("var2 = %d\n", var2);
printf("t_fine = %d\n", t_fine);
    dev->t_fine = t_fine;

    return 0;
}


int bmp280_compensate_pressure(struct bmp280 *dev,
                               uint32_t raw_pressure,
                               uint32_t *pressure)
{
    int64_t var1;
    int64_t var2;
    int64_t p;

    var1 = (int64_t)dev->t_fine - 128000;

    var2 = var1 * var1 *
           (int64_t)dev->calib.dig_P6;

    var2 = var2 +
           ((var1 * (int64_t)dev->calib.dig_P5) << 17);

    var2 = var2 +
           ((int64_t)dev->calib.dig_P4 << 35);

    var1 = ((var1 * var1 *
             (int64_t)dev->calib.dig_P3) >> 8) +
           ((var1 * (int64_t)dev->calib.dig_P2) << 12);

    var1 = (((((int64_t)1 << 47) + var1) *
             (int64_t)dev->calib.dig_P1) >> 33);

    if (var1 == 0)
        return -1;

    p = 1048576 - raw_pressure;

    p = (((p << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)dev->calib.dig_P9 *
            (p >> 13) *
            (p >> 13)) >> 25;

    var2 = ((int64_t)dev->calib.dig_P8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) +
        ((int64_t)dev->calib.dig_P7 << 4);

    *pressure = (uint32_t)p;

    return 0;
}


