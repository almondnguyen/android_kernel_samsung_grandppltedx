/*
 * leds-s2mu005.c - LED class driver for S2MU005 LEDs.
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mfd/samsung/s2mu005.h>
#include <linux/mfd/samsung/s2mu005-private.h>
#include <linux/leds-s2mu005.h>
#include <linux/platform_device.h>
#include <linux/battery/sec_battery.h>

#define BUILD_S2MU005	// For Removing Build Error
#define S2MU005_LED_OFF		-1
#ifdef CONFIG_LEDS_S2MU005_FLASH
#include "leds.h"
#endif

#define FLED_PINCTRL_STATE_DEFAULT "fled_default"
#define FLED_PINCTRL_STATE_SLEEP "fled_sleep"

#ifndef CONFIG_LEDS_S2MU005_FLASH
extern struct class *camera_class;
#else
struct class *camera_class;
#endif

static struct device *rear_dev;
static struct device *front_dev;
static struct device *flash_dev;
bool assistive_light = false;
struct s2mu005_led_data * g_led_datas[S2MU005_LED_MAX];
//struct sec_battery_info *battery;
int ta_attached = 0;
int sdp_attached = 0;

bool probe_flag = false;

static u8 leds_cur_max[] = {
	S2MU005_FLASH_OUT_I_1200MA,
	S2MU005_TORCH_OUT_I_400MA,
};

static u8 leds_time_max[] = {
	S2MU005_FLASH_TIMEOUT_992MS,
	S2MU005_TORCH_TIMEOUT_15728MS,
};

//static struct i2c_client *s2mu005_led_client = NULL;

struct s2mu005_led_data {
	struct led_classdev cdev;
	struct s2mu005_led *data;
	struct notifier_block batt_nb;
	struct i2c_client *i2c;
	struct work_struct work;
	struct mutex lock;
	spinlock_t value_lock;
	int brightness;
	int test_brightness;
	int attach_ta;
	int attach_sdp;
	bool enable;
	int torch_pin;
	int flash_pin;
	unsigned int flash_brightness;
	unsigned int preflash_brightness;
	unsigned int movie_brightness;
	unsigned int rear_torch_brightness;
	unsigned int torch_brightness;
	unsigned int factory_brightness;
};

u8 CH_FLASH_TORCH_EN = S2MU005_REG_FLED_RSVD;

enum {
	FLED_ENABLE_NULL = 0,
	FLED_ENABLE_FRONT,
	FLED_ENABLE_REAR,
};

static void s2mu005_led_enable_ctrl(int mode);
void ss_rear_flash_led_flash_on(void);
void ss_rear_flash_led_torch_on(void);
void ss_rear_flash_led_turn_off(void);
void ss_rear_torch_set_flashlight(int torch_mode);
void ss_front_flash_led_turn_on(void);
void ss_front_flash_led_turn_off(void);
int s2mu005_led_mode_ctrl(int enable_mode, int state);

#ifdef CONFIG_MUIC_NOTIFIER
static void attach_cable_check(muic_attached_dev_t attached_dev,
					int *attach_ta, int *attach_sdp)
{
	if (attached_dev == ATTACHED_DEV_USB_MUIC)
		*attach_sdp = 1;
	else
		*attach_sdp = 0;

	switch (attached_dev) {
	case ATTACHED_DEV_TA_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		*attach_ta = 1;
		break;
	default:
		*attach_ta = 0;
		break;
	}
}

static int ta_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
	u8 temp;

	int ret = 0;
	struct s2mu005_led_data *led_data =
		container_of(nb, struct s2mu005_led_data, batt_nb);

	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		if (!led_data->attach_ta)
			goto err;

		led_data->attach_ta = 0;

		if (!led_data->data->id) {
			pr_info("%s : flash mode\n", __func__);
			goto err;
		}
#ifndef CONFIG_S2MU005_LEDS_I2C
		if (gpio_is_valid(led_data->torch_pin)) {
			ret = devm_gpio_request(led_data->cdev.dev,
					led_data->torch_pin, "s2mu005_gpio");
			if (ret) {
				pr_err("%s : fail to assignment gpio\n",
								__func__);
				goto gpio_free_data;
			}
		}
		if (gpio_get_value(led_data->torch_pin)) {
			gpio_direction_output(led_data->torch_pin, 0);
			gpio_direction_output(led_data->torch_pin, 1);
			goto gpio_free_data;
		}
#else
		s2mu005_read_reg(led_data->i2c,
				CH_FLASH_TORCH_EN, &temp);
		if ((temp & S2MU005_TORCH_ON_I2C) == S2MU005_TORCH_ON_I2C) {
			ret = s2mu005_update_reg(led_data->i2c,
				CH_FLASH_TORCH_EN,
				S2MU005_FLASH_TORCH_OFF,
				S2MU005_TORCH_ENABLE_MASK);

			pr_info("%s : LED OFF\n", __func__);
			if (ret < 0)
				goto err;
			ret = s2mu005_update_reg(led_data->i2c,
				CH_FLASH_TORCH_EN,	//S2MU005_REG_LED_CTRL4,  jtt for compile error 11.16
				S2MU005_TORCH_ON_I2C,
				S2MU005_TORCH_ENABLE_MASK);

			pr_info("%s : LED ON\n", __func__);
			if (ret < 0)
				goto err;
		}
#endif
		/* CHGIN_ENGH = 0 */
		ret = s2mu005_update_reg(led_data->i2c,
			S2MU005_REG_FLED_CTRL1, 0x00, 0x80);
		if (ret < 0)
			goto err;

		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		led_data->attach_ta = 0;
		attach_cable_check(attached_dev, &led_data->attach_ta,
						&led_data->attach_sdp);
		if (led_data->attach_ta) {
			s2mu005_read_reg(led_data->i2c,
				S2MU005_REG_FLED_STATUS, &temp);

			/* if CH1_TORCH_ON or CH2_TORCH_ON setting CHGIN_ENGH bit 1 */
			if (temp & 0x50) {
				 ret = s2mu005_update_reg(led_data->i2c,
					S2MU005_REG_FLED_CTRL1, 0x80, 0x80);
				if (ret < 0)
					goto err;
			}
		}

		return 0;
	default:
		goto err;
		break;
	}

#ifndef CONFIG_S2MU005_LEDS_I2C
gpio_free_data:
	gpio_free(led_data->torch_pin);
	pr_info("%s : gpio free\n", __func__);
#endif
	pr_info("%s : complete detached\n", __func__);
	return 0;
err:
	pr_err("%s : abandond access %d\n", __func__, led_data->attach_ta);
	return 0;
}
#endif

