#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/iio/iio.h>


/*
 * BMP280 calibration coefficients
 */
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


/*
 * Driver private data
 */
struct bmp280_data {
    struct bmp280_calib calib;

    /*
     * Fine temperature value.
     *
     * Pressure compensation depends on this.
     */
    s32 t_fine;

    /*
     * IIO device associated with this sensor.
     */
    struct iio_dev *indio_dev;

    /*
     * I2C client used to communicate with BMP280.
     */
    struct i2c_client *client;
};


/*
 * ---------------------------------------------------------
 * Temperature compensation
 * ---------------------------------------------------------
 *
 * Input:
 *     adc_T = raw 20-bit temperature ADC value
 *
 * Output:
 *     temperature in hundredths of a degree Celsius
 *
 * Example:
 *
 *     3003 = 30.03 °C
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

    /*
     * t_fine is required by pressure compensation.
     */
    data->t_fine = var1 + var2;

    temperature = (data->t_fine * 5 + 128) >> 8;

    return temperature;
}


/*
 * ---------------------------------------------------------
 * Pressure compensation
 * ---------------------------------------------------------
 *
 * Input:
 *     adc_P = raw 20-bit pressure ADC value
 *
 * Output:
 *     pressure in Pascals
 */
static s32 bmp280_compensate_pressure(struct bmp280_data *data,
                                      s32 adc_P)
{
    s64 var1;
    s64 var2;
    s64 pressure;

    var1 = ((s64)data->t_fine) - 128000;

    var2 = var1 * var1 * data->calib.dig_P6;

    var2 = var2 +
           ((var1 * data->calib.dig_P5) << 17);

    var2 = var2 +
           (((s64)data->calib.dig_P4) << 35);

    var1 = ((var1 * var1 * data->calib.dig_P3) >> 8) +
           ((var1 * data->calib.dig_P2) << 12);

    var1 = (((((s64)1) << 47) + var1) *
            data->calib.dig_P1) >> 33;

    /*
     * Prevent division by zero.
     */
    if (var1 == 0)
        return 0;

    pressure = 1048576 - adc_P;

    pressure = (((pressure << 31) - var2) * 3125) / var1;

    var1 = ((s64)data->calib.dig_P9 *
            (pressure >> 13) *
            (pressure >> 13)) >> 25;

    var2 = ((s64)data->calib.dig_P8 *
            pressure) >> 19;

    pressure = ((pressure + var1 + var2) >> 8) +
               ((s64)data->calib.dig_P7 << 4);

    /*
     * Compensation result is Q24.8.
     *
     * Shift right by 8 to obtain Pascals.
     */
    return pressure >> 8;
}


/*
 * ---------------------------------------------------------
 * Read and compensate one BMP280 measurement
 * ---------------------------------------------------------
 *
 * Reads:
 *
 *     0xF7 pressure MSB
 *     0xF8 pressure LSB
 *     0xF9 pressure XLSB
 *     0xFA temperature MSB
 *     0xFB temperature LSB
 *     0xFC temperature XLSB
 *
 * Returns:
 *
 *     temperature = hundredths of °C
 *     pressure    = Pascals
 */
static int bmp280_read_measurement(struct bmp280_data *data,
                                   s32 *temperature,
                                   s32 *pressure)
{
    struct i2c_client *client = data->client;

    u8 raw_data[6];

    s32 adc_T;
    s32 adc_P;

    int ret;


    /*
     * Read pressure and temperature registers.
     */
    ret = i2c_smbus_read_i2c_block_data(client,
                                        0xF7,
                                        6,
                                        raw_data);

    if (ret < 0) {
        pr_err("bmp280: failed to read measurement data\n");
        return ret;
    }

    /*
     * Make sure all 6 bytes were received.
     */
    if (ret != 6) {
        pr_err("bmp280: incomplete measurement read\n");
        return -EIO;
    }


    /*
     * Construct 20-bit pressure ADC value.
     *
     *       MSB       LSB       XLSB
     *
     *       8 bits    8 bits    4 bits
     *
     *       [19:12]   [11:4]    [3:0]
     */
    adc_P = ((s32)raw_data[0] << 12) |
            ((s32)raw_data[1] << 4) |
            ((s32)raw_data[2] >> 4);


    /*
     * Construct 20-bit temperature ADC value.
     */
    adc_T = ((s32)raw_data[3] << 12) |
            ((s32)raw_data[4] << 4) |
            ((s32)raw_data[5] >> 4);


    /*
     * Temperature must be compensated first.
     *
     * This calculates t_fine.
     */
    *temperature =
        bmp280_compensate_temperature(data, adc_T);


    /*
     * Pressure compensation uses t_fine.
     */
    *pressure =
        bmp280_compensate_pressure(data, adc_P);


    return 0;
}


