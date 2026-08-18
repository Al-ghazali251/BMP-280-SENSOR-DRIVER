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

    /* Select the BMP280 on the I2C bus */
    if (ioctl(dev.fd, I2C_SLAVE, dev.address) < 0)
    {
        perror("Failed to select BMP280");
        close(dev.fd);
        return 1;
    }

    /* Read chip ID register */
    if (bmp280_read_reg(&dev, BMP280_REG_ID, &chip_id) < 0)
    {
        perror("Failed to read chip ID");
        close(dev.fd);
        return 1;
    }

    printf("Chip ID: 0x%02X\n", chip_id);

    /* Close I2C device */
    close(dev.fd);

    return 0;
}
