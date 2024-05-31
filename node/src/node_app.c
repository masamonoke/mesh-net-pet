#include "node_app.h"

#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include <zlib.h>

#include "node_essentials.h"
#include "settings.h"
#include "control_utils.h"
#include "format_client_server.h"
#include "crc.h"

static uint8_t msg_storage[APP_MESSAGE_LEN];
static uint8_t storage_msg_len = 0;
static uint16_t crc;

void node_app_fill_default(app_t apps[APPS_COUNT], uint8_t node_addr) {
	uint8_t i;
	uint8_t j;

	srandom((uint32_t) time(NULL));

	for (i = 0; i < APPS_COUNT; i++) {
		apps[i].app_addr = i;
		apps[i].node_addr = node_addr;
		for (j = 0; j < MAX_PAIR; j++) {
			apps[i].pairs[j].exchanged = false;
		}
	}
}

__attribute__((warn_unused_result))
static bool get_key(const app_t* app, uint8_t app_addr, uint8_t node_addr, uint8_t* key);

static void compress_message(uint8_t* msg, uint8_t* msg_len);

static void decompress_message(uint8_t* msg, uint8_t* msg_len);

// TODO: mock, replace with actual impl
static void decrypt_app_message(uint8_t key, uint8_t* msg, uint8_t msg_len) {
	(void) msg;
	(void) msg_len;
	node_log_debug("Decrypting message with key %d", key);
}

// TODO: mock, replace with actual impl
static void encrypt_app_message(uint8_t key, uint8_t* msg, uint8_t msg_len) {
	(void) msg;
	(void) msg_len;
	node_log_debug("Encrypting message with key %d", key);
}

bool node_app_handle_request(app_t* apps, struct app_payload* app_payload, uint8_t node_addr_from) {
	size_t i;

	switch (app_payload->req_type) {
		case APP_REQUEST_DELIVERY:
			for (i = 0; i < APPS_COUNT; i++) {
				if (apps[i].app_addr == app_payload->addr_to) {
					uint8_t key;

					if (app_payload->message_len) {

						if (get_key(&apps[i], app_payload->addr_from, node_addr_from, &key)) {
							uint16_t calc_crc;
							if (app_payload->message_len != 0) {
								decompress_message(app_payload->message, &app_payload->message_len);
								decrypt_app_message(key, app_payload->message, app_payload->message_len);

								calc_crc = crc16(app_payload->message, app_payload->message_len);
								if (calc_crc != app_payload->crc) {
									node_log_error("App message damaged: got CRC %d, calculated %d", app_payload->crc, calc_crc);
									return false;
								}
							}
							node_log_info("App %d got message (length %d) from app %d:%d: %.*s",
								app_payload->addr_to, app_payload->message_len, node_addr_from, app_payload->addr_from, app_payload->message_len, app_payload->message);
						}
					} else {
						node_log_info("App %d got empty message from app %d", app_payload->addr_from, app_payload->addr_to);
					}
					return true;
				}
			}
			break;
		case APP_REQUEST_KEY_EXCHANGE:
				app_payload->req_type = APP_REQUEST_EXCHANGED_KEY;
				return node_app_save_key(apps, app_payload, node_addr_from);
			break;
		default:
			node_log_error("Unexpected app request: %d", app_payload->req_type);
			break;
	}

	return false;
}

static bool has_pair(app_t* app, uint8_t app_addr, uint8_t node_addr) {
	size_t i;

	for (i = 0; i < MAX_PAIR; i++) {
		if (app->pairs[i].exchanged && app->pairs[i].app_addr == app_addr && app->pairs[i].node_addr == node_addr) {
			return true;
		}
	}

	return false;
}

static bool get_app(const app_t* apps, uint8_t app_addr, app_t* app) {
	size_t i;

	for (i = 0; i < APPS_COUNT; i++) {
		if (apps[i].app_addr == app_addr) {
			*app = apps[i];
			return true;
		}
	}

	return false;
}

__attribute__((warn_unused_result))
static bool set_app(app_t* apps, app_t* app) {
	size_t i;

	for (i = 0; i < APPS_COUNT; i++) {
		if (apps[i].app_addr == app->app_addr) {
			apps[i].node_addr = app->node_addr;
			memcpy(apps[i].pairs, app->pairs, sizeof(struct pair) * MAX_PAIR);
			return true;
		}
	}

	return false;
}

