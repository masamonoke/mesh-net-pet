#include "node_app.h"

#include <memory.h>
#include <stdlib.h>
#include <time.h>

#include <zlib.h>

#include "node_essentials.h"
#include "settings.h"
#include "crc.h"


void node_app_fill_default(app_t apps[APPS_COUNT], uint8_t node_addr) {
	uint8_t i;

	srandom((uint32_t) time(NULL));

	for (i = 0; i < APPS_COUNT; i++) {
		apps[i].app_addr = i;
		apps[i].node_addr = node_addr;
	}
}

__attribute__((nonnull(1, 2)))
static void compress_message(uint8_t* msg, uint8_t* msg_len);

__attribute__((nonnull(1, 2)))
static void decompress_message(uint8_t* msg, uint8_t* msg_len);

bool node_app_handle_request(app_t* apps, struct app_payload* app_payload, uint8_t node_addr) {
	size_t i;

	switch (app_payload->req_type) {
		case APP_REQUEST_DELIVERY:
			for (i = 0; i < APPS_COUNT; i++) {
				if (apps[i].app_addr == app_payload->addr_to) {

					if (app_payload->message_len) {
						uint16_t calc_crc;

						if (app_payload->message_len != 0) {
							decompress_message(app_payload->message, &app_payload->message_len);
							calc_crc = app_crc(app_payload);
							if (calc_crc != app_payload->crc) {
								node_log_error("App message damaged: got CRC %d, calculated %d. Message: %.*s",
									app_payload->crc, calc_crc, app_payload->message_len, app_payload->message);
								return false;
							}
						}
						node_log_info("App got message (length %d, id %d): %.*s",
							app_payload->message_len, app_payload->id, app_payload->message_len, app_payload->message);
					} else {
						node_log_info("App (id %d, node %d) got empty message from app %d", app_payload->id, node_addr, app_payload->addr_to);
					}
					return true;
				}
			}
			break;
		case APP_REQUEST_BROADCAST:
		case APP_REQUEST_UNICAST:
			if (app_payload->message_len != 0) {
				uint16_t calc_crc;

				decompress_message(app_payload->message, &app_payload->message_len);
				calc_crc = app_crc(app_payload);
				if (calc_crc != app_payload->crc) {
					node_log_error("App message damaged: got CRC %d, calculated %d", app_payload->crc, calc_crc);
					return false;
				}
			}
			node_log_info("App (id %d) got broadcast message (length %d): %.*s",
				app_payload->id, app_payload->message_len, app_payload->message_len, app_payload->message);
			return true;
			break;
		default:
			node_log_error("Unexpected app request: %d", app_payload->req_type);
			break;
	}

	return false;
}

void node_app_setup_delivery(struct app_payload* app_payload) {
	if (app_payload->message_len) {
		uint8_t before_compress_len;

		before_compress_len = app_payload->message_len;
		// little messages can become bigger after compress but for big ones works fine
		compress_message(app_payload->message, &app_payload->message_len);
		custom_log_debug("Before compress: %d, after %d", before_compress_len, app_payload->message_len);
	}
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
