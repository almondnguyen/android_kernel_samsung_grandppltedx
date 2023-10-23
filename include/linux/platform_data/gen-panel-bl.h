/* inclue/linux/platform_data/gen-panel-bl.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 * Header file for Samsung Display Backlight(LCD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _GEN_PANEL_BACKLIGHT_H
#define _GEN_PANEL_BACKLIGHT_H
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/platform_data/gen-panel.h>

#define GEN_PANEL_BL_NAME	("gen-panel-backlight")
#define IS_HBM(level)	(level >= 6)

enum {
	BL_CTRL_PWM,
	BL_CTRL_EASY_SCALE,
	BL_CTRL_STEP_CTRL,
	MIPI_CTRL,
};

enum {
	BRT_VALUE_OFF = 0,
	BRT_VALUE_MIN,
	BRT_VALUE_DIM,
	BRT_VALUE_DEF,
	BRT_VALUE_MAX,
	BRT_VALUE_HBM,
	MAX_BRT_VALUE_IDX,
};

struct brt_value {
	int brightness;	/* brightness level from user */
	int tune_level;	/* tuning value be sent */
};

struct gen_panel_backlight_ops {
	int (*set_brightness)(void *, int level);
	int (*get_brightness)(void *);
};

struct gen_panel_backlight_info {
	const char *name;
	atomic_t enable;
	void *bd_data;
	struct mutex ops_lock;
	const struct gen_panel_backlight_ops *ops;
	struct brt_value range[MAX_BRT_VALUE_IDX];
	unsigned int *maptbl;
	unsigned int nr_maptbl;
	unsigned int auto_brightness;
	int current_brightness;
	int prev_tune_level;
	unsigned hbm_en:1;
	unsigned tune_en:1;
};

#ifdef CONFIG_OF
static inline struct device_node *
gen_panel_find_dt_backlight(struct device_node *node, const char *pname)
{
	struct device_node *backlight_node =
		of_parse_phandle(node, pname, 0);

	if (!backlight_node) {
		pr_err("%s: backlight_node not found\n", __func__);
		return NULL;
	}

	return backlight_node;
}
#else
static inline struct device_node *
gen_panel_find_dt_backlight(struct platform_device *pdev, const char *pname)
{
	return NULL;
}
#endif

static inline int
gen_panel_backlight_enable(struct backlight_device *bd)
{
#ifdef CONFIG_PM_RUNTIME
	if (bd && bd->dev.parent)
		return pm_runtime_get_sync(bd->dev.parent);
#elif CONFIG_PM_SLEEP
	if (bd && bd->dev.parent) {
		pr_info("%s\n", __func__);
		return platform_pm_resume(bd->dev.parent);
	} else {
		pr_info("%s, bd %p\n", __func__, bd);
	}
#endif
	return 1;
}

static inline int
gen_panel_backlight_disable(struct backlight_device *bd)
{
#ifdef CONFIG_PM_RUNTIME
	if (bd && bd->dev.parent)
		return pm_runtime_put_sync(bd->dev.parent);
#elif CONFIG_PM_SLEEP
	if (bd && bd->dev.parent) {
		pr_info("%s\n", __func__);
		return platform_pm_suspend(bd->dev.parent);
	} else {
		pr_info("%s, bd %p\n", __func__, bd);
	}
#endif
	return 1;
}

static inline int
gen_panel_backlight_is_on(struct backlight_device *bd)
{
#ifdef CONFIG_PM_RUNTIME
	if (bd && bd->dev.parent)
		return atomic_read(&bd->dev.parent->power.usage_count);
#else
	struct gen_panel_backlight_info *bl_info =
		(struct gen_panel_backlight_info *)bl_get_data(bd);

	if (bl_info)
		return atomic_read(&bl_info->enable);
#endif
	return 0;
}

static inline int
gen_panel_backlight_onoff(struct backlight_device *bd, int on)
{
	int ret;

	if (on)
		ret = gen_panel_backlight_enable(bd);
	else
		ret = gen_panel_backlight_disable(bd);

	if (ret)
		pr_err("%s: error enter %s\n",
				__func__, on ? "resume" : "suspend");

	return ret;
}

#ifdef CONFIG_GEN_PANEL_BACKLIGHT
extern int gen_panel_backlight_device_register(struct backlight_device *,
		void *, const struct gen_panel_backlight_ops *);
extern void gen_panel_backlight_device_unregister(struct backlight_device *);
extern bool gen_panel_match_backlight(struct backlight_device *, const char *);
#else
static inline int gen_panel_backlight_device_register(struct backlight_device *bd,
		void *bd_data, const struct gen_panel_backlight_ops *ops)
{
	return 0;
}
static inline void gen_panel_backlight_device_unregister(struct backlight_device *bd) {}
static inline bool gen_panel_match_backlight(struct backlight_device *bd,
		const char *match)
{
	return false;
}
#endif
#endif	/* _GEN_PANEL_BACKLIGHT_H */
