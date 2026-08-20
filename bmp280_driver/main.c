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

    /* Open the I2C bus */
    dev.fd = open(BMP280_I2C_BUS, O_RDWR);

    if (dev.fd < 0)
    {
        perror("Failed to open I2C");
        return 1;
    }

    /* Set the BMP280 I2C address */
    dev.address = BMP280_I2C_ADDR;

    /* Select the BMP280 */
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

    /* Display calibration values */
    printf("dig_T1 = %u\n", dev.calib.dig_T1);
    printf("dig_T2 = %d\n", dev.calib.dig_T2);
    printf("dig_T3 = %d\n", dev.calib.dig_T3);

    printf("dig_P1 = %u\n", dev.calib.dig_P1);
    printf("dig_P2 = %d\n", dev.calib.dig_P2);
    printf("dig_P3 = %d\n", dev.calib.dig_P3);
    printf("dig_P4 = %d\n", dev.calib.dig_P4);
    printf("dig_P5 = %d\n", dev.calib.dig_P5);
    printf("dig_P6 = %d\n", dev.calib.dig_P6);
    printf("dig_P7 = %d\n", dev.calib.dig_P7);
    printf("dig_P8 = %d\n", dev.calib.dig_P8);
    printf("dig_P9 = %d\n", dev.calib.dig_P9);

    /* Close I2C device */
    close(dev.fd);

    return 0;
}
