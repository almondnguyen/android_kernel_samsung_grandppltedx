/* inclue/linux/platform_data/gen-panel.h
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 * Header file for Samsung Display Panel(LCD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _GEN_PANEL_GENERIC_H
#define _GEN_PANEL_GENERIC_H
#include <linux/mutex.h>
#include <linux/pm_qos.h>
#include <linux/platform_data/gen-panel-mdnie.h>

#define MAX_OLED_BRT	(255)
struct lcd;
struct oled_brt_map {
	unsigned int candela:12;
	unsigned int aid:12;
	unsigned int elvss:8;
};

struct candela_map {
	unsigned int *candela;
	unsigned int *value;
	size_t size;
};

enum {
	GEN_PANEL_RGB = 0,
	GEN_PANEL_BGR = 1,
};

enum {
	GEN_PANEL_PWR_OFF = 0,
	GEN_PANEL_PWR_ON_0 = 1, /* BEFORE LP11 */
	GEN_PANEL_PWR_ON_1 = 2, /* AFTER LP11 */
};

enum {
	OFFSET_R,
	OFFSET_G,
	OFFSET_B,
	RGB_OFFSET_MAX,
};

/*
	PANEL_INIT_CMD and PANEL_ENABLE_CMD :
		be transmitted befor it starts to transfer FB data via DSI bus.
	PANEL_POST_ENABLE_CMD :
		be transmitted after it start to transfer FB data.
*/
enum {
	PANEL_INIT_CMD,
	PANEL_ENABLE_CMD,
	PANEL_POST_ENABLE_CMD,
	PANEL_POST_ENABLE_1_CMD,
	PANEL_DISABLE_CMD,
	PANEL_BL_ON_CMD,
	PANEL_BL_SET_BRT_CMD,
	PANEL_NV_ENABLE_CMD,
	PANEL_NV_DISABLE_CMD,
	PANEL_HBM_ON,
	PANEL_HBM_OFF,
	PANEL_TSET_CMD,
	PANEL_AID_CMD,
	PANEL_ELVSS_CMD,
	PANEL_ELVSS_DIM_OFFSET_CMD,
	PANEL_ELVSS_TEMPERATURE_CMD,
	PANEL_GAMMA_CMD,
	PANEL_GAMMA_UPDATE_CMD,
#ifdef CONFIG_GEN_PANEL_OCTA
	PANEL_ACL_ON_CMD,
	PANEL_ACL_OFF_CMD,
#endif
	PANEL_CABC_ON_CMD,
	PANEL_CABC_OFF_CMD,
#ifdef CONFIG_GEN_PANEL_MDNIE

	PANEL_MDNIE_NEGATIVE_CMD,
	PANEL_MDNIE_COLOR_ADJ_CMD,
	PANEL_MDNIE_CURTAIN_CMD,
	PANEL_MDNIE_GRAY_SCALE_CMD,
	PANEL_MDNIE_GRAY_SCALE_NEGATIVE_CMD,
	PANEL_MDNIE_OUTDOOR_CMD,

	PANEL_MDNIE_HBM_CMD,
	PANEL_MDNIE_HBM_TEXT_CMD,

	/* STANDARD MODE */
	PANEL_MDNIE_STANDARD_MODE,
	PANEL_MDNIE_UI_CMD = PANEL_MDNIE_STANDARD_MODE,
	PANEL_MDNIE_VIDEO_CMD,
	PANEL_MDNIE_CAMERA_CMD,
	PANEL_MDNIE_GALLERY_CMD,
	PANEL_MDNIE_VT_CMD,
	PANEL_MDNIE_BROWSER_CMD,
	PANEL_MDNIE_EBOOK_CMD,
	PANEL_MDNIE_EMAIL_CMD,
	PANEL_MDNIE_STANDARD_MODE_END = PANEL_MDNIE_EMAIL_CMD,

