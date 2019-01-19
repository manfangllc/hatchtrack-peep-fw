#define _GNU_SOURCE

#include <sys/select.h>
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <argp.h>
#include <time.h>
#include <assert.h>

#include "pb_encode.h"
#include "pb_decode.h"
#include "peep.pb.h"

/***** Defines *****/

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0
#define APP_VERSION_STRING \
	STR(APP_VERSION_MAJOR) "." STR(APP_VERSION_MINOR) "." STR(APP_VERSION_PATCH)

#define PORT_NAME "/dev/ttyUSB0"
#define BUF_LEN_MAX 100000

#define PAYLOAD_MAX 2048

/***** Macros *****/

// Stores uint32_t into buffer big endian encoding.
#define UINT32_TO_PAYLOAD(buf, u32) \
	do { \
		(buf)[0] = (u32) >> 24; \
		(buf)[1] = (u32) >> 16; \
		(buf)[2] = (u32) >> 8; \
		(buf)[3] = (u32) >> 0; \
	} while (0)

/***** Global Data *****/

const char *argp_program_version = "version " APP_VERSION_STRING;
const char *argp_program_bug_address = "mreutman@widgt.ninja";

/***** Local Data *****/

static char _doc[] = "Configures Peep over UART connection.";
static struct argp_option _options[] = {
	{"wifi-ssid", 'w', "SSID", 0, "Wifi SSID to connect Peep to.", 0},
	{"wifi-pass", 'p', "PASSWORD", 0, "Wifi password for connection.", 0},
	{"root-ca", 'r', "FILE", 0, "Root certificate authority file.", 0},
	{"dev-cert", 'c', "FILE", 0, "Device certificate file.", 0},
	{"dev-key", 'k', "FILE", 0, "Device private key file.", 0},
	{"uuid", 'u', "ID", 0, "Universally unique identifier for Peep.", 0},
	{"encoding", 'e', 0, 0, "Obtain Protobuf encoding version.", 0},
	{"samples", 's', "NUMBER", 0, "Total number of samples for hatch.", 0},
	{"time", 't', "SECONDS", 0, "Measurement period time in seconds.", 0},
};

static int _fd = -1;
static uint8_t _buf[BUF_LEN_MAX];
static uint32_t _buf_len = 0;
static ClientMessage _c = ClientMessage_init_default;
static DeviceMessage _d = DeviceMessage_init_default;

/***** Tests *****/

_Static_assert(
	(sizeof(_c.command.payload.bytes) == PAYLOAD_MAX),
	"size check");

/***** Local Functions *****/

static bool
_uart_init(void)
{
	struct termios tty;

	_fd = open(PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
	if (_fd < 0) {
		printf("Error opening %s: %s\n", PORT_NAME, strerror(errno));
		return false;
	}

	if (tcgetattr(_fd, &tty) < 0) {
		printf("Error from tcgetattr: %s\n", strerror(errno));
		return false;
	}

	/* baudrate 115200, 8 bits, no parity, 1 stop bit */
	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &=
		~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(_fd, TCSANOW, &tty) != 0) {
		printf("Error from tcsetattr: %s\n", strerror(errno));
		return false;
	}

	return true;
}

static bool
_uart_write_cmsg(ClientMessage * cmsg)
{
	pb_ostream_t stream;
	bool r = true;
	int len = 0;

	if (r) {
		stream = pb_ostream_from_buffer(_buf, BUF_LEN_MAX);
	}

	if (r) {
		r = pb_encode_delimited(&stream, ClientMessage_fields, cmsg);
	}

	if (r) {
		_buf_len = stream.bytes_written;
		len = write(_fd, _buf, _buf_len);
		if (len != stream.bytes_written) {
			r = false;
		}
		else {
			printf("wrote %d of %d bytes\n", len, _buf_len);
		}
	}

	tcdrain(_fd); // output delay

	return r;
}

static bool
_uart_read_dmsg(DeviceMessage * dmsg)
{
	pb_istream_t stream;
	bool r = true;
	bool done = false;
	int len = 0;

	_buf_len = 0;

	while ((r) && (!done)) {
		struct timeval t;
		int retval = 0;
		fd_set fds;

		t.tv_sec = 1;
		t.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(_fd, &fds);

		retval = select(_fd + 1, &fds, NULL, NULL, &t);
		if (-1 == retval) {
			printf("error: %s\n", strerror(errno));
		}
		else if (retval) {
			len = read(_fd, _buf + _buf_len, BUF_LEN_MAX - _buf_len);
			if (len > 0) {
				printf("read %d bytes\n", len);
				_buf_len += len;
				stream = pb_istream_from_buffer(_buf, _buf_len);
				r = pb_decode_delimited(&stream, DeviceMessage_fields, dmsg);
				if (r) {
					done = true;
				}
				else {
					printf("debug:\n");
					uint32_t j = 0;
					for (j = 0; j < _buf_len; j++) {
						if (isprint(_buf[j])) {
							printf("%c", _buf[j]);
						}
					}
					printf("\n");
					done = false;
					r = true;
				}
			}
		}
	}

	return r;
}

