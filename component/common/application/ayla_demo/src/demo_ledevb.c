/*
 * Copyright 2017 Ayla Networks, Inc.  All rights reserved.
 */

/*
 * Ayla device agent demo of a simple lights and buttons evaluation board
 * using the "simple" property manager.
 *
 * The property names are chosen to be compatible with the Ayla Control
 * App.  E.g., the LED property is Blue_LED even though the color is yellow.
 * Button1 sends the Blue_button property, even though the button is white.
 */
#define HAVE_UTYPES
#include "lwip/ip_addr.h"

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <sys/types.h>
#include <ada/libada.h>
#include <ada/sprop.h>
#include <ada/task_label.h>
#include "conf.h"
#include "demo.h"
#include "led_key.h"

#define BUILD_PROGNAME "ayla_ledevb_demo"
#define BUILD_VERSION "1.0"
#define BUILD_STRING	BUILD_VERSION " "  __DATE__ " " __TIME__

const char mod_sw_build[] = BUILD_STRING;
const char mod_sw_version[] = BUILD_PROGNAME " " BUILD_STRING;

/*
 * The oem and oem_model strings determine the host name for the
 * Ayla device service and the device template on the service.
 *
 * If these are changed, the encrypted OEM secret must be re-encrypted
 * unless the oem_model was "*" (wild-card) when the oem_key was encrypted.
 */
char oem[] = DEMO_OEM_ID;
char oem_model[] = DEMO_LEDEVB_MODEL;

static unsigned blue_button;
static u8 blue_led;
static u8 green_led;
static int input;
static int output;
static int decimal_in;
static int decimal_out;
static char version[] = BUILD_PROGNAME " " BUILD_STRING;
static char cmd_buf[TLV_MAX_STR_LEN + 1];
static char demo_host_version[] = "1.0-rtk";	/* property template version */

static enum ada_err demo_led_set(struct ada_sprop *, const void *, size_t);
static enum ada_err demo_int_set(struct ada_sprop *, const void *, size_t);
static enum ada_err demo_cmd_set(struct ada_sprop *, const void *, size_t);

static struct ada_sprop demo_props[] = {
	/*
	 * version properties
	 */
	{ "oem_host_version", ATLV_UTF8,
	    demo_host_version, sizeof(demo_host_version),
	    ada_sprop_get_string, NULL},
	{ "version", ATLV_UTF8, &version[0], sizeof(version),
	    ada_sprop_get_string, NULL},
	/*
	 * boolean properties
	 */
	{ "Blue_button", ATLV_BOOL, &blue_button, sizeof(blue_button),
	    ada_sprop_get_bool, NULL},
	{ "Blue_LED", ATLV_BOOL, &blue_led, sizeof(blue_led),
	    ada_sprop_get_bool, demo_led_set },
	{ "Green_LED", ATLV_BOOL, &green_led, sizeof(green_led),
	    ada_sprop_get_bool, demo_led_set },
	/*
	 * string properties
	 */
	{ "cmd", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    ada_sprop_get_string, demo_cmd_set },
	{ "log", ATLV_UTF8, &cmd_buf[0], sizeof(cmd_buf),
	    ada_sprop_get_string, NULL },
	/*
	 * integer properties
	 */
	{ "input", ATLV_INT, &input, sizeof(input),
	    ada_sprop_get_int, demo_int_set },
	{ "output", ATLV_INT, &output, sizeof(output),
	    ada_sprop_get_int, NULL },
	/*
	 * decimal properties
	 */
	{ "decimal_in", ATLV_CENTS, &decimal_in, sizeof(decimal_in),
	    ada_sprop_get_int, demo_int_set },
	{ "decimal_out", ATLV_CENTS, &decimal_out, sizeof(decimal_out),
	    ada_sprop_get_int, NULL },
};

static void prop_send_by_name(const char *name)
{
	enum ada_err err;

	err = ada_sprop_send_by_name(name);
	if (err) {
		log_put(LOG_INFO "demo: %s: send of %s: err %d",
		    __func__, name, err);
	}
}

/*
 * Demo set function for bool properties.
 */
static enum ada_err demo_led_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret = 0;

	ret = ada_sprop_set_bool(sprop, buf, len);
	if (ret) {
		return ret;
	}
	if (sprop->val == &blue_led) {
		set_led(led_blue, blue_led);
	} else if (sprop->val == &green_led) {
		set_led(led_green, green_led);
	}
	log_put(LOG_INFO "%s: %s set to %u",
	    __func__, sprop->name, *(u8 *)sprop->val);
	return AE_OK;
}

/*
 * Demo set function for integer and decimal properties.
 */
static enum ada_err demo_int_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_sprop_set_int(sprop, buf, len);
	if (ret) {
		return ret;
	}

	if (sprop->val == &input) {
		log_put(LOG_INFO "%s: %s set to %d",
		    __func__, sprop->name, input);
		output = input;
		prop_send_by_name("output");
	} else if (sprop->val == &decimal_in) {
		log_put(LOG_INFO "%s: %s set to %d",
		    __func__, sprop->name, decimal_in);
		decimal_out = decimal_in;
		prop_send_by_name("decimal_out");
	} else {
		return AE_NOT_FOUND;
	}
	return AE_OK;
}

/*
 * Demo set function for string properties.
 */
static enum ada_err demo_cmd_set(struct ada_sprop *sprop,
				const void *buf, size_t len)
{
	int ret;

	ret = ada_sprop_set_string(sprop, buf, len);
	if (ret) {
		return ret;
	}

	prop_send_by_name("log");
	log_put(LOG_INFO "%s: cloud set %s to \"%s\"",
	    __func__, sprop->name, cmd_buf);
	return AE_OK;
}

/*
 * Initialize property manager.
 */
void demo_init(void)
{
	ada_sprop_mgr_register("ledevb", demo_props, ARRAY_LEN(demo_props));
}

void demo_idle(void)
{
	struct {
		enum gpio gpio;
		int val;
	} button[2];
	int tmp, i;
	int link_led = 0;

	log_thread_id_set(TASK_LABEL_DEMO);
	taskstat_dbg_start();

	init_led_key();

	button[0].gpio = key_blue;
	if (button[0].gpio >= 0) {
		button[0].val = get_key(button[0].gpio);
	}
	button[1].gpio = key_register;
	if (button[1].gpio >= 0) {
		button[1].val = get_key(button[1].gpio);
	}

	prop_send_by_name("oem_host_version");
	prop_send_by_name("version");

	while (1) {
		vTaskDelay(100);

		if (!(ada_sprop_dest_mask & NODES_ADS)) {
			if (link_led) {
				link_led = 0;
				set_led(led_cloud, 0);
			}
		} else if (!link_led) {
			conf_connected = 1;
			link_led = 1;
			set_led(led_cloud, 1);
		}

		for (i = 0; i < ARRAY_LEN(button); i++) {
			tmp = get_key(button[i].gpio);
			if (tmp == button[i].val) {
				continue;
			}
			log_put(LOG_INFO "Button%d change to %d", i,
				tmp);
			button[i].val = tmp;
			if (i == 0) {
				blue_button = button[0].val;
				log_put(LOG_INFO "%s: blue_button to %d",
				    __func__, button[0].val);
				prop_send_by_name("Blue_button");
			}
			if (i == 1 && button[i].val == 1) {
				client_reg_window_start();
			}
		}
	}
}