	/* AUTO MODE */
	PANEL_MDNIE_AUTO_MODE,
	PANEL_MDNIE_AUTO_UI_CMD = PANEL_MDNIE_AUTO_MODE,
	PANEL_MDNIE_AUTO_VIDEO_CMD,
	PANEL_MDNIE_AUTO_CAMERA_CMD,
	PANEL_MDNIE_AUTO_GALLERY_CMD,
	PANEL_MDNIE_AUTO_VT_CMD,
	PANEL_MDNIE_AUTO_BROWSER_CMD,
	PANEL_MDNIE_AUTO_EBOOK_CMD,
	PANEL_MDNIE_AUTO_MODE_END = PANEL_MDNIE_AUTO_EBOOK_CMD,

	/* DYNAMIC MODE */
	PANEL_MDNIE_DYNAMIC_MODE,
	PANEL_MDNIE_DYNAMIC_UI_CMD = PANEL_MDNIE_DYNAMIC_MODE,
	PANEL_MDNIE_DYNAMIC_VIDEO_CMD,
	PANEL_MDNIE_DYNAMIC_CAMERA_CMD,
	PANEL_MDNIE_DYNAMIC_GALLERY_CMD,
	PANEL_MDNIE_DYNAMIC_VT_CMD,
	PANEL_MDNIE_DYNAMIC_BROWSER_CMD,
	PANEL_MDNIE_DYNAMIC_EBOOK_CMD,
	PANEL_MDNIE_DYNAMIC_MODE_END = PANEL_MDNIE_DYNAMIC_EBOOK_CMD,

	/* NATURAL MODE */
	PANEL_MDNIE_NATURAL_MODE,
	PANEL_MDNIE_NATURAL_UI_CMD = PANEL_MDNIE_NATURAL_MODE,
	PANEL_MDNIE_NATURAL_VIDEO_CMD,
	PANEL_MDNIE_NATURAL_CAMERA_CMD,
	PANEL_MDNIE_NATURAL_GALLERY_CMD,
	PANEL_MDNIE_NATURAL_VT_CMD,
	PANEL_MDNIE_NATURAL_BROWSER_CMD,
	PANEL_MDNIE_NATURAL_EBOOK_CMD,
	PANEL_MDNIE_NATURAL_MODE_END = PANEL_MDNIE_NATURAL_EBOOK_CMD,

	/* MOVIE MODE */
	PANEL_MDNIE_MOVIE_MODE,
	PANEL_MDNIE_MOVIE_UI_CMD = PANEL_MDNIE_MOVIE_MODE,
	PANEL_MDNIE_MOVIE_VIDEO_CMD,
	PANEL_MDNIE_MOVIE_CAMERA_CMD,
	PANEL_MDNIE_MOVIE_GALLERY_CMD,
	PANEL_MDNIE_MOVIE_VT_CMD,
	PANEL_MDNIE_MOVIE_BROWSER_CMD,
	PANEL_MDNIE_MOVIE_EBOOK_CMD,
	PANEL_MDNIE_MOVIE_MODE_END = PANEL_MDNIE_MOVIE_EBOOK_CMD,
#endif
	PANEL_OP_CMD_MAX,
};

#ifdef CONFIG_GEN_PANEL_MDNIE
enum {
	PANEL_MDNIE_SCENARIO_UI,
	PANEL_MDNIE_SCENARIO_VIDEO,
	PANEL_MDNIE_SCENARIO_CAMERA,
	PANEL_MDNIE_SCENARIO_GALLERY,
	PANEL_MDNIE_SCENARIO_VT,
	PANEL_MDNIE_SCENARIO_BROWSER,
	PANEL_MDNIE_SCENARIO_EBOOK,
	PANEL_MDNIE_SCENARIO_EMAIL,
	PANEL_MDNIE_SCENARIO_MAX,
};