static int
_parse_opt(int key, char *arg, struct argp_state *state)
{
	FILE * fp = NULL;
	static uint32_t n = 0;
	static bool run_once = true;
	uint32_t val = 0;
	bool r = true;

	_c = (ClientMessage) ClientMessage_init_default;
	_d = (DeviceMessage) DeviceMessage_init_default;
	_c.id = n + 1;

	switch (key) {
	case ('e'):
		printf("[%ld]: read eoncoding version\n", time(NULL));
		_c.type = ClientMessageType_CLIENT_MESSAGE_QUERY;
		_c.query.type = ClientQueryType_CLIENT_QUERY_ENCODING_VERSION;
		break;

	case ('u'):
		printf("[%ld]: uuid %s\n", time(NULL), arg);
		_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
		_c.command.type = ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_DEVICE_UUID;
		_c.command.payload.size = strlen(arg);
		strcpy((char *) _c.command.payload.bytes, arg);
		break;

	case ('w'):
		printf("[%ld]: wifi ssid %s\n", time(NULL), arg);
		_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
		_c.command.type = ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_SSID;
		_c.command.payload.size = strlen(arg);
		strcpy((char *) _c.command.payload.bytes, arg);
		break;

	case ('p'):
		printf("[%ld]: wifi password %s\n", time(NULL), arg);
		_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
		_c.command.type = ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_PASS;
		_c.command.payload.size = strlen(arg);
		strcpy((char *) _c.command.payload.bytes, arg);
		break;

	case ('r'):
		printf("[%ld]: root ca %s\n", time(NULL), arg);
		fp = fopen(arg, "rb");
		if (fp) {
			_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
			_c.command.type =
				ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_ROOT_CA;
			_c.command.payload.size = fread(
				_c.command.payload.bytes,
				1,
				PAYLOAD_MAX,
				fp);
			fclose(fp);
		}
		break;

	case ('c'):
		printf("[%ld]: device certificate %s\n", time(NULL), arg);
		fp = fopen(arg, "rb");
		if (fp) {
			_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
			_c.command.type =
				ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_DEVICE_CERT;
			_c.command.payload.size = fread(
				_c.command.payload.bytes,
				1,
				PAYLOAD_MAX,
				fp);
			fclose(fp);
		}
		break;

	case ('k'):
		printf("[%ld]: device private key %s\n", time(NULL), arg);
		fp = fopen(arg, "rb");
		if (fp) {
			_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
			_c.command.type =
				ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_PRIVATE_KEY;
			_c.command.payload.size = fread(
				_c.command.payload.bytes,
				1,
				PAYLOAD_MAX,
				fp);
			fclose(fp);
		}
		break;

	case ('s'):
		printf("[%ld]: measurement samples %s\n", time(NULL), arg);
		_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
		_c.command.type =
			ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_MEASURE_TOTAL;
		val = (uint32_t) strtod(arg, NULL);
		UINT32_TO_PAYLOAD(_c.command.payload.bytes, val);
		break;

	case ('t'):
		printf("[%ld]: measurement period time seconds %s\n", time(NULL), arg);
		_c.type = ClientMessageType_CLIENT_MESSAGE_COMMAND;
		_c.command.type =
			ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_MEASURE_DELAY_SEC;
		val = (uint32_t) strtod(arg, NULL);
		UINT32_TO_PAYLOAD(_c.command.payload.bytes, val);
		break;

	default:
		r = false;
	}

	if ((run_once) && (r)) {
		r = _uart_init();
		run_once = false;
	}

	if (r) {
		r = _uart_write_cmsg(&_c);
	}

	if (r) {
		r = _uart_read_dmsg(&_d);
	}

	if (r) {
		switch (key) {
		case ('e'):
			if (DeviceResult_DEVICE_RESULT_SUCCESS == _d.query_result.result) {
				printf(
					"[%ld]: encoding version %d.%d.%d\n",
					time(NULL),
					_d.query_result.version.major_version,
					_d.query_result.version.minor_version,
					_d.query_result.version.patch_version);
			}
			else {
				printf("[%ld]: error (%d)\n", time(NULL), _d.query_result.result);
			}
			break;

		case ('c'):
		case ('k'):
		case ('p'):
		case ('r'):
		case ('w'):
		case ('u'):
			if (DeviceResult_DEVICE_RESULT_SUCCESS == _d.command_result.result) {
				printf("[%ld]: success\n", time(NULL));
			}
			else {
				printf("[%ld]: error (%d)\n", time(NULL), _d.command_result.result);
			}
			break;
		}
	}

	return 0;
}

/***** Global Functions *****/

int
main(int argc, char * argv[])
{
	struct argp argp = {_options, _parse_opt, 0, _doc, 0, 0, 0};
	argp_parse(&argp, argc, argv, 0, 0, 0);

	return 0;
}
