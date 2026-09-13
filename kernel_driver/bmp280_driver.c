#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>

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
    s32 t_fine;
};


/*
 * Compensate raw temperature ADC value
 *
 * Returns temperature in hundredths of a degree Celsius.
 *
 * Example:
 * 3003 = 30.03 °C
 */
static s32 bmp280_compensate_temperature(struct bmp280_data *data,
                                         s32 adc_T)
{
    s32 var1;
    s32 var2;
    s32 temperature;

    var1 = ((((adc_T >> 3) -
              ((s32)data->calib.dig_T1 << 1))) *
            data->calib.dig_T2) >> 11;

    var2 = (((((adc_T >> 4) -
               (s32)data->calib.dig_T1) *
              ((adc_T >> 4) -
               (s32)data->calib.dig_T1)) >> 12) *
            data->calib.dig_T3) >> 14;

    data->t_fine = var1 + var2;

    temperature = (data->t_fine * 5 + 128) >> 8;

    return temperature;
}


/*
 * Compensate raw pressure ADC value
 *
 * Returns pressure in Pascals.
 */
static s32 bmp280_compensate_pressure(struct bmp280_data *data,
                                      s32 adc_P)
{
    s64 var1;
    s64 var2;
    s64 pressure;

    var1 = ((s64)data->t_fine) - 128000;

    var2 = var1 * var1 * data->calib.dig_P6;
    var2 = var2 + ((var1 * data->calib.dig_P5) << 17);
    var2 = var2 + (((s64)data->calib.dig_P4) << 35);

    var1 = ((var1 * var1 * data->calib.dig_P3) >> 8) +
           ((var1 * data->calib.dig_P2) << 12);

    var1 = (((((s64)1) << 47) + var1) *
            data->calib.dig_P1) >> 33;

    if (var1 == 0)
        return 0;

    pressure = 1048576 - adc_P;

    pressure = (((pressure << 31) - var2) * 3125) / var1;

    var1 = ((s64)data->calib.dig_P9 *
            (pressure >> 13) *
            (pressure >> 13)) >> 25;

    var2 = ((s64)data->calib.dig_P8 * pressure) >> 19;

    pressure = ((pressure + var1 + var2) >> 8) +
               ((s64)data->calib.dig_P7 << 4);

    /*
     * BMP280 pressure result is in Q24.8 format.
     * Shift right by 8 to obtain whole Pascals.
     */
    return pressure >> 8;
}


static int bmp280_read_measurement(struct i2c_client *client,
                                   struct bmp280_data *data)
{
    u8 raw_data[6];
    s32 adc_P;
    s32 adc_T;
    s32 temperature;
    s32 pressure;
    int ret;

    /*
     * BMP280 measurement registers:
     *
     * 0xF7 - pressure MSB
     * 0xF8 - pressure LSB
     * 0xF9 - pressure XLSB
     * 0xFA - temperature MSB
     * 0xFB - temperature LSB
     * 0xFC - temperature XLSB
     */
    ret = i2c_smbus_read_i2c_block_data(client, 0xF7, 6, raw_data);

    if (ret < 0) {
        pr_err("bmp280: failed to read measurement data\n");
        return ret;
    }

    /*
     * Pressure is a 20-bit value.
     */
    adc_P = ((s32)raw_data[0] << 12) |
            ((s32)raw_data[1] << 4) |
            ((s32)raw_data[2] >> 4);

    /*
     * Temperature is a 20-bit value.
     */
    adc_T = ((s32)raw_data[3] << 12) |
            ((s32)raw_data[4] << 4) |
            ((s32)raw_data[5] >> 4);

    pr_info("bmp280: raw temperature = %d\n", adc_T);
    pr_info("bmp280: raw pressure = %d\n", adc_P);

    /*
     * Temperature must be compensated first because
     * pressure compensation uses t_fine.
     */
    temperature = bmp280_compensate_temperature(data, adc_T);

    pressure = bmp280_compensate_pressure(data, adc_P);

    pr_info("bmp280: temperature = %d.%02d C\n",
            temperature / 100,
            abs(temperature % 100));

    pr_info("bmp280: pressure = %d Pa\n", pressure);

    return 0;
}


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

    /*
     * Read chip ID from register 0xD0.
     */
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

    /*
     * Read the 24-byte calibration block.
     *
     * Calibration starts at 0x88.
     */
    ret = i2c_smbus_read_i2c_block_data(client,
                                        0x88,
                                        24,
                                        calib_data);

    if (ret < 0) {
        pr_err("bmp280: failed to read calibration data\n");
        return ret;
    }

    /*
     * Decode temperature calibration.
     */
    data->calib.dig_T1 =
        calib_data[0] |
        (calib_data[1] << 8);

    data->calib.dig_T2 =
        calib_data[2] |
        (calib_data[3] << 8);

    data->calib.dig_T3 =
        calib_data[4] |
        (calib_data[5] << 8);

    /*
     * Decode pressure calibration.
     */
    data->calib.dig_P1 =
        calib_data[6] |
        (calib_data[7] << 8);

    data->calib.dig_P2 =
        calib_data[8] |
        (calib_data[9] << 8);

    data->calib.dig_P3 =
        calib_data[10] |
        (calib_data[11] << 8);

    data->calib.dig_P4 =
        calib_data[12] |
        (calib_data[13] << 8);

    data->calib.dig_P5 =
        calib_data[14] |
        (calib_data[15] << 8);

    data->calib.dig_P6 =
        calib_data[16] |
        (calib_data[17] << 8);

    data->calib.dig_P7 =
        calib_data[18] |
        (calib_data[19] << 8);

    data->calib.dig_P8 =
        calib_data[20] |
        (calib_data[21] << 8);

    data->calib.dig_P9 =
        calib_data[22] |
        (calib_data[23] << 8);

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

    /*
     * CTRL_MEAS = 0x27
     *
     * Temperature oversampling x1
     * Pressure oversampling x1
     * Normal measurement mode
     */
    ret = i2c_smbus_write_byte_data(client, 0xF4, 0x27);

    if (ret < 0) {
        pr_err("bmp280: failed to configure sensor\n");
        return ret;
    }

    pr_info("bmp280: sensor configured\n");

    i2c_set_clientdata(client, data);

    /*
     * Give the BMP280 enough time to complete
     * its first measurement.
     */
    msleep(10);

    ret = bmp280_read_measurement(client, data);

    if (ret < 0)
        return ret;

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
