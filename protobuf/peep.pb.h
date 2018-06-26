/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.0-dev at Sun Jun 24 14:58:30 2018. */

#ifndef PB_PEEP_PB_H_INCLUDED
#define PB_PEEP_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _MajorVersion {
    MajorVersion_MAJOR_VERSION_UNKNOWN = 0,
    MajorVersion_MAJOR_VERSION = 1
} MajorVersion;
#define _MajorVersion_MIN MajorVersion_MAJOR_VERSION_UNKNOWN
#define _MajorVersion_MAX MajorVersion_MAJOR_VERSION
#define _MajorVersion_ARRAYSIZE ((MajorVersion)(MajorVersion_MAJOR_VERSION+1))

typedef enum _MinorVersion {
    MinorVersion_MINOR_VERSION_UNKNOWN = 0,
    MinorVersion_MINOR_VERSION = 0
} MinorVersion;
#define _MinorVersion_MIN MinorVersion_MINOR_VERSION_UNKNOWN
#define _MinorVersion_MAX MinorVersion_MINOR_VERSION
#define _MinorVersion_ARRAYSIZE ((MinorVersion)(MinorVersion_MINOR_VERSION+1))

typedef enum _PatchVersion {
    PatchVersion_PATCH_VERSION_UNKNOWN = 0,
    PatchVersion_PATCH_VERSION = 0
} PatchVersion;
#define _PatchVersion_MIN PatchVersion_PATCH_VERSION_UNKNOWN
#define _PatchVersion_MAX PatchVersion_PATCH_VERSION
#define _PatchVersion_ARRAYSIZE ((PatchVersion)(PatchVersion_PATCH_VERSION+1))

typedef enum _ClientCommandType {
    ClientCommandType_CLIENT_COMMAND_UNKNOWN = 0,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_DEVICE_UUID = 1,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_TIME = 2,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_ROOT_CA = 3,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_DEVICE_CERT = 4,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_PRIVATE_KEY = 5,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_SSID = 6,
    ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_PASS = 7
} ClientCommandType;
#define _ClientCommandType_MIN ClientCommandType_CLIENT_COMMAND_UNKNOWN
#define _ClientCommandType_MAX ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_PASS
#define _ClientCommandType_ARRAYSIZE ((ClientCommandType)(ClientCommandType_CLIENT_COMMAND_PAYLOAD_SET_WIFI_PASS+1))

typedef enum _ClientQueryType {
    ClientQueryType_CLIENT_QUERY_UNKNOWN = 0,
    ClientQueryType_CLIENT_QUERY_ENCODING_VERSION = 1,
    ClientQueryType_CLIENT_QUERY_HATCH_CONFIGURATION = 2
} ClientQueryType;
#define _ClientQueryType_MIN ClientQueryType_CLIENT_QUERY_UNKNOWN
#define _ClientQueryType_MAX ClientQueryType_CLIENT_QUERY_HATCH_CONFIGURATION
#define _ClientQueryType_ARRAYSIZE ((ClientQueryType)(ClientQueryType_CLIENT_QUERY_HATCH_CONFIGURATION+1))

typedef enum _ClientMessageType {
    ClientMessageType_CLIENT_MESSAGE_UNKNOWN = 0,
    ClientMessageType_CLIENT_MESSAGE_COMMAND = 1,
    ClientMessageType_CLIENT_MESSAGE_QUERY = 2
} ClientMessageType;
#define _ClientMessageType_MIN ClientMessageType_CLIENT_MESSAGE_UNKNOWN
#define _ClientMessageType_MAX ClientMessageType_CLIENT_MESSAGE_QUERY
#define _ClientMessageType_ARRAYSIZE ((ClientMessageType)(ClientMessageType_CLIENT_MESSAGE_QUERY+1))

typedef enum _DeviceResult {
    DeviceResult_DEVICE_RESULT_UNKNOWN = 0,
    DeviceResult_DEVICE_RESULT_SUCCESS = 1,
    DeviceResult_DEVICE_RESULT_ERROR = 2
} DeviceResult;
#define _DeviceResult_MIN DeviceResult_DEVICE_RESULT_UNKNOWN
#define _DeviceResult_MAX DeviceResult_DEVICE_RESULT_ERROR
#define _DeviceResult_ARRAYSIZE ((DeviceResult)(DeviceResult_DEVICE_RESULT_ERROR+1))

typedef enum _DeviceMessageType {
    DeviceMessageType_DEVICE_MESSAGE_UNKNOWN = 0,
    DeviceMessageType_DEVICE_MESSAGE_COMMAND_RESULT = 1,
    DeviceMessageType_DEVICE_MESSAGE_QUERY_RESULT = 2
} DeviceMessageType;
#define _DeviceMessageType_MIN DeviceMessageType_DEVICE_MESSAGE_UNKNOWN
#define _DeviceMessageType_MAX DeviceMessageType_DEVICE_MESSAGE_QUERY_RESULT
#define _DeviceMessageType_ARRAYSIZE ((DeviceMessageType)(DeviceMessageType_DEVICE_MESSAGE_QUERY_RESULT+1))