/*
 * ---------------------------------------------------------
 * IIO channels
 * ---------------------------------------------------------
 */
static const struct iio_chan_spec bmp280_channels[] = {
    {
        .type = IIO_TEMP,
        .info_mask_separate =
            BIT(IIO_CHAN_INFO_PROCESSED),
    },

    {
        .type = IIO_PRESSURE,
        .info_mask_separate =
            BIT(IIO_CHAN_INFO_PROCESSED),
    },
};


/*
 * ---------------------------------------------------------
 * IIO read_raw()
 * ---------------------------------------------------------
 *
 * This function is called when userspace reads:
 *
 *     /sys/bus/iio/devices/iio:deviceX/in_temp_input
 *
 * or:
 *
 *     /sys/bus/iio/devices/iio:deviceX/in_pressure_input
 */
static int bmp280_read_raw(struct iio_dev *indio_dev,
                           struct iio_chan_spec const *chan,
                           int *val,
                           int *val2,
                           long mask)
{
    struct bmp280_data *data = iio_priv(indio_dev);

    s32 temperature;
    s32 pressure;

    int ret;


    switch (mask) {

    case IIO_CHAN_INFO_PROCESSED:

        /*
         * Read sensor and perform compensation.
         */
        ret = bmp280_read_measurement(data,
                                      &temperature,
                                      &pressure);

        if (ret)
            return ret;


        /*
         * Temperature
         *
         * BMP280 gives us:
         *
         *     hundredths of °C
         *
         * Example:
         *
         *     2734 = 27.34 °C
         *
         * IIO expects:
         *
         *     millidegrees Celsius
         *
         * Therefore:
         *
         *     2734 × 10 = 27340
         */
        if (chan->type == IIO_TEMP) {

            *val = temperature * 10;

            return IIO_VAL_INT;
        }


        /*
         * Pressure
         *
         * BMP280 compensation gives Pascals.
         *
         * IIO processed pressure is represented
         * as kPa + micro.
         *
         * Example:
         *
         *     101325 Pa
         *
         * becomes:
         *
         *     101 kPa + 325000 micro
         */
        if (chan->type == IIO_PRESSURE) {

            *val = pressure / 1000;
            *val2 = (pressure % 1000) * 1000;

            return IIO_VAL_INT_PLUS_MICRO;
        }


        return -EINVAL;


    default:
        return -EINVAL;
    }
}


/*
 * ---------------------------------------------------------
 * IIO information structure
 * ---------------------------------------------------------
 */
static const struct iio_info bmp280_iio_info = {
    .read_raw = bmp280_read_raw,
};


/*
 * ---------------------------------------------------------
 * Probe
 * ---------------------------------------------------------
 */
