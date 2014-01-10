/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/input/synaptics_dsx.h>
#include "synaptics_dsx_core.h"

#define SYN_I2C_RETRY_TIMES 10

#define REPORT_ID_GET_BLOB 0x07
#define REPORT_ID_WRITE 0x09
#define REPORT_ID_READ_ADDRESS 0x0a
#define REPORT_ID_READ_DATA 0x0b
#define REPORT_ID_SET_RMI_MODE 0x0f

#define FEATURE_REPORT_TYPE 0x03

#define BLOB_REPORT_SIZE (256 + 3)

#define POWER_COMMAND 0x08
#define RESET_COMMAND 0x01

#define FINGER_MODE 0x00
#define RMI_MODE 0x02

struct hid_device_descriptor {
	unsigned short device_descriptor_length;
	unsigned short format_version;
	unsigned short report_descriptor_length;
	unsigned short report_descriptor_index;
	unsigned short input_register_index;
	unsigned short input_report_max_length;
	unsigned short output_register_index;
	unsigned short output_report_max_length;
	unsigned short command_register_index;
	unsigned short data_register_index;
	unsigned short vendor_id;
	unsigned short product_id;
	unsigned short version_id;
	unsigned int reserved;
};

static struct hid_device_descriptor hid_dd;

struct i2c_rw_buffer {
	unsigned char *read;
	unsigned char *write;
	unsigned short read_size;
	unsigned short write_size;
};

static struct i2c_rw_buffer buffer;

static int do_i2c_transfer(struct i2c_client *client, struct i2c_msg *msg)
{
	unsigned char retry;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		dev_err(&client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&client->dev,
				"%s: I2C transfer over retry limit\n",
				__func__);
		return -EIO;
	}

	return 0;
}

static int check_buffer(unsigned char **buffer, unsigned short *buffer_size,
		unsigned short length)
{
	if (*buffer_size < length) {
		if (*buffer_size)
			kfree(*buffer);
		*buffer = kzalloc(length, GFP_KERNEL);
		if (!(*buffer))
			return -ENOMEM;
		*buffer_size = length;
	}

	return 0;
}

static int generic_read(struct i2c_client *client, unsigned short length)
{
	int retval;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
		}
	};

	check_buffer(&buffer.read, &buffer.read_size, length);
	msg[0].buf = buffer.read;

	retval = do_i2c_transfer(client, msg);
	if (retval == 0)
		retval = length;

	return retval;
}

static int generic_write(struct i2c_client *client, unsigned short length)
{
	int retval;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = buffer.write,
		}
	};

	retval = do_i2c_transfer(client, msg);
	if (retval == 0)
		retval = length;

	return retval;
}

static void switch_to_rmi(struct synaptics_rmi4_data *rmi4_data)
{
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);

	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	check_buffer(&buffer.write, &buffer.write_size, 11);

	/* set rmi mode */
	buffer.write[0] = hid_dd.command_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.command_register_index >> 8;
	buffer.write[2] = (FEATURE_REPORT_TYPE << 4) | REPORT_ID_SET_RMI_MODE;
	buffer.write[3] = 0x03;
	buffer.write[4] = REPORT_ID_SET_RMI_MODE;
	buffer.write[5] = hid_dd.data_register_index & MASK_8BIT;
	buffer.write[6] = hid_dd.data_register_index >> 8;
	buffer.write[7] = 0x04;
	buffer.write[8] = 0x00;
	buffer.write[9] = REPORT_ID_SET_RMI_MODE;
	buffer.write[10] = RMI_MODE;

	generic_write(i2c, 11);

	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	return;
}

