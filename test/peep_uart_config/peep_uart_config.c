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

#include "pb_encode.h"
#include "pb_decode.h"
#include "peep.pb.h"

/***** Defines *****/

#define PORT_NAME "/dev/ttyUSB0"
#define BUF_LEN_MAX 10000

#define str(x) #x
#define xstr(x) str(x)

//#define DEVICETYPE_STR(e) \
	//((e) == DeviceMsgType_ACK_DMSG) ? "ACK" : \
	//((e) == DeviceMsgType_COMMAND_RESULT_DMSG) ? "COMMAND" : \
	//((e) == DeviceMsgType_INQUIRY_RESULT_DMSG) ? "INQUIRY" : \
	//"UNKNOWN"

/***** Local Data *****/

static int _fd = -1;
static uint8_t _buf[BUF_LEN_MAX];
static uint32_t _buf_len = 0;
static ClientMessage _cmsg = ClientMessage_init_default;
static DeviceMessage _dmsg = DeviceMessage_init_default;

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
			printf("wrote %d bytes\n", len);
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
	uint32_t n = 0;
	_buf_len = 0;


	if (r) {
		bool done = false;
		int len = 0;
		_buf_len = 0;

		while (!done) {
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
						uint32_t j = 0;
						for (j = 0; j < _buf_len; j++) {
							if (isprint(_buf[j])) {
								printf("%c", _buf[j]);
							}
						}
						done = false;
						r = true;
					}
				}
			}
		}

		if (!r) {
			printf("error (%d): %s\n", __LINE__, stream.errmsg);
		}
		else if ((r) && (0 == n)) {

		}
	}

	return r;
}

static bool
_query_encoding_version(ClientMessage * cmsg)
{
	*cmsg = (ClientMessage) ClientMessage_init_default;
	cmsg->id = 1;
	cmsg->type = ClientMessageType_CLIENT_MESSAGE_QUERY;
	
}

/***** Global Functions *****/

int
main(int argc, char * argv[])
{
	bool r = true;

	if (r) {
		r = _uart_init();
	}


	return (r) ? 0 : 1;
}