static int bmp280_probe(struct i2c_client *client)
{
    struct iio_dev *indio_dev;
    struct bmp280_data *data;

    u8 calib_data[24];

    int chip_id;
    int ret;


    /*
     * Allocate IIO device and private driver data.
     *
     * sizeof(*data) creates the private area used by:
     *
     *     iio_priv(indio_dev)
     */
    indio_dev = devm_iio_device_alloc(&client->dev,
                                      sizeof(*data));

    if (!indio_dev)
        return -ENOMEM;


    /*
     * Get our private data area.
     */
    data = iio_priv(indio_dev);


    /*
     * Save references.
     */
    data->indio_dev = indio_dev;
    data->client = client;


    pr_info("bmp280: probe called\n");

    pr_info("bmp280: I2C address = 0x%02x\n",
            client->addr);


    /*
     * -----------------------------------------------------
     * Read chip ID
     * -----------------------------------------------------
     *
     * BMP280 chip ID register:
     *
     *     0xD0
     *
     * Expected value:
     *
     *     0x58
     */
    chip_id = i2c_smbus_read_byte_data(client, 0xD0);

    if (chip_id < 0) {

        pr_err("bmp280: failed to read chip ID\n");

        return chip_id;
    }


    pr_info("bmp280: chip ID = 0x%02x\n",
            chip_id);


    if (chip_id != 0x58) {

        pr_err("bmp280: unexpected chip ID\n");

        return -ENODEV;
    }


    pr_info("bmp280: BMP280 detected!\n");


    /*
     * -----------------------------------------------------
     * Read calibration data
     * -----------------------------------------------------
     *
     * Calibration starts at:
     *
     *     0x88
     *
     * Total:
     *
     *     24 bytes
     */
    ret = i2c_smbus_read_i2c_block_data(client,
                                        0x88,
                                        24,
                                        calib_data);

    if (ret < 0) {

        pr_err("bmp280: failed to read calibration data\n");

        return ret;
    }


    if (ret != 24) {

        pr_err("bmp280: incomplete calibration read\n");

        return -EIO;
    }


    /*
     * -----------------------------------------------------
     * Decode temperature calibration
     * -----------------------------------------------------
     */

    data->calib.dig_T1 =
        (u16)(calib_data[0] |
              ((u16)calib_data[1] << 8));


    data->calib.dig_T2 =
        (s16)(calib_data[2] |
              ((u16)calib_data[3] << 8));


    data->calib.dig_T3 =
        (s16)(calib_data[4] |
              ((u16)calib_data[5] << 8));


    /*
     * -----------------------------------------------------
     * Decode pressure calibration
     * -----------------------------------------------------
     */

    data->calib.dig_P1 =
        (u16)(calib_data[6] |
              ((u16)calib_data[7] << 8));


    data->calib.dig_P2 =
        (s16)(calib_data[8] |
              ((u16)calib_data[9] << 8));


    data->calib.dig_P3 =
        (s16)(calib_data[10] |
              ((u16)calib_data[11] << 8));


    data->calib.dig_P4 =
        (s16)(calib_data[12] |
              ((u16)calib_data[13] << 8));


    data->calib.dig_P5 =
        (s16)(calib_data[14] |
              ((u16)calib_data[15] << 8));


    data->calib.dig_P6 =
        (s16)(calib_data[16] |
              ((u16)calib_data[17] << 8));


    data->calib.dig_P7 =
        (s16)(calib_data[18] |
              ((u16)calib_data[19] << 8));


    data->calib.dig_P8 =
        (s16)(calib_data[20] |
              ((u16)calib_data[21] << 8));


    data->calib.dig_P9 =
        (s16)(calib_data[22] |
              ((u16)calib_data[23] << 8));


    /*
     * Print calibration values for debugging.
     */
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
     * -----------------------------------------------------
     * Configure BMP280
     * -----------------------------------------------------
     *
     * CTRL_MEAS = 0xF4
     *
     * 0x27:
     *
     * Temperature oversampling = x1
     * Pressure oversampling    = x1
     * Mode                     = normal
     */
    ret = i2c_smbus_write_byte_data(client,
                                    0xF4,
                                    0x27);

    if (ret < 0) {

        pr_err("bmp280: failed to configure sensor\n");

        return ret;
    }


    pr_info("bmp280: sensor configured\n");


    /*
     * -----------------------------------------------------
     * Configure IIO device
     * -----------------------------------------------------
     */

    indio_dev->name = "bmp280_custom";

    indio_dev->info = &bmp280_iio_info;

    indio_dev->modes = INDIO_DIRECT_MODE;

    indio_dev->channels = bmp280_channels;

    indio_dev->num_channels =
        ARRAY_SIZE(bmp280_channels);


    /*
     * Associate IIO device with I2C client.
     */
    i2c_set_clientdata(client, indio_dev);


    /*
     * Register IIO device.
     */
    ret = devm_iio_device_register(&client->dev,
                                   indio_dev);

    if (ret < 0) {

        pr_err("bmp280: failed to register IIO device\n");

        return ret;
    }


    pr_info("bmp280: IIO device registered\n");


    return 0;
}


/*
 * ---------------------------------------------------------
 * Remove
 * ---------------------------------------------------------
 */
static void bmp280_remove(struct i2c_client *client)
{
    pr_info("bmp280: remove called\n");
}


/*
 * ---------------------------------------------------------
 * Device Tree matching
 * ---------------------------------------------------------
 */
static const struct of_device_id bmp280_of_match[] = {
    {
        .compatible = "alghazali,bmp280",
    },

    { }
};

MODULE_DEVICE_TABLE(of, bmp280_of_match);


/*
 * ---------------------------------------------------------
 * I2C driver
 * ---------------------------------------------------------
 */
static struct i2c_driver bmp280_driver = {
    .driver = {
        .name = "bmp280_custom",
        .of_match_table = bmp280_of_match,
    },

    .probe = bmp280_probe,
    .remove = bmp280_remove,
};


/*
 * Register I2C driver.
 */
module_i2c_driver(bmp280_driver);


/*
 * ---------------------------------------------------------
 * Module information
 * ---------------------------------------------------------
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Al-Ghazali");
MODULE_DESCRIPTION("BMP280 I2C Linux IIO driver");
