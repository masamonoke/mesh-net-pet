#include "format_app.h"

#include "control_utils.h"
#include "settings.h"

#include <memory.h>

void format_app_create_message(const struct app_payload* app_payload, uint8_t* p) {
	memcpy(p, &app_payload->req_type, sizeof_enum(app_payload->req_type));
	p += sizeof_enum(app_payload->req_type);
	memcpy(p, &app_payload->addr_from, sizeof(app_payload->addr_from));
	p += sizeof(app_payload->addr_from);
	memcpy(p, &app_payload->addr_to, sizeof(app_payload->addr_to));
	p += sizeof(app_payload->addr_to);
	memcpy(p, &app_payload->key, sizeof(app_payload->key));
	p += sizeof(app_payload->key);

	memcpy(p, &app_payload->message_len, sizeof(app_payload->message_len));
	p += sizeof(app_payload->message_len);
	if (app_payload->message_len) {
		memcpy(p, app_payload->message, app_payload->message_len);
	}
}

void format_app_parse_message(void* payload, const uint8_t* p) {
	struct app_payload* app_payload;

	app_payload = (struct app_payload*) payload;

	memcpy(&app_payload->req_type, p, sizeof_enum(app_payload->req_type));
	p += sizeof_enum(app_payload->req_type);
	memcpy(&app_payload->addr_from, p, sizeof(app_payload->addr_from));
	p += sizeof(app_payload->addr_from);
	memcpy(&app_payload->addr_to, p, sizeof(app_payload->addr_to));
	p += sizeof(app_payload->addr_to);
	memcpy(&app_payload->key, p, sizeof(app_payload->key));
	p += sizeof(app_payload->key);

	memcpy(&app_payload->message_len, p, sizeof(app_payload->message_len));
	p += sizeof(app_payload->message_len);
	if (app_payload->message_len) {
		memcpy(app_payload->message, p, app_payload->message_len);
	}
}

uint32_t format_app_message_len(struct app_payload* payload) {
	return sizeof(payload->addr_from) + sizeof(payload->addr_to) + sizeof(payload->key) +
		sizeof(payload->message_len) + payload->message_len + sizeof_enum(app_payload->req_type);
}
