#include "id.h"

static uint16_t message_id = 1;

uint16_t id_generate(void) {

	message_id++;

	if (message_id == 0) {
		message_id++;
	}

	return message_id;
}