static int PANEL_MDNIE_MODES[] = {
	PANEL_MDNIE_DYNAMIC_MODE,
	PANEL_MDNIE_STANDARD_MODE,
	PANEL_MDNIE_NATURAL_MODE,
	PANEL_MDNIE_MOVIE_MODE,
	PANEL_MDNIE_AUTO_MODE,
};

#define _MDNIE_MODE_SIZE(mode)	(((mode##_END) - (mode) + 1))
static int PANEL_MDNIE_MODES_SIZE[] = {
	_MDNIE_MODE_SIZE(PANEL_MDNIE_DYNAMIC_MODE),
	_MDNIE_MODE_SIZE(PANEL_MDNIE_STANDARD_MODE),
	_MDNIE_MODE_SIZE(PANEL_MDNIE_NATURAL_MODE),
	_MDNIE_MODE_SIZE(PANEL_MDNIE_MOVIE_MODE),
	_MDNIE_MODE_SIZE(PANEL_MDNIE_AUTO_MODE),
};

static int PANEL_MDNIE_SCENARIOS[] = {
	PANEL_MDNIE_SCENARIO_UI,
	PANEL_MDNIE_SCENARIO_VIDEO,
	PANEL_MDNIE_SCENARIO_VIDEO,
	PANEL_MDNIE_SCENARIO_VIDEO,
	PANEL_MDNIE_SCENARIO_CAMERA,
	PANEL_MDNIE_SCENARIO_UI,	/* Navi */
	PANEL_MDNIE_SCENARIO_GALLERY,
	PANEL_MDNIE_SCENARIO_VT,
	PANEL_MDNIE_SCENARIO_BROWSER,
	PANEL_MDNIE_SCENARIO_EBOOK,
	PANEL_MDNIE_SCENARIO_EMAIL,
};

static inline int MDNIE_MODE_SIZE(int mode)
{
	if (mode >= ARRAY_SIZE(PANEL_MDNIE_MODES)) {
		pr_err("%s, unknown mode(%d)\n", __func__, mode);
		return 0;
	}
	return PANEL_MDNIE_MODES_SIZE[mode];
}

static inline int MDNIE_MODE_OP_INDEX(int mode)
{
	if (mode >= ARRAY_SIZE(PANEL_MDNIE_MODES)) {
		pr_err("%s, unknown mode(%d)\n", __func__, mode);
		return PANEL_MDNIE_STANDARD_MODE;
	}
	return PANEL_MDNIE_MODES[mode];
}

static inline int MDNIE_OP_INDEX(int mode, int scenario)
{
	if (mode >= ARRAY_SIZE(PANEL_MDNIE_MODES)) {
		pr_err("%s, unknown mode(%d)\n", __func__, mode);
		return PANEL_MDNIE_UI_CMD;
	}
	if (scenario >= ARRAY_SIZE(PANEL_MDNIE_SCENARIOS)) {
		pr_err("%s, unknown scenario(%d)\n", __func__, scenario);
		return PANEL_MDNIE_UI_CMD;
	}

	if (mode != MDNIE_STANDARD &&
		PANEL_MDNIE_SCENARIOS[scenario]	>= PANEL_MDNIE_SCENARIO_EMAIL)
		return PANEL_MDNIE_STANDARD_MODE +
			PANEL_MDNIE_SCENARIOS[scenario];
	return PANEL_MDNIE_MODES[mode] + PANEL_MDNIE_SCENARIOS[scenario];
}
#endif

/*
	gen-panel-init-cmds and gen-panel-enable-cmds :
		be transmitted befor it starts to transfer FB data via DSI bus.
	gen-panel-post-enable-cmds:
		be transmitted after it start to transfer FB data.
*/