static void torch_led_on_off(int value)
{
	int ret;
	if (value && g_led_datas[S2MU005_FLASH_LED]->attach_ta) { //torch on & ta attach
		ret = s2mu005_update_reg(g_led_datas[S2MU005_FLASH_LED]->i2c,
			S2MU005_REG_FLED_CTRL1, 0x80, 0x80);
		if (ret < 0)
			pr_err("%s : CHGIN_ENGH = 1 fail\n", __func__);
	}

	if (value == 0) { // torch off
		ret = s2mu005_update_reg(g_led_datas[S2MU005_FLASH_LED]->i2c,
			S2MU005_REG_FLED_CTRL1, 0x00, 0x80);
		if (ret < 0)
			pr_err("%s : CHGIN_ENGH = 0 fail\n", __func__);
	}
	
	pr_info("%s : val = %d, attach_ta = %d\n", __func__, value, g_led_datas[S2MU005_FLASH_LED]->attach_ta );
}

static void led_set(struct s2mu005_led_data *led_data)
{
	int ret;
	struct s2mu005_led *data = led_data->data;
	int id = data->id;
	u8 mask = 0, reg = 0, value;

#ifdef CONFIG_S2MU005_LEDS_I2C
	u8 enable_mask;
#else
	int gpio_pin;
#endif

#ifdef CONFIG_S2MU005_LEDS_I2C
	value =	S2MU005_FLASH_TORCH_OFF;
#else
	if (id == S2MU005_FLASH_LED) {
		value = S2MU005_FLASH_ON_GPIO | S2MU005_TORCH_ON_GPIO;
	} else {
		value = S2MU005_FRONT_FLASH_ON_GPIO | S2MU005_FRONT_TORCH_ON_GPIO;
	}
#endif
	mask = 0xFF;	//S2MU005_TORCH_ENABLE_MASK | S2MU005_FLASH_ENABLE_MASK;
	ret = s2mu005_update_reg(led_data->i2c, CH_FLASH_TORCH_EN,
		value, mask);

	if (id == S2MU005_FLASH_LED) {
		pr_info("%s for rear flash sysfs\n", __func__);
		reg = S2MU005_REG_FLED_CH1_CTRL1;
		mask = S2MU005_TORCH_IOUT_MASK;
#ifndef CONFIG_S2MU005_LEDS_I2C
		pr_info("%s gpio_torch mode\n", __func__);
		gpio_pin = led_data->torch_pin;
#endif
	} else {
		pr_info("%s for front flash sysfs\n", __func__);
		reg = S2MU005_REG_FLED_CH2_CTRL1;
		mask = S2MU005_TORCH_IOUT_MASK;
#ifndef CONFIG_S2MU005_LEDS_I2C
		pr_info("%s gpio_torch mode\n", __func__);
		gpio_pin = led_data->torch_pin;
#endif
	}


#ifndef CONFIG_S2MU005_LEDS_I2C
	if (gpio_is_valid(gpio_pin)) {
		ret = devm_gpio_request(led_data->cdev.dev, gpio_pin,
				"s2mu005_gpio");
		if (ret) {
			pr_err("%s : fail to assignment gpio\n", __func__);
			goto gpio_free_data;
		}
	}
#endif
	pr_info("%s start led_set\n", __func__);

	if (led_data->test_brightness == S2MU005_LED_OFF) {
		pr_info("%s: LED off set brightness =%d",__func__,led_data->data->brightness);
		ret = s2mu005_update_reg(led_data->i2c, reg,
				led_data->data->brightness, mask);
		if (ret < 0)
			goto error_set_bits;

#ifdef CONFIG_S2MU005_LEDS_I2C
		value = S2MU005_FLASH_TORCH_OFF;
#else
		gpio_direction_output(gpio_pin, 0);
#endif

#ifndef BUILD_S2MU005
		/* torch mode off sequence */
		if (id && led_data->attach_ta) {
			if (!factory_mode) {
				ret = s2mu005_update_reg(led_data->i2c,
					S2MU005_REG_FLED_CTRL1, 0x00, 0x80);
				if (ret < 0)
					goto error_set_bits;
			}
		}
#endif

#ifndef CONFIG_S2MU005_LEDS_I2C
		goto gpio_free_data;
#endif
	} else {
		pr_info("%s led on\n", __func__);
		/* torch mode on sequence */
		if (id && led_data->attach_ta) {
			ret = s2mu005_update_reg(led_data->i2c,
				S2MU005_REG_FLED_CTRL1, 0x80, 0x80);
			if (ret < 0)
				goto error_set_bits;
			pr_info("%s torch mode setting complete. 0x37[7] = 1\n", __func__);
			/* ta attach & sdp mode :  brightness limit 300mA */
			if (led_data->attach_sdp)
				led_data->test_brightness =
					(led_data->test_brightness > S2MU005_TORCH_OUT_I_300MA) ?
					S2MU005_TORCH_OUT_I_300MA : led_data->test_brightness;
		}
		pr_err("%s led brightness = %d\n", __func__, led_data->test_brightness);
		ret = s2mu005_update_reg(led_data->i2c,
				reg, led_data->test_brightness, mask);
		if (ret < 0)
			goto error_set_bits;

#ifndef CONFIG_S2MU005_LEDS_I2C
		gpio_direction_output(gpio_pin, 1);
		goto gpio_free_data;

#endif
	}
#ifdef CONFIG_S2MU005_LEDS_I2C

	if(led_data->test_brightness != S2MU005_LED_OFF)
		value = (id==S2MU005_FLASH_LED) ? S2MU005_CH1_TORCH_ON_I2C : S2MU005_CH2_TORCH_ON_I2C;
	
	enable_mask = (id==S2MU005_FLASH_LED) ? S2MU005_CH1_TORCH_ENABLE_MASK :
		S2MU005_CH2_TORCH_ENABLE_MASK;
	pr_err("%s: id=%d value = %x enable_mask=%x(CH_FLASH_TORCH_EN=%x)\n", __func__, id,value,enable_mask,CH_FLASH_TORCH_EN);

	ret = s2mu005_update_reg(led_data->i2c,
		CH_FLASH_TORCH_EN,
		value, enable_mask);

	if (ret < 0)
		goto error_set_bits;
#endif
	return;

#ifndef CONFIG_S2MU005_LEDS_I2C
gpio_free_data:
	gpio_free(gpio_pin);
	pr_info("%s : gpio free\n", __func__);
	return;
#endif
error_set_bits:
	pr_err("%s: can't set led level %d\n", __func__, ret);
	return;
}

static void s2mu005_led_set(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	unsigned long flags;
	struct s2mu005_led_data *led_data =
		container_of(led_cdev, struct s2mu005_led_data, cdev);
	u8 max;

	max = led_cdev->max_brightness;

	pr_info("%s value = %d, max = %d\n", __func__, value, max);

	spin_lock_irqsave(&led_data->value_lock, flags);
	led_data->test_brightness = min_t(int, (int)value, (int)max);
	spin_unlock_irqrestore(&led_data->value_lock, flags);

//	schedule_work(&led_data->work);
	led_set(led_data);
	return;
}

/***JTT  need check what this function is for 11.13**/
static void s2mu005_led_work(struct work_struct *work)
{
	struct s2mu005_led_data *led_data
		= container_of(work, struct s2mu005_led_data, work);

	pr_debug("%s [led]\n", __func__);

	mutex_lock(&led_data->lock);
	led_set(led_data);
	mutex_unlock(&led_data->lock);
}