/* Struct definitions */
typedef PB_BYTES_ARRAY_T(2048) ClientCommand_payload_t;
typedef struct _ClientCommand {
    ClientCommandType type;
    ClientCommand_payload_t payload;
/* @@protoc_insertion_point(struct:ClientCommand) */
} ClientCommand;

typedef struct _ClientQuery {
    ClientQueryType type;
/* @@protoc_insertion_point(struct:ClientQuery) */
} ClientQuery;

typedef struct _DeviceCommandResult {
    ClientCommandType type;
    DeviceResult result;
/* @@protoc_insertion_point(struct:DeviceCommandResult) */
} DeviceCommandResult;

typedef struct _Version {
    MajorVersion major_version;
    MinorVersion minor_version;
    PatchVersion patch_version;
/* @@protoc_insertion_point(struct:Version) */
} Version;

typedef struct _ClientMessage {
    ClientMessageType type;
    uint32_t id;
    ClientCommand command;
    ClientQuery query;
/* @@protoc_insertion_point(struct:ClientMessage) */
} ClientMessage;

typedef struct _DeviceInquryResult {
    ClientQueryType type;
    DeviceResult result;
    Version version;
/* @@protoc_insertion_point(struct:DeviceInquryResult) */
} DeviceInquryResult;

typedef struct _DeviceMessage {
    DeviceMessageType type;
    uint32_t id;
    DeviceCommandResult command_result;
    DeviceInquryResult query_result;
/* @@protoc_insertion_point(struct:DeviceMessage) */
} DeviceMessage;

/* Default values for struct fields */

/* Initializer values for message structs */
#define Version_init_default                     {_MajorVersion_MIN, _MinorVersion_MIN, _PatchVersion_MIN}
#define ClientCommand_init_default               {_ClientCommandType_MIN, {0, {0}}}
#define ClientQuery_init_default                 {_ClientQueryType_MIN}
#define ClientMessage_init_default               {_ClientMessageType_MIN, 0, ClientCommand_init_default, ClientQuery_init_default}
#define DeviceCommandResult_init_default         {_ClientCommandType_MIN, _DeviceResult_MIN}
#define DeviceInquryResult_init_default          {_ClientQueryType_MIN, _DeviceResult_MIN, Version_init_default}
#define DeviceMessage_init_default               {_DeviceMessageType_MIN, 0, DeviceCommandResult_init_default, DeviceInquryResult_init_default}
#define Version_init_zero                        {_MajorVersion_MIN, _MinorVersion_MIN, _PatchVersion_MIN}
#define ClientCommand_init_zero                  {_ClientCommandType_MIN, {0, {0}}}
#define ClientQuery_init_zero                    {_ClientQueryType_MIN}
#define ClientMessage_init_zero                  {_ClientMessageType_MIN, 0, ClientCommand_init_zero, ClientQuery_init_zero}
#define DeviceCommandResult_init_zero            {_ClientCommandType_MIN, _DeviceResult_MIN}
#define DeviceInquryResult_init_zero             {_ClientQueryType_MIN, _DeviceResult_MIN, Version_init_zero}
#define DeviceMessage_init_zero                  {_DeviceMessageType_MIN, 0, DeviceCommandResult_init_zero, DeviceInquryResult_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define ClientCommand_type_tag                   1
#define ClientCommand_payload_tag                2
#define ClientQuery_type_tag                     1
#define DeviceCommandResult_type_tag             1
#define DeviceCommandResult_result_tag           2
#define Version_major_version_tag                1
#define Version_minor_version_tag                2
#define Version_patch_version_tag                3
#define ClientMessage_type_tag                   1
#define ClientMessage_id_tag                     2
#define ClientMessage_command_tag                3
#define ClientMessage_query_tag                  4
#define DeviceInquryResult_type_tag              1
#define DeviceInquryResult_result_tag            2
#define DeviceInquryResult_version_tag           3
#define DeviceMessage_type_tag                   1
#define DeviceMessage_id_tag                     2
#define DeviceMessage_command_result_tag         3
#define DeviceMessage_query_result_tag           4

/* Struct field encoding specification for nanopb */
extern const pb_field_t Version_fields[4];
extern const pb_field_t ClientCommand_fields[3];
extern const pb_field_t ClientQuery_fields[2];
extern const pb_field_t ClientMessage_fields[5];
extern const pb_field_t DeviceCommandResult_fields[3];
extern const pb_field_t DeviceInquryResult_fields[4];
extern const pb_field_t DeviceMessage_fields[5];

/* Maximum encoded size of messages (where known) */
#define Version_size                             6
#define ClientCommand_size                       2053
#define ClientQuery_size                         2
#define ClientMessage_size                       2068
#define DeviceCommandResult_size                 4
#define DeviceInquryResult_size                  12
#define DeviceMessage_size                       28

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define PEEP_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