static const char * const op_cmd_names[] = {
	"gen-panel-init-cmds",
	"gen-panel-enable-cmds",
	"gen-panel-post-enable-cmds",
	"gen-panel-post-enable-1-cmds",
	"gen-panel-disable-cmds",
	"gen-panel-backlight-on-cmds",
	"gen-panel-backlight-set-brightness-cmds",
	"gen-panel-nv-read-enable-cmds",
	"gen-panel-nv-read-disable-cmds",
	"gen-panel-hbm-on-cmds",
	"gen-panel-hbm-off-cmds",
	"gen-panel-tset-cmds",
	"gen-panel-aid-cmds",
	"gen-panel-elvss-cmds",
	"gen-panel-elvss-dim-offset-cmds",
	"gen-panel-elvss-temperature-cmds",
	"gen-panel-gamma-cmds",
	"gen-panel-gamma-update-cmds",
#ifdef CONFIG_GEN_PANEL_OCTA
	"gen-panel-acl-on-cmds",
	"gen-panel-acl-off-cmds",
#endif
	"gen-panel-cabc-on-cmds",
	"gen-panel-cabc-off-cmds",
#ifdef CONFIG_GEN_PANEL_MDNIE
	"gen-panel-mdnie-negative-mode-cmds",
	"gen-panel-mdnie-color-adjustment-mode-cmds",
	"gen-panel-mdnie-dark-screene-mode-cmds",
	"gen-panel-mdnie-gray-scale-cmds",
	"gen-panel-mdnie-gray-scale-negative-cmds",
	"gen-panel-mdnie-outdoor-mode-cmds",

	/* HBM */
	"gen-panel-mdnie-hbm-cmds",
	"gen-panel-mdnie-hbm-text-cmds",

	/* STANDARD MODE */
	"gen-panel-mdnie-ui-mode-cmds",
	"gen-panel-mdnie-video-mode-cmds",
	"gen-panel-mdnie-camera-mode-cmds",
	"gen-panel-mdnie-gallery-mode-cmds",
	"gen-panel-mdnie-vt-mode-cmds",
	"gen-panel-mdnie-browser-mode-cmds",
	"gen-panel-mdnie-ebook-mode-cmds",
	"gen-panel-mdnie-email-mode-cmds",

	/* AUTO MODE */
	"gen-panel-mdnie-auto-ui-mode-cmds",
	"gen-panel-mdnie-auto-video-mode-cmds",
	"gen-panel-mdnie-auto-camera-mode-cmds",
	"gen-panel-mdnie-auto-gallery-mode-cmds",
	"gen-panel-mdnie-auto-vt-mode-cmds",
	"gen-panel-mdnie-auto-browser-mode-cmds",
	"gen-panel-mdnie-auto-ebook-mode-cmds",

	/* DYNAMIC MODE */
	"gen-panel-mdnie-dynamic-ui-mode-cmds",
	"gen-panel-mdnie-dynamic-video-mode-cmds",
	"gen-panel-mdnie-dynamic-camera-mode-cmds",
	"gen-panel-mdnie-dynamic-gallery-mode-cmds",
	"gen-panel-mdnie-dynamic-vt-mode-cmds",
	"gen-panel-mdnie-dynamic-browser-mode-cmds",
	"gen-panel-mdnie-dynamic-ebook-mode-cmds",

	/* NATURAL MODE */
	"gen-panel-mdnie-natural-ui-mode-cmds",
	"gen-panel-mdnie-natural-video-mode-cmds",
	"gen-panel-mdnie-natural-camera-mode-cmds",
	"gen-panel-mdnie-natural-gallery-mode-cmds",
	"gen-panel-mdnie-natural-vt-mode-cmds",
	"gen-panel-mdnie-natural-browser-mode-cmds",
	"gen-panel-mdnie-natural-ebook-mode-cmds",

	/* NATURAL MODE */
	"gen-panel-mdnie-movie-ui-mode-cmds",
	"gen-panel-mdnie-movie-video-mode-cmds",
	"gen-panel-mdnie-movie-camera-mode-cmds",
	"gen-panel-mdnie-movie-gallery-mode-cmds",
	"gen-panel-mdnie-movie-vt-mode-cmds",
	"gen-panel-mdnie-movie-browser-mode-cmds",
	"gen-panel-mdnie-movie-ebook-mode-cmds",
#endif
};

