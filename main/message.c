/***** Includes *****/

#include "system.h"
#include "message.h"
#include "memory.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "peep.pb.h"

_Static_assert(
	(sizeof(ClientMessage) + sizeof(DeviceMessage)) < 4096,
	"ClientMessage/DeviceMessage size check");

_Static_assert(
	(sizeof(pb_size_t) == sizeof(uint32_t)),
	"pb_size_t size check");

/***** Local Data *****/

// TODO: mutex
static ClientMessage * _cmsg = NULL;
static DeviceMessage * _dmsg = NULL;
static bool _has_device_message = false;

/***** Local Functions *****/

static bool
_handle_client_command(void)
{
	int32_t len = 0;
	bool r = true;

	*_dmsg = (DeviceMessage) DeviceMessage_init_default;

	switch (_cmsg->command.type) {
	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_DEVICE_CERT):
		len = memory_set_item(
			MEMORY_ITEM_DEV_CERT,
			_cmsg->command.payload.bytes,
			_cmsg->command.payload.size);

		if (0 >= len) r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_PRIVATE_KEY):
		len = memory_set_item(
			MEMORY_ITEM_DEV_PRIV_KEY,
			_cmsg->command.payload.bytes,
			_cmsg->command.payload.size);

		if (0 >= len) r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_ROOT_CA):
		len = memory_set_item(
			MEMORY_ITEM_ROOT_CA,
			_cmsg->command.payload.bytes,
			_cmsg->command.payload.size);

		if (0 >= len) r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_TIME):
		// TODO: set the system time to the provided unix epoch
		r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_PASS):
		len = memory_set_item(
			MEMORY_ITEM_WIFI_PASS,
			_cmsg->command.payload.bytes,
			_cmsg->command.payload.size);

		if (0 >= len) r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_SSID):
		len = memory_set_item(
			MEMORY_ITEM_WIFI_SSID,
			_cmsg->command.payload.bytes,
			_cmsg->command.payload.size);

		if (0 >= len) r = false;
		break;

	case (ClientCommandType_CLIENT_COMMAND_CONFIGURE_HATCH):
		// TODO: Protobuf code for this needs some thought...
		r = false;
		break;

	default:
		r = false;
		break;
	}

	_dmsg->id = _cmsg->id;
	_dmsg->type = DeviceMessageType_DEVICE_MESSAGE_COMMAND_RESULT;
	_dmsg->command_result.type = _cmsg->command.type;
	if (r) {
		_dmsg->command_result.result = DeviceResult_DEVICE_RESULT_SUCCESS;
	}
	else {
		_dmsg->command_result.result = DeviceResult_DEVICE_RESULT_ERROR;
	}
	_has_device_message = true;

	// we have a valid message to send, set to true
	return true;
}

static bool
_handle_client_query(void)
{
	bool r = true;

	*_dmsg = (DeviceMessage) DeviceMessage_init_default;

	switch (_cmsg->query.type) {
	case (ClientQueryType_CLIENT_QUERY_ENCODING_VERSION):
		_dmsg->query_result.version.major_version = MajorVersion_MAJOR_VERSION;
		_dmsg->query_result.version.minor_version = MinorVersion_MINOR_VERSION;
		_dmsg->query_result.version.patch_version = PatchVersion_PATCH_VERSION;
		break;

	case (ClientQueryType_CLIENT_QUERY_UNKNOWN):
	default:
		r = false;
	}

	_dmsg->id = _cmsg->id;
	_dmsg->type = DeviceMessageType_DEVICE_MESSAGE_QUERY_RESULT;
	if (r) {
		_dmsg->query_result.result = DeviceResult_DEVICE_RESULT_SUCCESS;
	}
	else {
		_dmsg->query_result.result = DeviceResult_DEVICE_RESULT_ERROR;
	}
	_has_device_message = true;

	// we have a valid message to send, set to true
	return true;
}

/***** Global Functions *****/

bool
message_init(uint8_t * buffer, uint32_t length)
{
	bool r = true;

	if (length < 4096) {
		r = false;
	}

	if (r) {
		// This is valid due to check on "length" and the _Static_assert at the
		// top of this file.
		uint32_t offset = sizeof(ClientMessage);
		_cmsg = (ClientMessage *) (buffer + 0);
		_dmsg = (DeviceMessage *) (buffer + offset);
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
		istream = pb_istream_from_buffer(message, length);
		r = pb_decode_delimited(&istream, ClientMessage_fields, _cmsg);
	}

	if (r) {
		switch (_cmsg->type) {
		case (ClientMessageType_CLIENT_MESSAGE_COMMAND):
			r = _handle_client_command();
			break;

		case (ClientMessageType_CLIENT_MESSAGE_QUERY):
			r = _handle_client_query();
			break;

		default:
			r = false;
		}
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
		r = pb_encode_delimited(&ostream, DeviceMessage_fields, _dmsg);
		*length = ostream.bytes_written;
		_has_device_message = false;
	}
	else {
		r = false;
	}

	return r; 
}