static int s2mu005_led_setup(struct s2mu005_led_data *led_data)
{
	int ret = 0;
	int mask, value;
	u8 temp;

	pr_err("%s : led setup start\n", __func__);
	
	ret = s2mu005_read_reg(led_data->i2c, 0x73, &temp);	/* EVT0 0x73[3:0] == 0x0 */
	if (ret < 0)
		goto out;

	if ((temp & 0xf) == 0x00) {
		/* forced BATID recognition 0x89[1:0] = 0x3 */
		ret = s2mu005_update_reg(led_data->i2c, 0x89, 0x03, 0x03);
		if (ret < 0)
			goto out;
		ret = s2mu005_update_reg(led_data->i2c, 0x92, 0x80, 0x80);
		if (ret < 0)
			goto out;
	}
	pr_err("%s : led setup 1\n", __func__);

	/* Controlled Channel1, Channel2 independently */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CTRL2,
			0x00, S2MU005_EN_CHANNEL_SHARE_MASK);
	if (ret < 0)
		goto out;

	/* Boost vout flash 4.5V */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CTRL2,
			0x0A, S2MU005_BOOST_VOUT_FLASH_MASK);
	if (ret < 0)
		goto out;

	pr_err("%s : led setup 2\n", __func__);

	/* FLED_BOOST_EN */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CTRL1,
			0x40, S2MU005_FLASH_BOOST_EN_MASK);
	if (ret < 0)
		goto out;

	/* Flash timer Maximum mode */
	ret = s2mu005_update_reg(led_data->i2c,
			S2MU005_REG_FLED_CH1_CTRL3, 0x80, 0x80);
	if (ret < 0)
		goto out;

	pr_err("%s : led setup 3\n", __func__);

	if (led_data->data->id == S2MU005_FLASH_LED) {
		/* flash timer Maximum set */
		ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH1_CTRL3,
				led_data->data->timeout, S2MU005_TIMEOUT_MAX);
		if (ret < 0)
			goto out;
	} else {
		/* torch timer Maximum set */ 
	/*	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH1_CTRL2,
				led_data->data->timeout, S2MU005_TIMEOUT_MAX);*/
		ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH2_CTRL2,// jtt set timer for front flash
				led_data->data->timeout, S2MU005_TIMEOUT_MAX);
		if (ret < 0)
			goto out;
	}

	pr_err("%s : led setup 4\n", __func__);

	/* flash brightness set */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH1_CTRL0,
			led_data->flash_brightness, S2MU005_FLASH_IOUT_MASK);
	if (ret < 0)
		goto out;

	/* torch brightness set */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH1_CTRL1,
			led_data->preflash_brightness, S2MU005_TORCH_IOUT_MASK);
	if (ret < 0)
		goto out;

	/*front  torch brightness set */
	ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CH2_CTRL1,
			led_data->torch_brightness, S2MU005_TORCH_IOUT_MASK);
	if (ret < 0)
		goto out;

	pr_err("%s : led setup 5\n", __func__);

	  /* UVLO 2.8V */
  	ret = s2mu005_update_reg(led_data->i2c, 0x9A, 0x10, 0x10);
  	if (ret < 0)
    	goto out;
  

#ifndef BUILD_S2MU005
	/* factory mode additional setting */
	if (factory_mode) {
		ret = s2mu005_update_reg(led_data->i2c, S2MU005_REG_FLED_CTRL1,
						0x80, 0x80);
		pr_err("%s : led setup fac mode1:%d\n", __func__,ret);
		if (ret < 0)
			goto out;
		ret = s2mu005_update_reg(led_data->i2c, 0xAC,0x40, 0x40);
		pr_err("%s : led setup fac mode2:%d\n", __func__,ret);
		if (ret < 0)
			goto out;
	}
#endif	
	pr_err("%s : led setup 6\n", __func__);

#ifdef CONFIG_S2MU005_LEDS_I2C
	value =	S2MU005_FLASH_TORCH_OFF;
#else
	value = S2MU005_FLASH_ON_GPIO | S2MU005_TORCH_ON_GPIO;
#endif
	mask = 0xFF;	//S2MU005_TORCH_ENABLE_MASK | S2MU005_FLASH_ENABLE_MASK;
	ret = s2mu005_update_reg(led_data->i2c, CH_FLASH_TORCH_EN,
		value, mask);
	pr_err("%s : led setup 7:%d\n", __func__,ret);
	if (ret < 0)
		goto out;
	
	pr_info("%s : led setup complete\n", __func__);
	return ret;

out:
	pr_err("%s : led setup fail\n", __func__);
	return ret;
}

void ss_rear_flash_led_flash_on()
{
	int brightness;

	pr_err("%s : sscam-1\n", __func__);
	if(!probe_flag)
		return;
	pr_err("%s : sscam-2\n", __func__);

	struct i2c_client * client;
	client = g_led_datas[S2MU005_FLASH_LED]->i2c;

	brightness =  g_led_datas[S2MU005_FLASH_LED]->flash_brightness;

	s2mu005_update_reg(client, S2MU005_REG_FLED_CH1_CTRL0, brightness, S2MU005_FLASH_IOUT_MASK);

	s2mu005_led_mode_ctrl(FLED_ENABLE_REAR, S2MU005_FLED_MODE_FLASH);

}EXPORT_SYMBOL_GPL(ss_rear_flash_led_flash_on);

 void ss_rear_flash_led_torch_on()
{
	int brightness;
	 pr_err("%s : sscam-1\n", __func__);
	if(!probe_flag)
		 return;
	pr_err("%s : sscam-2\n", __func__);

	struct i2c_client * client;
	client = g_led_datas[S2MU005_FLASH_LED]->i2c;

	//brightness =  g_led_datas[S2MU005_FLASH_LED]->preflash_brightness;

	//s2mu005_update_reg(client, S2MU005_REG_FLED_CH1_CTRL1, brightness, S2MU005_TORCH_IOUT_MASK);

	s2mu005_led_mode_ctrl(FLED_ENABLE_REAR, S2MU005_FLED_MODE_MOVIE);
	
}EXPORT_SYMBOL_GPL(ss_rear_flash_led_torch_on);

 void ss_rear_flash_led_turn_off()
{
	 pr_err("%s : sscam-1\n", __func__);
	if(!probe_flag)
		return;
	 pr_err("%s : sscam-2\n", __func__);

	s2mu005_led_mode_ctrl(FLED_ENABLE_REAR, S2MU005_FLED_MODE_OFF);

}EXPORT_SYMBOL_GPL(ss_rear_flash_led_turn_off);

 void ss_front_flash_led_turn_on()
{
	int brightness;
	 pr_err("%s : sscam-1\n", __func__);
	if(!probe_flag)
		return;
	pr_err("%s : sscam-2\n", __func__);
	  
	struct i2c_client * client;
	client = g_led_datas[S2MU005_FLASH_LED]->i2c;

	brightness = g_led_datas[S2MU005_FLASH_LED]->torch_brightness;
	pr_info("front flash brightness %d \n", brightness);

	s2mu005_update_reg(client, S2MU005_REG_FLED_CH2_CTRL1, brightness, S2MU005_TORCH_IOUT_MASK);
	  
	s2mu005_led_mode_ctrl(FLED_ENABLE_FRONT, S2MU005_FLED_MODE_MOVIE);

}EXPORT_SYMBOL_GPL(ss_front_flash_led_turn_on);

