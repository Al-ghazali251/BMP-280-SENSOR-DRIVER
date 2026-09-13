#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/slab.h>

struct bmp280_calib {
    u16 dig_T1;
    s16 dig_T2;
    s16 dig_T3;

    u16 dig_P1;
    s16 dig_P2;
    s16 dig_P3;
    s16 dig_P4;
    s16 dig_P5;
    s16 dig_P6;
    s16 dig_P7;
    s16 dig_P8;
    s16 dig_P9;
};

struct bmp280_data {
    struct bmp280_calib calib;
};

static int bmp280_probe(struct i2c_client *client)
{
    struct bmp280_data *data;
    u8 calib_data[24];
    int chip_id;
    int ret;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);

    if (!data)
        return -ENOMEM;

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

    ret = i2c_smbus_read_i2c_block_data(client, 0x88, 24, calib_data);

    if (ret < 0) {
        pr_err("bmp280: failed to read calibration data\n");
        return ret;
    }

    data->calib.dig_T1 = calib_data[0] | (calib_data[1] << 8);
    data->calib.dig_T2 = calib_data[2] | (calib_data[3] << 8);
    data->calib.dig_T3 = calib_data[4] | (calib_data[5] << 8);

    data->calib.dig_P1 = calib_data[6] | (calib_data[7] << 8);
    data->calib.dig_P2 = calib_data[8] | (calib_data[9] << 8);
    data->calib.dig_P3 = calib_data[10] | (calib_data[11] << 8);
    data->calib.dig_P4 = calib_data[12] | (calib_data[13] << 8);
    data->calib.dig_P5 = calib_data[14] | (calib_data[15] << 8);
    data->calib.dig_P6 = calib_data[16] | (calib_data[17] << 8);
    data->calib.dig_P7 = calib_data[18] | (calib_data[19] << 8);
    data->calib.dig_P8 = calib_data[20] | (calib_data[21] << 8);
    data->calib.dig_P9 = calib_data[22] | (calib_data[23] << 8);

    pr_info("bmp280: T1=%u T2=%d T3=%d\n",
            data->calib.dig_T1,
            data->calib.dig_T2,
            data->calib.dig_T3);

    pr_info("bmp280: P1=%u P2=%d P3=%d P4=%d P5=%d P6=%d\n",
            data->calib.dig_P1,
            data->calib.dig_P2,
            data->calib.dig_P3,
            data->calib.dig_P4,
            data->calib.dig_P5,
            data->calib.dig_P6);

    pr_info("bmp280: P7=%d P8=%d P9=%d\n",
            data->calib.dig_P7,
            data->calib.dig_P8,
            data->calib.dig_P9);

    i2c_set_clientdata(client, data);

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