static void hid_i2c_init(struct synaptics_rmi4_data *rmi4_data)
{
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);
	const struct synaptics_dsx_board_data *bdata =
			rmi4_data->hw_if->board_data;

	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	check_buffer(&buffer.write, &buffer.write_size, 6);

	/* read device descriptor */
	buffer.write[0] = bdata->device_descriptor_addr & MASK_8BIT;
	buffer.write[1] = bdata->device_descriptor_addr >> 8;
	generic_write(i2c, 2);
	generic_read(i2c, sizeof(hid_dd));
	memcpy((unsigned char *)&hid_dd, buffer.read, sizeof(hid_dd));

	/* set power */
	buffer.write[0] = hid_dd.command_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.command_register_index >> 8;
	buffer.write[2] = 0x00;
	buffer.write[3] = POWER_COMMAND;
	generic_write(i2c, 4);

	/* reset */
	buffer.write[0] = hid_dd.command_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.command_register_index >> 8;
	buffer.write[2] = 0x00;
	buffer.write[3] = RESET_COMMAND;
	generic_write(i2c, 4);

	while (gpio_get_value(bdata->irq_gpio))
		msleep(20);

	generic_read(i2c, hid_dd.input_report_max_length);

	/* get blob */
	buffer.write[0] = hid_dd.command_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.command_register_index >> 8;
	buffer.write[2] = (FEATURE_REPORT_TYPE << 4) | REPORT_ID_GET_BLOB;
	buffer.write[3] = 0x02;
	buffer.write[4] = hid_dd.data_register_index & MASK_8BIT;
	buffer.write[5] = hid_dd.data_register_index >> 8;

	generic_write(i2c, 6);
	msleep(20);
	generic_read(i2c, BLOB_REPORT_SIZE);

	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	switch_to_rmi(rmi4_data);

	return;
}

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char recover = 1;
	unsigned short report_length;
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.len = hid_dd.output_report_max_length + 2,
		},
		{
			.addr = i2c->addr,
			.flags = I2C_M_RD,
			.len = length + 4,
		},
	};

recover:
	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	check_buffer(&buffer.write, &buffer.write_size,
			hid_dd.output_report_max_length + 2);
	msg[0].buf = buffer.write;
	buffer.write[0] = hid_dd.output_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.output_register_index >> 8;
	buffer.write[2] = hid_dd.output_report_max_length & MASK_8BIT;
	buffer.write[3] = hid_dd.output_report_max_length >> 8;
	buffer.write[4] = REPORT_ID_READ_ADDRESS;
	buffer.write[5] = 0x00;
	buffer.write[6] = addr & MASK_8BIT;
	buffer.write[7] = addr >> 8;
	buffer.write[8] = length & MASK_8BIT;
	buffer.write[9] = length >> 8;

	check_buffer(&buffer.read, &buffer.read_size, length + 4);
	msg[1].buf = buffer.read;

	retval = do_i2c_transfer(i2c, &msg[0]);
	if (retval != 0)
		goto exit;

	retry = 0;
	do {
		retval = do_i2c_transfer(i2c, &msg[1]);
		if (retval == 0)
			retval = length;
		else
			goto exit;

		report_length = (buffer.read[1] << 8) | buffer.read[0];
		if (report_length == hid_dd.input_report_max_length) {
			memcpy(&data[0], &buffer.read[4], length);
			goto exit;
		}

		msleep(20);
		retry++;
	} while (retry < SYN_I2C_RETRY_TIMES);

	dev_err(rmi4_data->pdev->dev.parent,
			"%s: Failed to receive read report\n",
			__func__);
	retval = -EIO;

exit:
	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	if ((retval != length) && (recover == 1)) {
		recover = 0;
		hid_i2c_init(rmi4_data);
		goto recover;
	}

	return retval;
}

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char recover = 1;
	unsigned char msg_length;
	struct i2c_client *i2c = to_i2c_client(rmi4_data->pdev->dev.parent);
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
		}
	};

	if ((length + 10) < (hid_dd.output_report_max_length + 2))
		msg_length = hid_dd.output_report_max_length + 2;
	else
		msg_length = length + 10;