void ss_rear_torch_set_flashlight(int torch_mode)
{
	//CDBG("%s\n", __func__);
	if (assistive_light == false) {
		struct i2c_client * client;
		client = g_led_datas[S2MU005_FLASH_LED]->i2c;
		if(torch_mode == 1){
			pr_info("%s g_led_datas[S2MU005_FLASH_LED]->movie_brightness = %d \n", __func__,g_led_datas[S2MU005_FLASH_LED]->movie_brightness);
			/* flashright brightness set */
			s2mu005_update_reg(client, S2MU005_REG_FLED_CH1_CTRL1,g_led_datas[S2MU005_FLASH_LED]->movie_brightness, S2MU005_TORCH_IOUT_MASK);
		}
		else if(torch_mode == 0){
			/* preflash brightness set */
			pr_info("%s g_led_datas[S2MU005_FLASH_LED]->preflash_brightness = %d \n", __func__,g_led_datas[S2MU005_FLASH_LED]->preflash_brightness);
			s2mu005_update_reg(client, S2MU005_REG_FLED_CH1_CTRL1,g_led_datas[S2MU005_FLASH_LED]->preflash_brightness, S2MU005_TORCH_IOUT_MASK);
		}
		else if(torch_mode == 2)
		{
			/* torch brightness set */
			pr_info("%s g_led_datas[S2MU005_FLASH_LED]->rear_torch_brightness = %d \n", __func__,g_led_datas[S2MU005_FLASH_LED]->rear_torch_brightness);
			s2mu005_update_reg(client, S2MU005_REG_FLED_CH1_CTRL1,g_led_datas[S2MU005_FLASH_LED]->rear_torch_brightness, S2MU005_TORCH_IOUT_MASK);
		}
	}
}EXPORT_SYMBOL_GPL(ss_rear_torch_set_flashlight);

void ss_front_flash_led_turn_off()
{
	if(!probe_flag)
		return;

	s2mu005_led_mode_ctrl(FLED_ENABLE_FRONT, S2MU005_FLED_MODE_OFF);
}EXPORT_SYMBOL_GPL(ss_front_flash_led_turn_off);
 
int s2mu005_led_mode_ctrl(int enable_mode, int state)
{
	struct s2mu005_led_data *led_data = g_led_datas[S2MU005_FLASH_LED];
	int gpio_torch = led_data->torch_pin;
	int gpio_flash = led_data->flash_pin;

	pr_info("%s : state = %d\n", __func__, state);

	if (assistive_light == true) {
		pr_info("%s : assistive_light is enabled \n", __func__);
		return 0;
	}

	if(enable_mode != FLED_ENABLE_NULL) {
		s2mu005_led_enable_ctrl(enable_mode);
	}

	devm_gpio_request(led_data->cdev.dev, gpio_torch,
				"s2mu005_gpio_torch");

	devm_gpio_request(led_data->cdev.dev, gpio_flash,
				"s2mu005_gpio_flash");

	switch(state) {
		case S2MU005_FLED_MODE_OFF:
			gpio_direction_output(gpio_torch, 0);
			gpio_direction_output(gpio_flash, 0);
			torch_led_on_off(0);
			break;
		case S2MU005_FLED_MODE_PREFLASH:
			gpio_direction_output(gpio_torch, 1);
			break;
		case S2MU005_FLED_MODE_FLASH:
			gpio_direction_output(gpio_flash, 1);
			break;
		case S2MU005_FLED_MODE_MOVIE:
			torch_led_on_off(1);
			gpio_direction_output(gpio_torch, 1);
			break;
		default:
			break;
	}

	gpio_free(gpio_torch);
	gpio_free(gpio_flash);

	return 0;
}

static void s2mu005_led_enable_ctrl(int mode)
{
	int value;
	struct i2c_client * client;
	client = g_led_datas[S2MU005_FLASH_LED]->i2c;
	if(mode == FLED_ENABLE_FRONT) {
		value = S2MU005_FRONT_FLASH_ON_GPIO | S2MU005_FRONT_TORCH_ON_GPIO;
		s2mu005_update_reg(client, CH_FLASH_TORCH_EN, value, 0xFF);
	}
	else if (mode == FLED_ENABLE_REAR) {
		value = S2MU005_FLASH_ON_GPIO | S2MU005_TORCH_ON_GPIO;
		s2mu005_update_reg(client, CH_FLASH_TORCH_EN, value, 0xFF);
	}
}

// sysfs functions
#define SYSFS_BEFORE_PV

extern void get_imx219_mfr_info(u8 *data);
extern unsigned int get_imx219_af_macro_cal(void);
extern unsigned int get_imx219_af_pan_cal(void);
#if defined(CONFIG_GET_REAR_MODULE_ID)
extern void get_imx219_moduleid(u8 *data);
#endif

static ssize_t rear_camfw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char fw_version[12]={0};
	get_imx219_mfr_info(fw_version);
	pr_err("%s : fw ver: %s\n", __func__, fw_version);
	return snprintf(buf, 24, "%s %s\n", fw_version, fw_version);
}

static ssize_t rear_camfw_full_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char fw_version[12]={0};
	get_imx219_mfr_info(fw_version);
	pr_err("%s : fw ver: %s\n", __func__, fw_version);
	return snprintf(buf, 36, "%s %s %s\n", fw_version, fw_version, fw_version);
}

static ssize_t rear_checkfw_factory_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *retStr = "OK";
	char fw_version[12]={0};
	char *ref_version = "W08KSIA";
	get_imx219_mfr_info(fw_version);
	if(!strncmp(fw_version, ref_version, strlen(ref_version))) {
#ifdef SYSFS_BEFORE_PV
		retStr = "OK";
#else
		if(fw_version[10]=='M')
			retStr = "OK";
		else
			retStr = "NG_VER";
#endif
	} else if((fw_version[3] != 'K')) { // 'K' MTK, 'P' Sprd, 'Q' Qualcomm, 'L' LSI
		pr_err("%s : ISP = %c (not K for MTK)\n", __func__, fw_version[3]);
		retStr = "ER";
	} else {
		retStr = "NG_VER";
	}

	pr_err("%s : fw ver: %s, check: %s\n", __func__, fw_version, retStr);
	return snprintf(buf, 20, "%s\n", retStr);
}

