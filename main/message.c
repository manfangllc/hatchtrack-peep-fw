/***** Includes *****/

#include "message.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "peep.pb.h"

// MREUTMAN: Might want to move this elsewhere, maybe peep_compile_checks.h?
_Static_assert(sizeof(ClientMessage) < 2048, "ClientMessage size check");
_Static_assert(sizeof(ClientMessage) < 2048, "D size check");

/***** Local Data *****/

static ClientMessage * _cmsg = NULL;
static DeviceMessage * _dmsg = NULL;
static bool _has_device_message = false;

/***** Global Functions *****/

bool
message_init(uint8_t * buffer, uint32_t length)
{
	bool r = true;

	if (length < 4096) {
		r = false;
	}

	if (r) {
		_cmsg = (ClientMessage *) (buffer + 0);
		_dmsg = (DeviceMessage *) (buffer + 2048);
		_has_device_message = false;
	}

	return r;
}

extern void
message_free(void)
{
	_cmsg = NULL;
	_dmsg = NULL;
}

bool
message_client_write(uint8_t * message, uint32_t length)
{
	pb_istream_t istream;
	bool r = true;

	if ((NULL == _cmsg) || (NULL == _dmsg)) {
		r = false;
	}

	if (r) {
		*_cmsg = (ClientMessage) ClientMessage_init_default;
		*_dmsg = (DeviceMessage) DeviceMessage_init_default;
		istream = pb_istream_from_buffer(message, length);
		r = pb_decode_delimited(&istream, ClientMessage_fields, _cmsg);
	}

	return r;
}

bool
message_device_response(uint8_t * message, uint32_t * length,
uint32_t max_length)
{
	pb_ostream_t ostream;
	bool r = true;

	if (_has_device_message) {
		ostream = pb_ostream_from_buffer(message, max_length);
		r = pb_encode_delimited(&ostream, DeviceMessage_fields, &_dmsg);
		*length = ostream.bytes_written;
		_has_device_message = false;
	}
	else {
		r = false;
	}

	return r; 
}