enum dsi_tx_mode {
	DSI_HS_MODE = 0,
	DSI_LP_MODE = 1,
};

static const char * const tx_modes[] = {
	"dsi-hs-mode", "dsi-lp-mode",
};

struct gen_cmd_hdr {
	u8 dtype;	/* data type */
	u8 txmode;	/* tx mode */
	u16 wait;	/* ms */
	u16 dlen;	/* data length */
};

struct gen_cmd_desc {
	u8 data_type;
	u8 lp;
	unsigned int delay;
	unsigned int length;
	u8 *data;
};

struct gen_cmds_info {
	char *name;
	struct gen_cmd_desc *desc;
	unsigned int nr_desc;
};

struct manipulate_table {
	struct list_head list;
	struct device_node *np;
	u32 type;
	struct gen_cmds_info cmd;
};

struct manipulate_action {
	struct list_head list;
	const char *name;
	struct manipulate_table **pmani;
	unsigned int nr_mani;
};

struct gen_dev_attr {
	struct list_head list;
	const char *attr_name;
	struct device_attribute dev_attr;
	struct list_head action_list;
	struct manipulate_action *action;
};

enum {
	EXT_PIN_OFF = 0,
	EXT_PIN_ON = 1,
	EXT_PIN_LOCK = 2,
};

enum {
	/* gpio controlled LDO based supply to LCD */
	EXT_PIN_GPIO = 0,
	/* PMIC Regulator based supply to LCD */
	EXT_PIN_REGULATOR = 1,
};

enum {
	TEMPERATURE_LOW = 0,
	TEMPERATURE_HIGH = 1,
};

struct extpin {
	struct list_head list;
	const char *name;
	u32 type;
	union {
		int gpio;
		struct regulator *supply;
	};
	struct mutex expires_lock;
	unsigned long expires;
};

struct extpin_ctrl {
	struct list_head list;
	struct extpin *pin;
	u32 on;
	u32 usec;
};

struct extpin_ctrl_list {
	unsigned int nr_ctrls;
	struct extpin_ctrl *ctrls;
};

struct temp_compensation {
	u8 *new_data;
	u8 *old_data;
	u32 data_len;
	u32 trig_type;
	int temperature;
};

struct read_info {
	u8 reg;
	u8 idx;
	u8 len;
} __attribute__((__packed__));

struct gen_panel_ops {
	int (*tx_cmds)(const struct lcd *, const void *, int);
	int (*rx_cmds)(const struct lcd *, u8 *, const void *, int);
#if CONFIG_OF
	int (*parse_dt)(const struct device_node *);
#endif
};

struct gen_panel_mode {
	const char *name;
	unsigned int refresh;
	unsigned int xres;
	unsigned int yres;
	unsigned int real_xres;
	unsigned int real_yres;
	unsigned int left_margin;
	unsigned int right_margin;
	unsigned int upper_margin;
	unsigned int lower_margin;
	unsigned int hsync_len;
	unsigned int vsync_len;
	unsigned int hsync_invert;
	unsigned int vsync_invert;
	unsigned int invert_pixclock;
	unsigned int pixclock_freq;
	int pix_fmt_out;
	u32 height; /* screen height in mm */
	u32 width; /* screen width in mm */
};

enum {
	BRIGHTNESS_MODE_NONE = 0,
	BRIGHTNESS_MODE_COLOR_WEAKNESS = 1,
	BRIGHTNESS_MODE_CLEAR_VIEW = 2,
};