static ssize_t rear_checkfw_user_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *retStr = "OK";
	char fw_version[12]={0};
	char *ref_version = "W08KSIA";
	get_imx219_mfr_info(fw_version);
	if(!strncmp(fw_version, ref_version, strlen(ref_version))) {
#ifdef SYSFS_BEFORE_PV
		retStr = "OK";
#else
		if(fw_version[10]=='M')
			retStr = "OK";
		else
			retStr = "NG_VER";
#endif
	} else if((fw_version[3] != 'K')) { // 'K' MTK, 'P' Sprd, 'Q' Qualcomm, 'L' LSI
		pr_err("%s : ISP = %c (not K for MTK)\n", __func__, fw_version[3]);
		retStr = "ER";
	} else {
		retStr = "NG_VER";
	}

	pr_err("%s : fw ver: %s, check: %s\n", __func__, fw_version, retStr);
	return snprintf(buf, 20, "%s\n", retStr);
}

static ssize_t rear_camtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// TODO: check its value.
	char *camTypeStr = "ISP\n";
	pr_err("%s : %s\n", __func__, camTypeStr);
	return snprintf(buf, 100, camTypeStr);
}

static ssize_t rear_caminfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// TODO: check its value.
	char *camInfoStr = "ISP=INT;CALMEM=Y;READVER=SYSFS;COREVOLT=N;UPGRADE=N;CC=N;OIS=N;";
	pr_err("%s : %s\n", __func__, camInfoStr);
	return snprintf(buf, 100, camInfoStr);
}

static ssize_t rear_afcal_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int macro_cal = get_imx219_af_macro_cal();
	unsigned int pan_cal = get_imx219_af_pan_cal();	
	pr_err("%s : macro_cal:%u, pan_cal:%u\n", __func__, macro_cal, pan_cal);
	return snprintf(buf, 2+22, "1 %u %u\n", macro_cal, pan_cal);
}

static ssize_t front_camfw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *camfwStr = "S5K5E3YX N\n";
	pr_err("%s\n", __func__, camfwStr);
	return snprintf(buf, 24, camfwStr);
}

static ssize_t front_camfw_full_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *camfwStr = "S5K5E3YX N S5K5E3YX\n";
	pr_err("%s\n", __func__, camfwStr);
	return snprintf(buf, 36, camfwStr);
}

static ssize_t front_camtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// TODO: check its value.
	char *camTypeStr = "SOC\n";
	pr_err("%s : %s\n", __func__, camTypeStr);
	return snprintf(buf, 100, camTypeStr);
}

static ssize_t front_caminfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// TODO: check its value.
	char *camInfoStr = "ISP=INT;CALMEM=N;READVER=SYSFS;COREVOLT=N;UPGRADE=N;CC=N;OIS=N;";
	pr_err("%s : %s\n", __func__, camInfoStr);
	return snprintf(buf, 100, camInfoStr);
}

// TODO: normal flash operation should be blocked if it is not 0
static int rear_flash_status = 0;
static int front_flash_status = 0;

static ssize_t rear_flash_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if 1
	pr_err("%s : %d\n", __func__, rear_flash_status);
	return snprintf(buf, 20, "%d\n", rear_flash_status);
#else // TODO: check and delete
	struct s2mu005_led_data *led_data = g_led_datas[S2MU005_FLASH_LED];
	char *str;

//jtt if needed string should be re-checked
	switch (led_data->data->id) {
	case S2MU005_FLASH_LED:
		str = "FLASH";
		break;
	case S2MU005_TORCH_LED:
		str = "TORCH";
		break;
	default:
		str = "NONE";
		break;
	}

	return snprintf(buf, 20, "%s\n", str);
#endif
}

static ssize_t front_flash_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	pr_err("%s : %d\n", __func__, front_flash_status);
	return snprintf(buf, 20, "%d\n", front_flash_status);
}

static ssize_t rear_flash_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct s2mu005_led_data *led_data = g_led_datas[S2MU005_FLASH_LED];
	struct led_classdev *led_cdev = &led_data->cdev;
	int value = 0;
	int brightness = 0;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}

	//pr_info("[Rear_FLED]%s , value:%d\n", __func__, value);
	pr_err("[Rear_FLED]%s , value:%d\n", __func__, value);// jtt debug
	mutex_lock(&led_data->lock);

	/*if (led_data->data->id == S2MU005_FLASH_LED) {
		pr_info("%s : flash is not controlled by sysfs", __func__);
		goto err;
	}*/

	if (value == 0) {
		/* Turn off Torch */
		brightness = S2MU005_LED_OFF;
		assistive_light = false;
	} else if (value == 1) {
		/* Turn on Torch */
		brightness = led_data->preflash_brightness;
		assistive_light = true;
	} else if (value == 100) {
		/* Factory mode Turn on Torch */
		brightness = led_data->factory_brightness;
		assistive_light = true;
	} else if (value > 1000) {
		/* torch brightness levels from app */
		brightness = led_data->rear_torch_brightness; //level-3 (default level)
		assistive_light = false;
		switch(value)
		{
			case 1001:
				brightness -= 2; //level-1
				break;
			case 1002:
				brightness -= 1; //level-2
				break;
			case 1004:
				//no change
				break;
			case 1006:
				brightness +=1;//level-4
				break;
			case 1009:
				brightness +=4;//level-5
				break;
			default:
				break;
		}
		
		if(brightness <0)//guard against wrong input  
		{
			brightness = 0;
		}
				
	}else {
		pr_err("[FLED]%s , Invalid value:%d\n", __func__, value);
		goto err;
	}

	if (led_cdev->flags & LED_SUSPENDED) {
		pr_err("%s : Fled suspended\n", __func__);
		goto err;
	}

	// TODO: check the spec
	rear_flash_status = value;
	s2mu005_led_set(led_cdev, brightness);

	mutex_unlock(&led_data->lock);
	return size;

err:
	pr_err("%s : Fled abnormal end\n", __func__);

	mutex_unlock(&led_data->lock);
	return size;
}

static ssize_t front_flash_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct s2mu005_led_data *led_data = g_led_datas[S2MU005_TORCH_LED];
	struct led_classdev *led_cdev = &led_data->cdev;
	int value = 0;
	int brightness = 0;

	if ((buf == NULL) || kstrtouint(buf, 10, &value)) {
		return -1;
	}

	//pr_info("[Front_FLED]%s , value:%d\n", __func__, value);
	pr_err("[Front_FLED]%s , value:%d\n", __func__, value);// jtt debug
	mutex_lock(&led_data->lock);

	/*if (led_data->data->id == S2MU005_FLASH_LED) {
		pr_info("%s : flash is not controlled by sysfs", __func__);
		goto err;
	}*/

	if (value == 0) {
		/* Turn off Torch */
		brightness = S2MU005_LED_OFF;
		assistive_light = false;
	} else if (value == 1) {
		/* Turn on Torch */
		brightness = led_data->torch_brightness;//50mA
		assistive_light = true;
	} else if (value == 100) {
		/* Factory mode Turn on Torch */
		brightness = led_data->torch_brightness + 4;// 150mA ; jtt to be set with wanted  front brightness for factory test
	} else {
		pr_err("[FLED]%s , Invalid value:%d\n", __func__, value);
		goto err;
	}

	if (led_cdev->flags & LED_SUSPENDED) {
		pr_err("%s : led suspended\n", __func__);
		goto err;
	}

	// TODO: check the spec
	front_flash_status = value;
	s2mu005_led_set(led_cdev, brightness);

	mutex_unlock(&led_data->lock);
	return size;

