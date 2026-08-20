#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "bmp280.h"

int main(void)
{
    struct bmp280 dev;
    uint8_t chip_id;
    uint8_t ctrl_meas;
    uint8_t config;

    /* Open the I2C bus */
    dev.fd = open(BMP280_I2C_BUS, O_RDWR);

    if (dev.fd < 0)
    {
        perror("Failed to open I2C");
        return 1;
    }

    /* Set BMP280 address */
    dev.address = BMP280_I2C_ADDR;

    /* Select BMP280 */
    if (ioctl(dev.fd, I2C_SLAVE, dev.address) < 0)
    {
        perror("Failed to select BMP280");
        close(dev.fd);
        return 1;
    }

    /* Read chip ID */
    if (bmp280_read_reg(&dev, BMP280_REG_ID, &chip_id) < 0)
    {
        perror("Failed to read chip ID");
        close(dev.fd);
        return 1;
    }

    printf("Chip ID: 0x%02X\n", chip_id);

    /* Read factory calibration data */
    if (bmp280_read_calibration(&dev) < 0)
    {
        perror("Failed to read calibration data");
        close(dev.fd);
        return 1;
    }

    /* Configure CTRL_MEAS */
    if (bmp280_configure(&dev) < 0)
    {
        perror("Failed to configure BMP280");
        close(dev.fd);
        return 1;
    }

    /* Verify CTRL_MEAS */
    if (bmp280_read_reg(&dev,
                        BMP280_REG_CTRL_MEAS,
                        &ctrl_meas) < 0)
    {
        perror("Failed to read CTRL_MEAS");
        close(dev.fd);
        return 1;
    }

    printf("CTRL_MEAS: 0x%02X\n", ctrl_meas);

    /* Configure CONFIG register */
    if (bmp280_configure_filter(&dev) < 0)
    {
        perror("Failed to configure BMP280 filter");
        close(dev.fd);
        return 1;
    }

    /* Verify CONFIG */
    if (bmp280_read_reg(&dev,
                        BMP280_REG_CONFIG,
                        &config) < 0)
    {
        perror("Failed to read CONFIG");
        close(dev.fd);
        return 1;
    }

    printf("CONFIG: 0x%02X\n", config);

uint32_t raw_pressure;
uint32_t raw_temperature;

if (bmp280_read_measurements(&dev,
                             &raw_pressure,
                             &raw_temperature) < 0)
{
    perror("Failed to read measurements");
    close(dev.fd);
    return 1;
}

printf("Raw pressure: %u\n", raw_pressure);
printf("Raw temperature: %u\n", raw_temperature);


int32_t temperature;

if (bmp280_compensate_temperature(&dev,
                                  raw_temperature,
                                  &temperature) < 0)
{
    perror("Failed to compensate temperature");
    close(dev.fd);
    return 1;
}

printf("Temperature: %d.%02d C\n",
       temperature / 100,
       temperature % 100);


uint32_t pressure;

if (bmp280_compensate_pressure(&dev,
                               raw_pressure,
                               &pressure) < 0)
{
    perror("Failed to compensate pressure");
    close(dev.fd);
    return 1;
}

printf("Pressure: %u Pa\n", pressure);
printf("Pressure: %u.%02u hPa\n",
       pressure / 100,
       pressure % 100);



    /* Close I2C device */
    close(dev.fd);

    return 0;
}