recover:
	mutex_lock(&rmi4_data->rmi4_io_ctrl_mutex);

	check_buffer(&buffer.write, &buffer.write_size, msg_length);
	msg[0].len = msg_length;
	msg[0].buf = buffer.write;
	buffer.write[0] = hid_dd.output_register_index & MASK_8BIT;
	buffer.write[1] = hid_dd.output_register_index >> 8;
	buffer.write[2] = hid_dd.output_report_max_length & MASK_8BIT;
	buffer.write[3] = hid_dd.output_report_max_length >> 8;
	buffer.write[4] = REPORT_ID_WRITE;
	buffer.write[5] = 0x00;
	buffer.write[6] = addr & MASK_8BIT;
	buffer.write[7] = addr >> 8;
	buffer.write[8] = length & MASK_8BIT;
	buffer.write[9] = length >> 8;
	memcpy(&buffer.write[10], &data[0], length);

	retval = do_i2c_transfer(i2c, msg);
	if (retval == 0)
		retval = length;

	mutex_unlock(&rmi4_data->rmi4_io_ctrl_mutex);

	if ((retval != length) && (recover == 1)) {
		recover = 0;
		hid_i2c_init(rmi4_data);
		goto recover;
	}

	return retval;
}

static struct synaptics_dsx_bus_access bus_access = {
	.type = BUS_I2C,
	.read = synaptics_rmi4_i2c_read,
	.write = synaptics_rmi4_i2c_write,
};

static struct synaptics_dsx_hw_interface hw_if;

static struct platform_device *synaptics_dsx_i2c_device;

static void synaptics_rmi4_i2c_dev_release(struct device *dev)
{
	kfree(synaptics_dsx_i2c_device);

	return;
}

static int synaptics_rmi4_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
				"%s: SMBus byte data commands not supported by host\n",
				__func__);
		return -EIO;
	}

	synaptics_dsx_i2c_device = kzalloc(
			sizeof(struct platform_device),
			GFP_KERNEL);
	if (!synaptics_dsx_i2c_device) {
		dev_err(&client->dev,
				"%s: Failed to allocate memory for synaptics_dsx_i2c_device\n",
				__func__);
		return -ENOMEM;
	}

	hw_if.board_data = client->dev.platform_data;
	hw_if.bus_access = &bus_access;
	hw_if.bl_hw_init = switch_to_rmi;
	hw_if.ui_hw_init = hid_i2c_init;

	synaptics_dsx_i2c_device->name = PLATFORM_DRIVER_NAME;
	synaptics_dsx_i2c_device->id = 0;
	synaptics_dsx_i2c_device->num_resources = 0;
	synaptics_dsx_i2c_device->dev.parent = &client->dev;
	synaptics_dsx_i2c_device->dev.platform_data = &hw_if;
	synaptics_dsx_i2c_device->dev.release = synaptics_rmi4_i2c_dev_release;

	retval = platform_device_register(synaptics_dsx_i2c_device);
	if (retval) {
		dev_err(&client->dev,
				"%s: Failed to register platform device\n",
				__func__);
		return -ENODEV;
	}

	return 0;
}

static int synaptics_rmi4_i2c_remove(struct i2c_client *client)
{
	if (buffer.read_size)
		kfree(buffer.read);

	if (buffer.write_size)
		kfree(buffer.write);

	platform_device_unregister(synaptics_dsx_i2c_device);

	return 0;
}

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{I2C_DRIVER_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);

static struct i2c_driver synaptics_rmi4_i2c_driver = {
	.driver = {
		.name = I2C_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = synaptics_rmi4_i2c_probe,
	.remove = __devexit_p(synaptics_rmi4_i2c_remove),
	.id_table = synaptics_rmi4_id_table,
};

int synaptics_rmi4_bus_init(void)
{
	return i2c_add_driver(&synaptics_rmi4_i2c_driver);
}
EXPORT_SYMBOL(synaptics_rmi4_bus_init);

void synaptics_rmi4_bus_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_i2c_driver);

	return;
}
EXPORT_SYMBOL(synaptics_rmi4_bus_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics DSX I2C Bus Support Module");
MODULE_LICENSE("GPL v2");