err:
	pr_err("%s : Fled abnormal end\n", __func__);

	mutex_unlock(&led_data->lock);
	return size;
}

#if defined(CONFIG_GET_REAR_MODULE_ID)
static ssize_t rear_cam_moduleid_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{	
#define FROM_MODULE_ID_SIZE	10
	char rear_module_id[FROM_MODULE_ID_SIZE + 1] = "\0";
	get_imx219_moduleid(rear_module_id);

	pr_err("%s : rear_module_id %c%c%c%c%c%02X%02X%02X%02X%02X\n",
		__func__,
		rear_module_id[0], rear_module_id[1], rear_module_id[2], rear_module_id[3], rear_module_id[4],
		rear_module_id[5], rear_module_id[6], rear_module_id[7], rear_module_id[8], rear_module_id[9]);

	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
		rear_module_id[0], rear_module_id[1], rear_module_id[2], rear_module_id[3], rear_module_id[4],
		rear_module_id[5], rear_module_id[6], rear_module_id[7], rear_module_id[8], rear_module_id[9]);  
}
#endif

#define S_IRUGO (S_IRUSR|S_IRGRP|S_IROTH)

static DEVICE_ATTR(rear_camfw, S_IRUGO, rear_camfw_show, NULL);
static DEVICE_ATTR(rear_camfw_full, S_IRUGO, rear_camfw_full_show, NULL);
static DEVICE_ATTR(rear_checkfw_factory, S_IRUGO, rear_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear_checkfw_user, S_IRUGO, rear_checkfw_user_show, NULL);
static DEVICE_ATTR(rear_camtype, S_IRUGO, rear_camtype_show, NULL);
static DEVICE_ATTR(rear_caminfo, S_IRUGO, rear_caminfo_show, NULL);
static DEVICE_ATTR(rear_afcal, S_IRUGO, rear_afcal_show, NULL);

static DEVICE_ATTR(front_camfw, S_IRUGO, front_camfw_show, NULL);
static DEVICE_ATTR(front_camfw_full, S_IRUGO, front_camfw_full_show, NULL);
static DEVICE_ATTR(front_camtype, S_IRUGO, front_camtype_show, NULL);
static DEVICE_ATTR(front_caminfo, S_IRUGO, front_caminfo_show, NULL);

//static DEVICE_ATTR(rear_flash, 0644, rear_flash_show, rear_flash_store);
static DEVICE_ATTR(rear_flash, S_IRUGO|S_IWUSR|S_IWGRP, rear_flash_show, rear_flash_store);
static DEVICE_ATTR(front_flash, S_IRUGO|S_IWUSR|S_IWGRP, front_flash_show, front_flash_store);

#if defined(CONFIG_GET_REAR_MODULE_ID)
static DEVICE_ATTR(rear_moduleid, S_IRUGO, rear_cam_moduleid_show, NULL);
#endif

#if defined(CONFIG_OF)
static int s2mu005_led_dt_parse_pdata(struct device *dev,
				struct s2mu005_fled_platform_data *pdata)
{
	struct device_node *led_np, *np, *c_np;
	int ret;
	u32 temp;
	const char *temp_str;
	int index;
	//int i = 0;
	//int flash_type  = 0;

	led_np = dev->parent->of_node; // jtt need to check if led_np is necessary

	if (!led_np) {
		pr_err("<%s> could not find led sub-node led_np\n", __func__);
		return -ENODEV;
	}

	np = of_find_node_by_name(led_np, "s2mu005_fled");	// jtt  name " "  need  to be checked
	if (!np) {
		pr_err("%s : could not find led sub-node np\n", __func__);
		return -EINVAL;
	}
	
//	flash_type = of_get_child_count(led_np);
	
//	for_each_child_of_node(led_np,c_led_np){			// jtt add to make 2 childe node in dtsi for front & rear flash( 0: rear flash,1:front_flash)

		pr_err(" [%s] JTT parse for flash\n",__func__);
		ret = pdata->torch_pin = of_get_named_gpio(np, "s2mu005,torch-gpio", 0);
		if (ret < 0) {
			pr_err("%s : can't get torch-gpio\n", __func__);
			return ret;
		}

		ret = pdata->flash_pin = of_get_named_gpio(np, "s2mu005,flash-gpio", 0);
		if (ret < 0) {
			pr_err("%s : can't get flash-gpio\n", __func__);
			return ret;
		}

		ret = of_property_read_u32(np, "rear_flash_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->flash_brightness = S2MU005_FLASH_BRIGHTNESS(temp);
		dev_info(dev, "flash_current = <%d>, brightness = %x\n", temp, pdata->flash_brightness);

		ret = of_property_read_u32(np, "rear_preflash_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->preflash_brightness = S2MU005_TORCH_BRIGHTNESS(temp);
		dev_info(dev, "preflash_current = <%d>, brightness = %x\n", temp, pdata->preflash_brightness);

		ret = of_property_read_u32(np, "rear_movie_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->movie_brightness = S2MU005_TORCH_BRIGHTNESS(temp);
		dev_info(dev, "movie_current = <%d>, brightness = %x\n", temp, pdata->movie_brightness);

		ret = of_property_read_u32(np, "rear_torch_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->rear_torch_brightness = S2MU005_TORCH_BRIGHTNESS(temp);
		dev_info(dev, "rear_torch_current = <%d>, brightness = %x\n", temp, pdata->rear_torch_brightness);

		ret = of_property_read_u32(np, "front_torch_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->torch_brightness = S2MU005_TORCH_BRIGHTNESS(temp);
		dev_info(dev, "torch_current = <%d>, brightness = %x\n", temp, pdata->torch_brightness);

		ret = of_property_read_u32(np, "factory_current", &temp);
		if (ret < 0)
			goto dt_err;
		pdata->factory_brightness = S2MU005_TORCH_BRIGHTNESS(temp);
		dev_info(dev, "factory_current = <%d>, brightness = %x\n", temp, pdata->factory_brightness);

		pdata->num_leds = of_get_child_count(np);

		for_each_child_of_node(np, c_np) {
			ret = of_property_read_u32(c_np, "id", &temp);
			if (ret < 0)
				goto dt_err;
			if(temp >= S2MU005_LED_MAX)
				goto dt_err;
			index = temp;
			pdata->leds[index].id = temp;

			ret = of_property_read_string(c_np, "ledname", &temp_str);
			if (ret)
				goto dt_err;
			pdata->leds[index].name = temp_str;

		//	temp = index ? pdata->preflash_brightness : pdata->flash_brightness;
			ret = of_property_read_u32(c_np, "brightness", &temp);
			if (ret < 0)
				goto dt_err;

			temp = index?S2MU005_TORCH_BRIGHTNESS(temp):S2MU005_FLASH_BRIGHTNESS(temp);
			if (temp > leds_cur_max[index])
				temp = leds_cur_max[index];
			pdata->leds[index].brightness = temp;

			ret = of_property_read_u32(c_np, "timeout", &temp);
			if (ret)
				goto dt_err;
			if (temp > leds_time_max[index])
				temp = leds_time_max[index];
			pdata->leds[index].timeout = temp;

		}
//		i++;
//	}
	return 0;//flash_type;// jtt when success parse  return flash_type instead of 0
dt_err:
	pr_err("%s failed to get parse dtsi file \n", __func__);
	return ret;
}
#endif /* CONFIG_OF */

int create_rear_sysfs(void)
{
	int err = -ENODEV;

	pr_err("rear_sysfs: create");

	if (IS_ERR_OR_NULL(camera_class)) {
		pr_err("flash_sysfs: error, camera class not exist");
		return -ENODEV;
	}

	rear_dev = device_create(camera_class, NULL, 0, NULL, "rear");
	if (IS_ERR(rear_dev)) {
		pr_err("rear_sysfs: failed to create device(rear)\n");
		return -ENODEV;
	}

	err = device_create_file(rear_dev, &dev_attr_rear_camfw);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_camfw.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_camfw_full);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_camfw_full.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_checkfw_factory);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_checkfw_factory.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_checkfw_user);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_checkfw_user.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_camtype);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_camtype.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_caminfo);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_caminfo.attr.name);
	}

	err = device_create_file(rear_dev, &dev_attr_rear_afcal);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_afcal.attr.name);
	}

