#include "id.h"

static uint32_t message_id = 1;

uint32_t id_generate(void) {
	uint32_t id;

	id = message_id++;

	if (message_id == 0) {
		message_id++;
	}

	return id;
}
