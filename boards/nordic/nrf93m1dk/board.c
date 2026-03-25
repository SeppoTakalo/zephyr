/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board, CONFIG_LOG_DEFAULT_LEVEL);

void board_late_init_hook(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_npm1300_charger)

	static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));
	int ret;

	/* Set USB current limit */
	struct sensor_value limit = {
		.val1 = 1,
		.val2 = 500000,
	};

	if (!device_is_ready(charger)) {
		LOG_ERR("Device not ready");
		return;
	}

	ret = sensor_attr_set(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_CONFIGURATION, &limit);

	if (ret < 0) {
		LOG_ERR("Failed to set current limit: %d", ret);
		return;
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_npm1300_charger) */
}