#if defined(CONFIG_GET_REAR_MODULE_ID)
	err = device_create_file(rear_dev, &dev_attr_rear_moduleid);
	if (unlikely(err < 0)) {
		pr_err("rear_sysfs: failed to create device file, %s\n",
				dev_attr_rear_moduleid.attr.name);
	}
#endif

	return 0;
}

int create_front_sysfs(void)
{
	int err = -ENODEV;

	pr_err("front_sysfs: create");

	if (IS_ERR_OR_NULL(camera_class)) {
		pr_err("flash_sysfs: error, camera class not exist");
		return -ENODEV;
	}

	front_dev = device_create(camera_class, NULL, 0, NULL, "front");
	if (IS_ERR(front_dev)) {
		pr_err("front_sysfs: failed to create device(front)\n");
		return -ENODEV;
	}

	err = device_create_file(front_dev, &dev_attr_front_camfw);
	if (unlikely(err < 0)) {
		pr_err("front_sysfs: failed to create device file, %s\n",
				dev_attr_front_camfw.attr.name);
	}

	err = device_create_file(front_dev, &dev_attr_front_camfw_full);
	if (unlikely(err < 0)) {
		pr_err("front_sysfs: failed to create device file, %s\n",
				dev_attr_front_camfw_full.attr.name);
	}

	err = device_create_file(front_dev, &dev_attr_front_camtype);
	if (unlikely(err < 0)) {
		pr_err("front_sysfs: failed to create device file, %s\n",
				dev_attr_front_camtype.attr.name);
	}

	err = device_create_file(front_dev, &dev_attr_front_caminfo);
	if (unlikely(err < 0)) {
		pr_err("front_sysfs: failed to create device file, %s\n",
				dev_attr_front_caminfo.attr.name);
	}
	return 0;
}

int create_flash_sysfs(void)
{
	int err = -ENODEV;

	pr_err("flash_sysfs: error, camera class not exist-HS0407");

	if (IS_ERR_OR_NULL(camera_class)) {
		pr_err("flash_sysfs: error, camera class not exist");
		return -ENODEV;
	}

	flash_dev = device_create(camera_class, NULL, 0, NULL, "flash");
	if (IS_ERR(flash_dev)) {
		pr_err("flash_sysfs: failed to create device(flash)\n");
		return -ENODEV;
	}

	err = device_create_file(flash_dev, &dev_attr_rear_flash);
	if (unlikely(err < 0)) {
		pr_err("flash_sysfs: failed to create device file, %s\n",
				dev_attr_rear_flash.attr.name);
	}

	err = device_create_file(flash_dev, &dev_attr_front_flash);
	if (unlikely(err < 0)) {
		pr_err("flash_sysfs: failed to create device file, %s\n",
				dev_attr_front_flash.attr.name);
	}
	return 0;
}

static int s2mu005_led_probe(struct platform_device *pdev)
{
	int ret = 0, i = 0;
	u8 temp = 0;

	probe_flag = false;

	struct s2mu005_dev *s2mu005 = dev_get_drvdata(pdev->dev.parent);
#ifndef CONFIG_OF
	struct s2mu005_mfd_platform_data *s2mu005_pdata = s2mu005->pdata;
#endif
	struct s2mu005_fled_platform_data *pdata;
	struct s2mu005_led_data *led_data;
	struct s2mu005_led *data;
	struct s2mu005_led_data **led_datas;

#ifdef CONFIG_LEDS_S2MU005_FLASH
	if (camera_class == NULL){
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class))
		pr_err("failed to create device cam_dev_rear!\n");
	}
#endif
	pr_err("[%s] s2mu005_fled start\n", __func__);

	if (!s2mu005) {
		dev_err(&pdev->dev, "drvdata->dev.parent not supplied\n");
		return -ENODEV;
	}
