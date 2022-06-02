/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if (CONFIG_AUDIO_DEV == HEADSET)
#include "le_audio.h"

#include <bluetooth/audio/audio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>

#include "macros_common.h"
#include "ctrl_events.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cis_headset, CONFIG_LOG_BLE_LEVEL);

#define CHANNEL_COUNT_1 BIT(0)
#define BLE_ISO_LATENCY_MS 10
#define BLE_ISO_RETRANSMITS 2
#define BLE_ISO_PRSENTATION_DELAY_MIN_US 10000
#define BLE_ISO_PRSENTATION_DELAY_MAX_US 40000
#define BLE_ISO_PREF_PRSENTATION_DELAY_MIN_US 10000
#define BLE_ISO_PREF_PRSENTATION_DELAY_MAX_US 40000
#define DEVICE_NAME_PEER "NRF5340_AUDIO"
#define DEVICE_NAME_PEER_LEN (sizeof(DEVICE_NAME_PEER) - 1)

static le_audio_receive_cb receive_cb;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME_PEER, DEVICE_NAME_PEER_LEN),
};
static struct bt_audio_capability_ops lc3_cap_codec_ops;
static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10, CHANNEL_COUNT_1, 40u, 120u,
		     1u, (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA),
		     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_capability caps = {
	.type = BT_AUDIO_SINK,
	.pref = BT_AUDIO_CAPABILITY_PREF(
		BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED, BT_GAP_LE_PHY_2M, BLE_ISO_RETRANSMITS,
		BLE_ISO_LATENCY_MS, BLE_ISO_PRSENTATION_DELAY_MIN_US,
		BLE_ISO_PRSENTATION_DELAY_MAX_US, BLE_ISO_PREF_PRSENTATION_DELAY_MIN_US,
		BLE_ISO_PREF_PRSENTATION_DELAY_MAX_US),
	.codec = &lc3_codec,
	.ops = &lc3_cap_codec_ops,
};
static struct bt_conn *default_conn;
static struct bt_audio_stream streams;

static struct bt_audio_stream *lc3_cap_config_cb(struct bt_conn *conn, struct bt_audio_ep *ep,
						 enum bt_audio_pac_type type,
						 struct bt_audio_capability *cap,
						 struct bt_codec *codec)
{
	struct bt_audio_stream *stream = &streams;

	if (!stream->conn) {
		LOG_INF("ASE Codec Config stream %p", (void *)stream);
		return stream;
	}

	LOG_WRN("No streams available");
	return NULL;
}

static int lc3_cap_reconfig_cb(struct bt_audio_stream *stream, struct bt_audio_capability *cap,
			       struct bt_codec *codec)
{
	LOG_INF("ASE Codec Reconfig: stream %p cap %p", (void *)stream, (void *)cap);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_cap_qos_cb(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	LOG_INF("QoS: stream %p qos %p", (void *)stream, (void *)qos);

	return 0;
}

static int lc3_cap_enable_cb(struct bt_audio_stream *stream, struct bt_codec_data *meta,
			     size_t meta_count)
{
	int ret;
	struct event_t event;

	LOG_INF("Enable: stream %p meta_count %u", (void *)stream, meta_count);
	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);
	return 0;
}

static int lc3_cap_start_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Start: stream %p", (void *)stream);
	return 0;
}

static int lc3_cap_metadata_cb(struct bt_audio_stream *stream, struct bt_codec_data *meta,
			       size_t meta_count)
{
	LOG_INF("Metadata: stream %p meta_count %u", (void *)stream, meta_count);
	return 0;
}

static int lc3_cap_disable_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Disable: stream %p", (void *)stream);
	return 0;
}

static int lc3_cap_stop_cb(struct bt_audio_stream *stream)
{
	int ret;
	struct event_t event;

	LOG_INF("Stop: stream %p", (void *)stream);

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_NOT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);

	return 0;
}

static int lc3_cap_release_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Release: stream %p", (void *)stream);
	return 0;
}
static struct bt_audio_capability_ops lc3_cap_codec_ops = {
	.config = lc3_cap_config_cb,
	.reconfig = lc3_cap_reconfig_cb,
	.qos = lc3_cap_qos_cb,
	.enable = lc3_cap_enable_cb,
	.start = lc3_cap_start_cb,
	.metadata = lc3_cap_metadata_cb,
	.disable = lc3_cap_disable_cb,
	.stop = lc3_cap_stop_cb,
	.release = lc3_cap_release_cb,
};

static void stream_recv_cb(struct bt_audio_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	static uint32_t recv_cnt;
	bool bad_frame = false;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts);

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		LOG_INF("Received %u total ISO packets", recv_cnt);
	}
}

static void stream_stop_cb(struct bt_audio_stream *stream)
{
	int ret;
	struct event_t event;

	LOG_INF("Stop: stream %p", (void *)stream);

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_NOT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		default_conn = NULL;
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);
	default_conn = bt_conn_ref(conn);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		LOG_WRN("Disconnected on wrong conn");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static struct bt_audio_stream_ops stream_ops = { .recv = stream_recv_cb,
						 .stopped = stream_stop_cb };

static void initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (!initialized) {
		receive_cb = recv_cb;
		ret = bt_audio_capability_register(&caps);
		ERR_CHK_MSG(ret, "Capability register failed");
		ret = bt_audio_capability_set_location(BT_AUDIO_SINK, BT_AUDIO_LOCATION_SIDE_LEFT);
		ERR_CHK_MSG(ret, "Location set failed");

		bt_audio_stream_cb_register(&streams, &stream_ops);
		initialized = true;
	}
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	return 0;
}

int le_audio_volume_up(void)
{
	return 0;
}

int le_audio_volume_down(void)
{
	return 0;
}

int le_audio_volume_mute(void)
{
	return 0;
}

int le_audio_play(void)
{
	return 0;
}

int le_audio_pause(void)
{
	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	return 0;
}

void le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	initialize(recv_cb);
	ret = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (ret) {
		LOG_INF("Advertising failed to start: %d", ret);
		return;
	}

	LOG_INF("Advertising successfully started\n");
}

void le_audio_disable(void)
{
}

#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
