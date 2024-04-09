/*
 * SPDX-License-Identifier: WTFPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mailbox_example, LOG_LEVEL_DBG);

static const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
static const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

struct cmd {
	uint8_t ip_id;
	uint8_t cmd_id: 7;
	uint8_t block: 1;
	union {
		struct {
			uint8_t linux;
			uint8_t rtos;
		} valid;
		uint16_t mstime;
	} resv;
	uint32_t param_ptr;
} __packed;

enum SYS_CMD_ID {
	CMD_TEST_A = 0x10,
	CMD_TEST_B,
	CMD_TEST_C,
	CMD_DUO_LED,
	SYS_CMD_INFO_LIMIT,
};

enum DUO_LED_STATUS {
	DUO_LED_ON = 0x02,
	DUO_LED_OFF,
	DUO_LED_DONE,
};

K_MSGQ_DEFINE(msgq, sizeof(struct cmd), 10, 4);

static void callback(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
		     struct mbox_msg *data)
{
	struct cmd cmd;

	memcpy(&cmd, data->data, sizeof(struct cmd));
	k_msgq_put(&msgq, &cmd, K_NO_WAIT);
}

int main(void)
{
	struct mbox_msg mbox_msg;
	struct cmd cmd;
	int ret;

	ret = mbox_register_callback_dt(&rx_channel, callback, NULL);
	if (ret < 0) {
		LOG_ERR("Could not register callback (%d)", ret);
		return 0;
	}

	ret = mbox_set_enabled_dt(&rx_channel, true);
	if (ret < 0) {
		LOG_ERR("Could not enable RX channel %d (%d)", rx_channel.channel_id, ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure GPIO pin %d (%d)", led.pin, ret);
		return 0;
	}

	LOG_DBG("Hello, world!");

	while (1) {
		ret = k_msgq_get(&msgq, &cmd, K_FOREVER);
		if (ret == 0) {
			LOG_DBG("Received cmd ip_id(%d) cmd_id(%d) param_ptr(0x%x)", cmd.ip_id,
				cmd.cmd_id, cmd.param_ptr);

			switch (cmd.cmd_id) {
			case CMD_TEST_A:
				/* send to C906B */
				cmd.cmd_id = CMD_TEST_A;
				cmd.param_ptr = 0x12345678;
				cmd.resv.valid.rtos = 1;
				cmd.resv.valid.linux = 0;
				LOG_DBG("CMD_TEST_A(%d): send param_ptr=0x%x", cmd.cmd_id,
					cmd.param_ptr);
				goto send_label;
			case CMD_TEST_B:
				LOG_DBG("CMD_TEST_B(%d): nothing to do", cmd.cmd_id);
				break;
			case CMD_TEST_C:
				cmd.cmd_id = CMD_TEST_C;
				cmd.param_ptr = 0x55aa;
				cmd.resv.valid.rtos = 1;
				cmd.resv.valid.linux = 0;
				LOG_DBG("CMD_TEST_C(%d): send param_ptr=0x%x", cmd.cmd_id,
					cmd.param_ptr);
				goto send_label;
			case CMD_DUO_LED:
				cmd.cmd_id = CMD_DUO_LED;
				LOG_DBG("CMD_DUO_LED(%d): recv param_ptr=0x%x", cmd.cmd_id,
					cmd.param_ptr);
				if (cmd.param_ptr == DUO_LED_ON) {
					gpio_pin_set_dt(&led, 1);
				} else {
					gpio_pin_set_dt(&led, 0);
				}
				cmd.param_ptr = DUO_LED_DONE;
				cmd.resv.valid.rtos = 1;
				cmd.resv.valid.linux = 0;
				LOG_DBG("CMD_DUO_LED(%d): send param_ptr=0x%x", cmd.cmd_id,
					cmd.param_ptr);
				goto send_label;
			default:
			send_label:
				mbox_msg.data = &cmd;
				mbox_msg.size = sizeof(struct cmd);
				ret = mbox_send_dt(&tx_channel, &mbox_msg);
				if (ret < 0) {
					LOG_ERR("Could not send message (%d)", ret);
					return 0;
				}
				LOG_DBG("Responsed via channel %d", tx_channel.channel_id);
				break;
			}
		}
	}

	return 0;
}
