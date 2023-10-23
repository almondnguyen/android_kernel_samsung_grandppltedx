/*
 * Copyright (C) 2013-2016 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "tz_iwcbuf.h"
#include "tzdev.h"
#include "tz_cdev.h"

MODULE_AUTHOR("Pavel Bogachev <p.bogachev@partner.samsung.com>");
MODULE_DESCRIPTION("Trustzone transport driver");
MODULE_LICENSE("GPL");

#define TZ_TRANSPORT_DEVICE_NAME "tz_transport"

#define TZDEV_TRANSPORT_BUF_SIZE	(CONFIG_TZ_TRANSPORT_PG_CNT * PAGE_SIZE - sizeof(struct tzio_iw_channel))

static DEFINE_MUTEX(tz_transport_init_mutex);
static DEFINE_SPINLOCK(tz_transport_slock);
static DEFINE_PER_CPU(struct tzio_iw_channel *, tz_transport_channel);

enum tz_transport_state {
	NOT_INITIALIZED,
	INITIALIZED,
	PANICKED,
};

static enum tz_transport_state tz_transport_state = NOT_INITIALIZED;

static ssize_t tz_transport_read_channel(struct tzio_iw_channel *ch,
		char __user *buffer, size_t size)
{
	unsigned int count, write_count, avail;
	size_t total = 0;

	write_count = ch->write_count;

	if (write_count == ch->read_count)
		return 0;

	if (write_count < ch->read_count)
		avail = TZDEV_TRANSPORT_BUF_SIZE - ch->read_count + write_count;
	else
		avail = write_count - ch->read_count;

	if (size < avail)
		return 0;

	if (write_count < ch->read_count) {
		count = TZDEV_TRANSPORT_BUF_SIZE - ch->read_count;

		if (copy_to_user(buffer, ch->buffer + ch->read_count, count))
			return -EFAULT;

		ch->read_count = 0;
		buffer += count;
		total += count;
	}

	count = write_count - ch->read_count;
	if (copy_to_user(buffer, ch->buffer + ch->read_count, count))
		return -EFAULT;

	ch->read_count += count;
	total += count;

	return total;
}

static ssize_t tz_transport_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct tzio_iw_channel *ch;
	unsigned int i;
	ssize_t position = 0;
	ssize_t ret;

	(void) file;
	(void) ppos;

	if (count == 0)
		return 0;

	spin_lock(&tz_transport_slock);

	for (i = 0; i < NR_SW_CPU_IDS; ++i) {
		ch = per_cpu(tz_transport_channel, i);
		if (!ch)
			continue;

		ret = tz_transport_read_channel(ch, buf + position,
				count - position);
		if (ret < 0) {
			spin_unlock(&tz_transport_slock);
			return ret;
		}

		position += ret;
	}

	spin_unlock(&tz_transport_slock);

	return position;
}

static int tz_transport_alloc_channels(void)
{
	unsigned int i;

	for (i = 0; i < NR_SW_CPU_IDS; ++i) {
		struct tzio_iw_channel *channel = tzio_alloc_iw_channel(
				TZDEV_CONNECT_TRANSPORT,
				CONFIG_TZ_TRANSPORT_PG_CNT);
		if (!channel)
			return -ENOMEM;

		per_cpu(tz_transport_channel, i) = channel;
	}

	return 0;
}

static int tz_transport_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	if (!tzdev_is_initialized())
		return -EPERM;

	mutex_lock(&tz_transport_init_mutex);
	if (tz_transport_state == PANICKED) {
		ret = -ESHUTDOWN;
		goto out;
	}

	if (tz_transport_state == INITIALIZED)
		goto out;

	ret = tz_transport_alloc_channels();
	if (ret)
		tz_transport_state = PANICKED;
	else
		tz_transport_state = INITIALIZED;

out:
	mutex_unlock(&tz_transport_init_mutex);

	return ret;
}

static const struct file_operations tz_transport_fops = {
	.owner = THIS_MODULE,
	.open = tz_transport_open,
	.read = tz_transport_read,
};

static struct tz_cdev tz_transport_cdev = {
	.name = TZ_TRANSPORT_DEVICE_NAME,
	.fops = &tz_transport_fops,
	.owner = THIS_MODULE,
};

static int __init tz_transport_init(void)
{
	int rc;

	rc = tz_cdev_register(&tz_transport_cdev);

	return rc;
}

static void __exit tz_transport_exit(void)
{
	tz_cdev_unregister(&tz_transport_cdev);
}

module_init(tz_transport_init);
module_exit(tz_transport_exit);
