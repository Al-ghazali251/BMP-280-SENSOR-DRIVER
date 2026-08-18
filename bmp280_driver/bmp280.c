#include "bmp280.h"

#include <unistd.h>

int bmp280_read_reg(struct bmp280 *dev,
                    uint8_t reg,
                    uint8_t *value)
{
    /* Send the register address */
    if (write(dev->fd, &reg, 1) != 1)
        return -1;

    /* Read one byte from the register */
    if (read(dev->fd, value, 1) != 1)
        return -1;

    return 0;
}