__attribute__((warn_unused_result))
static bool save_pair(app_t* app, uint8_t app_addr, uint8_t node_addr, uint8_t key) { // NOLINT
	size_t i;

	for (i = 0; i < MAX_PAIR; i++) {
		if (!app->pairs[i].exchanged) {
			app->pairs[i].exchanged = true;
			app->pairs[i].app_addr = app_addr;
			app->pairs[i].node_addr = node_addr;
			app->pairs[i].key = key;
			return true;
		}
	}

	return false;
}

bool node_app_save_key(app_t apps[APPS_COUNT], struct app_payload* app_payload, uint8_t addr_from) {
	app_t app;

	if (!get_app(apps, app_payload->addr_to, &app)) {
		node_log_error("Failed to get app %d", app_payload->addr_from);
		return false;
	}

	if (!has_pair(&app, app_payload->addr_from, addr_from)) {
		if (!save_pair(&app, app_payload->addr_from, addr_from, app_payload->key)) {
			node_log_error("Failed to save key for pair");
			return false;
		}
		if (!set_app(apps, &app)) {
			node_log_error("Failed to save key entry");
			return false;
		}
	}

	return true;
}

void node_app_setup_delivery(app_t apps[APPS_COUNT], struct app_payload* app_payload, uint8_t addr_to) {
	app_t app;

	if (!get_app(apps, app_payload->addr_from, &app)) {
		node_log_error("Failed to get app %d", app_payload->addr_from);
	}

	if (!has_pair(&app, app_payload->addr_to, addr_to)) {
		node_log_warn("Keys not exchanged with %d:%d",
			addr_to, app_payload->addr_to);

		memcpy(msg_storage, app_payload->message, app_payload->message_len);
		storage_msg_len = app_payload->message_len;
		crc = app_payload->crc;

		app_payload->req_type = APP_REQUEST_KEY_EXCHANGE;
		app_payload->message_len = 0;
		app_payload->key = (uint8_t) (random() % UINT8_MAX);
		node_log_debug("Generated key %d", app_payload->key);
	} else {
		uint8_t key;

		node_log_debug("Found Keys between app %d (current) and %d:%d, app req %d",
			app_payload->addr_from, addr_to, app_payload->addr_to, app_payload->req_type);

		if (storage_msg_len != 0) {
			memcpy(app_payload->message, msg_storage, storage_msg_len);
			app_payload->message_len = storage_msg_len;
			app_payload->crc = crc;
			storage_msg_len = 0;
		}

		if (get_key(&app, app_payload->addr_to, addr_to, &key)) {
			if (app_payload->message_len) {
				uint8_t before_compress_len;
				before_compress_len = app_payload->message_len;
				// little messages can become bigger after compress but for big ones works fine
				compress_message(app_payload->message, &app_payload->message_len);
				custom_log_debug("Before compress: %d, after %d", before_compress_len, app_payload->message_len);
				encrypt_app_message(key, app_payload->message, app_payload->message_len);
			}
		}
	}
}

static bool get_key(const app_t* app, uint8_t app_addr, uint8_t node_addr, uint8_t* key) {
	size_t i ;

	for (i = 0; i < MAX_PAIR; i++) {
		if (app->pairs[i].exchanged && app->pairs[i].app_addr == app_addr && app->pairs[i].node_addr == node_addr) {
			*key = app->pairs[i].key;
			return true;
		}
	}

	return false;
}

static void compress_message(uint8_t* msg, uint8_t* msg_len) { // NOLINT
	z_stream defstream;
	char b[APP_MESSAGE_LEN];

	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;
	defstream.avail_in = (uInt) *msg_len;
	defstream.next_in = (Bytef *) msg;
	defstream.avail_out = (uInt) sizeof(b);
	defstream.next_out = (Bytef *)b;

	deflateInit(&defstream, Z_DEFAULT_COMPRESSION);
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);

	*msg_len = (uint8_t) ((char*)defstream.next_out - b);
	memcpy(msg, b, APP_MESSAGE_LEN);
}

static void decompress_message(uint8_t* msg, uint8_t* msg_len) { // NOLINT
	z_stream infstream;
	char b[APP_MESSAGE_LEN];

	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	infstream.avail_in = (uInt) (*msg_len);
	infstream.next_in = (Bytef*) msg;
	infstream.avail_out = (uInt) sizeof(b);
	infstream.next_out = (Bytef*) b;

	inflateInit(&infstream);
	inflate(&infstream, Z_NO_FLUSH);
	inflateEnd(&infstream);

	*msg_len = (uint8_t) infstream.total_out;
	memcpy(msg, b, APP_MESSAGE_LEN);
}