#ifdef CONFIG_OF
	pdata = kzalloc(sizeof(struct s2mu005_fled_platform_data), GFP_KERNEL);
	if (!pdata) {
		pr_err("[%s] failed to allocate driver data\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_LEDS_S2MU005_FLASH
	if (s2mu005->dev->of_node) {
		pr_err("[%s]: prepare to parse dtsi file for s2mu flash\n",__func__);//jtt just for debug info
		ret = s2mu005_led_dt_parse_pdata(&pdev->dev, pdata);
		if (ret < 0) {
			pr_err("[%s] not found leds dt! ret[%d]\n",
				__func__, ret);
			kfree(pdata);
			return -1;
		}
	}
#endif
#else
	if (!s2mu005_pdata) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -ENODEV;
	}
	pdata = s2mu005_pdata->fled_platform_data;
	if (!pdata) {
		pr_err("[%s] no platform data for this led is found\n",
				__func__);
		return -EFAULT;
	}
#endif

	led_datas = devm_kzalloc(s2mu005->dev,
			sizeof(struct s2mu005_led_data *) *
			S2MU005_LED_MAX, GFP_KERNEL);
	if (!led_datas) {
		pr_err("[%s] memory allocation error led_datas", __func__);
		kfree(pdata);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led_datas);// jtt  here led_datas is empty,useful??

	//pr_info("%s %d leds\n", __func__, pdata->num_leds);
	pr_err("%s  %d  leds get from dtsi\n", __func__, pdata->num_leds); // jtt debug info

	for (i = 0; i != pdata->num_leds; ++i) {
		pr_info("%s led%d setup ...\n", __func__, i);

		data = devm_kzalloc(s2mu005->dev, sizeof(struct s2mu005_led),
				GFP_KERNEL);
		if (!data) {
			pr_err("[%s] memory allocation error data\n",
					__func__);
			ret = -ENOMEM;
			continue;
		}

		memcpy(data, &(pdata->leds[i]), sizeof(struct s2mu005_led));
		led_data = devm_kzalloc(&pdev->dev,
				sizeof(struct s2mu005_led_data), GFP_KERNEL);

		g_led_datas[i] = led_data;
		led_datas[i] = led_data;

		if (!led_data) {
			pr_err("[%s] memory allocation error led_data\n",
					__func__);
			kfree(data);
			ret = -ENOMEM;
			continue;
		}

		led_data->i2c = s2mu005->i2c;
		led_data->data = data;
		led_data->cdev.name = data->name;
		led_data->cdev.brightness_set = s2mu005_led_set;
		led_data->cdev.flags = 0;
		led_data->cdev.brightness = data->brightness;
		led_data->cdev.max_brightness = led_data->data->id ?
		S2MU005_TORCH_OUT_I_400MA : S2MU005_FLASH_OUT_I_1200MA;
		
		//s2mu005_led_client = s2mu005->i2c;

		mutex_init(&led_data->lock);
		spin_lock_init(&led_data->value_lock);
		INIT_WORK(&led_data->work, s2mu005_led_work);// jtt if there is any call for "s2mu005_led_work" from &led_data->work->func
		ret = led_classdev_register(&pdev->dev, &led_data->cdev);// jtt register 2 child device: flash & torch    ??necessary??
		if (ret < 0) {
			pr_err("unable to register FLED %d\n",i);
			cancel_work_sync(&led_data->work);
			mutex_destroy(&led_data->lock);
			kfree(data);
			kfree(led_data);
			led_datas[i] = NULL;
			g_led_datas[i] = NULL;
			ret = -EFAULT;
			continue;
		}
		if (led_data->data->id == S2MU005_FLASH_LED){		
			create_rear_sysfs();
		}
		if (led_data->data->id == S2MU005_TORCH_LED) {
			create_front_sysfs();
		}
		if (led_data->data->id == S2MU005_TORCH_LED) {
			pr_err("[%s] create_flash_sysfs start\n", __func__);
			create_flash_sysfs();
			pr_err("[%s] create_flash_sysfs end\n", __func__);
		}
#ifndef CONFIG_S2MU005_LEDS_I2C
		pr_err("[%s] JTT debug infro for macro1\n", __func__);

		if (gpio_is_valid(pdata->torch_pin) &&
				gpio_is_valid(pdata->flash_pin)) {
			if (ret < 0) {
				pr_err("%s : s2mu005 fled gpio allocation error\n",
					__func__);
			} else {
				led_data->torch_pin = pdata->torch_pin;
				led_data->flash_pin = pdata->flash_pin;
				gpio_request_one(pdata->torch_pin, GPIOF_OUT_INIT_LOW, "LED_GPIO_OUTPUT_LOW");
				gpio_request_one(pdata->flash_pin, GPIOF_OUT_INIT_LOW, "LED_GPIO_OUTPUT_LOW");
				gpio_free(pdata->torch_pin);
				gpio_free(pdata->flash_pin);
			}
		}
#endif

		led_data->flash_brightness = pdata->flash_brightness;
		led_data->preflash_brightness = pdata->preflash_brightness;
		led_data->movie_brightness = pdata->movie_brightness;
		led_data->rear_torch_brightness= pdata->rear_torch_brightness;
		led_data->torch_brightness = pdata->torch_brightness;
		led_data->factory_brightness = pdata->factory_brightness;
		

		ret = s2mu005_read_reg(led_data->i2c, 0x73, &temp);	/* EVT0 0x73[3:0] == 0x0 */  // jtt reg0x73??
		if (ret < 0)
			pr_err("%s : s2mu005 reg fled read fail\n",__func__);
		pr_err("%s : s2mu005 reg fled read 0x73 = %d\n",__func__,temp);

		if ((temp & 0xf) == 0x00) {// jtt need to check why can not get into if
			pr_err(" \n[%s] JTT add for debug only* \n",__func__);
			/* FLED_CTRL4 = 0x3A */
			CH_FLASH_TORCH_EN = S2MU005_REG_FLED_CTRL4;
		}
#if 0
    pdata->fled_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pdata->fled_pinctrl)) {
		pr_err("%s:%d Getting pinctrl handle failed\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	pdata->gpio_state_active = pinctrl_lookup_state(pdata->fled_pinctrl, FLED_PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pdata->gpio_state_active)) {
		pr_err("%s:%d Failed to get the active state pinctrl handle\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	pdata->gpio_state_suspend = pinctrl_lookup_state(pdata->fled_pinctrl, FLED_PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pdata->gpio_state_suspend)) {
		pr_err("%s:%d Failed to get the active state pinctrl handle\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	ret = pinctrl_select_state(pdata->fled_pinctrl, pdata->gpio_state_suspend);
	if (ret) {
		pr_err("%s:%d cannot set pin to active state", __func__, __LINE__);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_MUIC_NOTIFIER
		pr_err("[%s] JTT debug infro for macro2\n", __func__);

		muic_notifier_register(&led_data->batt_nb,
				ta_notification,
				MUIC_NOTIFY_DEV_CHARGER);
#endif
		pr_err("[%s] s2mu005_led_setup start\n", __func__);

		ret = s2mu005_led_setup(led_data);
		if (ret < 0)
			pr_err("%s : failed s2mu005 led reg init\n", __func__);
	}

	probe_flag = true;

#ifdef CONFIG_OF
	kfree(pdata);
#endif
	pr_err("%s end!", __func__);
	return 0;
}

static int s2mu005_led_remove(struct platform_device *pdev)
{
	struct s2mu005_led_data **led_datas = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i != S2MU005_LED_MAX; ++i) {
		if (led_datas[i] == NULL)
			continue;

		cancel_work_sync(&led_datas[i]->work);
		mutex_destroy(&led_datas[i]->lock);
		led_classdev_unregister(&led_datas[i]->cdev);
		kfree(led_datas[i]->data);
		kfree(led_datas[i]);
		g_led_datas[i] = NULL;
	}
	kfree(led_datas);

	return 0;
}

static struct of_device_id s2mu005_led_match_table[] = {
	{ .compatible = "samsung,s2mu005-fled",},
	{},
};

static struct platform_driver s2mu005_led_driver = {
	
	.probe  = s2mu005_led_probe,
	.remove = s2mu005_led_remove,
	.driver = {
		.name  = "s2mu005-flash",
		.owner = THIS_MODULE,
		.of_match_table = s2mu005_led_match_table,
		},
		
};

static int __init s2mu005_led_driver_init(void)
{
	pr_err("s2mu005_led_driver_init called");
	return platform_driver_register(&s2mu005_led_driver);
}
module_init(s2mu005_led_driver_init);

static void __exit s2mu005_led_driver_exit(void)
{
	platform_driver_unregister(&s2mu005_led_driver);
}
module_exit(s2mu005_led_driver_exit);

MODULE_AUTHOR("SUJI LEE <suji0908.lee@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG s2mu005 LED Driver");
MODULE_LICENSE("GPL");
