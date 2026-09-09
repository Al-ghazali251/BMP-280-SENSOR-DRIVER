#include <linux/module.h>
#include <linux/i2c.h>



static int bmp280_probe(struct i2c_client *client)
{
    int chip_id;

    pr_info("bmp280: probe called\n");
    pr_info("bmp280: I2C address = 0x%02x\n", client->addr);

    chip_id = i2c_smbus_read_byte_data(client, 0xD0);

    if (chip_id < 0) {
        pr_err("bmp280: failed to read chip ID\n");
        return chip_id;
    }

    pr_info("bmp280: chip ID = 0x%02x\n", chip_id);

    if (chip_id != 0x58) {
        pr_err("bmp280: unexpected chip ID\n");
        return -ENODEV;
    }

    pr_info("bmp280: BMP280 detected!\n");

    return 0;
}


static void bmp280_remove(struct i2c_client *client)
{
    pr_info("bmp280: remove called\n");
}

static const struct of_device_id bmp280_of_match[] = {
    {
        .compatible = "alghazali,bmp280",
    },
    { }
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);

static struct i2c_driver bmp280_driver = {
    .driver = {
        .name = "bmp280_custom",
        .of_match_table = bmp280_of_match,
    },

    .probe = bmp280_probe,
    .remove = bmp280_remove,
};

module_i2c_driver(bmp280_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Al-Ghazali");
MODULE_DESCRIPTION("BMP280 I2C Linux kernel driver");