struct lcd {
	struct device *dev;
	struct class *class;
	void *pdata;
	struct backlight_device *bd;
	u32 (*set_panel_id)(struct lcd *lcd);
	const struct gen_panel_ops *ops;
	/* Further fields can be added here */
	struct mutex access_ok;
	const char *manufacturer_name;
	const char *panel_model_name;
	const char *panel_name;
	char manufacturer_code[5];
	unsigned int id;
	unsigned int esd_type;
	int esd_gpio;
	unsigned rst_gpio_en:1;
	unsigned int rst_gpio;
	unsigned active:1;
	unsigned esd_en:1;
	unsigned temp_comp_en:1;
	unsigned power:1;
	unsigned acl:1;
	unsigned hbm:1;
	unsigned rgb:1;         /* 0 - rgb, 1 - bgr */
	int temperature;
	int brightness_mode;
	/* OCTA parameter offset */
	u8 octa_product_reg;
	u8 octa_coordinate_reg;
	int octa_product_gpara;
	int octa_coordinate_gpara;
	int tset_param_offset;
	int mps_param_offset;
	int elvss_param_offset;
	int elvss_temp_param_offset;
	int aid_param_offset;
	int hbm_gamma_param_offset;
	u8 temperature_elvss_value;
	struct gen_panel_mode mode;
	/* mdnie lite */
	struct mdnie_lite mdnie;
	/* external pins */
	struct extpin_ctrl_list extpin_on_0_seq;
	struct extpin_ctrl_list extpin_on_seq;
	struct extpin_ctrl_list extpin_off_seq;
	/* pin ctrl */
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_enable;
	struct pinctrl_state *pin_disable;
	/* command tables */
	struct gen_cmds_info *op_cmds;
	/* temperature compensation */
	struct temp_compensation *temp_comp;
	unsigned int nr_temp_comp;
	struct read_info id_rd_info[3];
	struct read_info hbm_rd_info[3];
	struct read_info mtp_rd_info[1];
	u32 set_brt_reg;
	u8 color_adj_reg;
	int color_adj_offset;
	u32 status_reg;
	u32 status_ok;
};

/* status types */
enum {
	/* status of what object at */
	GEN_PANEL_OFF = 0,
	GEN_PANEL_ON,
	GEN_PANEL_ON_REDUCED,
	GEN_PANEL_OFF_DMA,
	GEN_PANEL_ON_DMA,
	GEN_PANEL_RESET,
};

static inline int gen_panel_is_reduced_on(int status)
{
	return (status == GEN_PANEL_ON_REDUCED);
}

static inline int gen_panel_is_on(int status)
{
	return (status == GEN_PANEL_ON ||
		status == GEN_PANEL_ON_REDUCED ||
		status == GEN_PANEL_ON_DMA);
}

static inline int gen_panel_is_off(int status)
{
	return (status == GEN_PANEL_OFF ||
		status == GEN_PANEL_OFF_DMA);
}

u32 get_panel_id(void);
int gen_dsi_panel_verify_reg(struct lcd *lcd,
		struct gen_cmd_desc desc[], int count);

extern bool lcd_connected(void);
#ifdef CONFIG_GEN_PANEL_TUNING
extern int gen_panel_attach_tuning(struct lcd *);
extern void gen_panel_detach_tuning(struct lcd *);
#else
static inline int gen_panel_attach_tuning(struct lcd *lcd) { return 0; }
static inline void gen_panel_detach_tuning(struct lcd *lcd) {}
#endif

extern int gen_panel_probe(struct device_node *, struct lcd *lcd);
extern int gen_panel_remove(struct lcd *);
extern void gen_panel_set_status(struct lcd *lcd, int status);
extern void gen_panel_start(struct lcd *lcd, int status);
extern void gen_panel_set_external_pin(struct lcd *lcd, int status);
extern void gen_panel_set_external_pin_1(struct lcd *lcd, int status);

#define panel_usleep(t)				\
do {						\
	unsigned int usec = (t);		\
	if (usec < 20 * 1000)			\
		usleep_range(usec, usec + 10);	\
	else					\
		msleep(usec / 1000);		\
} while (0)
#endif /* _GEN_PANEL_GENERIC_H */
