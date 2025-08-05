#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwip/sockets.h>
#include <bk_ssl.h>

typedef enum {
	WEBSOCKET_STATE_ERROR = -1,
	WEBSOCKET_STATE_UNKNOW = 0,
	WEBSOCKET_STATE_INIT,
	WEBSOCKET_STATE_CONNECTED,
	WEBSOCKET_STATE_WAIT_TIMEOUT,
	WEBSOCKET_STATE_CLOSING,
} websocket_client_state_t;

typedef enum ws_transport_opcodes {
	WS_TRANSPORT_OPCODES_CONT =  0x00,
	WS_TRANSPORT_OPCODES_TEXT =  0x01,
	WS_TRANSPORT_OPCODES_BINARY = 0x02,
	WS_TRANSPORT_OPCODES_CLOSE = 0x08,
	WS_TRANSPORT_OPCODES_PING = 0x09,
	WS_TRANSPORT_OPCODES_PONG = 0x0a,
	WS_TRANSPORT_OPCODES_FIN = 0x80,
	WS_TRANSPORT_OPCODES_NONE = 0x100,
} ws_transport_opcodes_t;

typedef enum {
	WEBSOCKET_EVENT_ANY = -1,
	WEBSOCKET_EVENT_ERROR = 0,
	WEBSOCKET_EVENT_CONNECTED,
	WEBSOCKET_EVENT_DISCONNECTED,
	WEBSOCKET_EVENT_DATA,
	WEBSOCKET_EVENT_CLOSED,
	WEBSOCKET_EVENT_MAX
} websocket_event_id_t;

typedef struct {
	uint8_t opcode;
	char mask_key[4];
	int payload_len;
	int bytes_remaining;
	bool header_received;
} ws_transport_frame_state_t;

typedef struct {
	char *path;
	char *buffer;
	char *sub_protocol;
	char *user_agent;
	char *headers;
	bool propagate_control_frames;
	ws_transport_frame_state_t frame_state;
} transport_ws_t;

typedef struct {
	char						*host;
	char						*path;
	char						*scheme;
	char						*username;
	char						*password;
	int 						port;
	void						*user_context;
} websocket_config_t;

typedef void  (*bk_event_handler_t)(void* event_handler_arg, char *event_base, int32_t event_id, void* event_data);

typedef struct {
	websocket_config_t			*config;
	websocket_client_state_t	state;
  //  uint64_t					  keepalive_tick_ms;
	uint64_t					reconnect_tick_ms;
	uint64_t					ping_tick_ms;
	uint64_t					pingpong_tick_ms;
	int 						auto_reconnect;
	bool						run;
	bool						wait_for_pong_resp;
	char						*rx_buffer;
	char						*tx_buffer;
	int 						buffer_size;
	int							rx_retry;
	ws_transport_opcodes_t		last_opcode;
	int 						payload_len;
	int 						payload_offset;
	transport_ws_t			   *ws_transport;
	int 						sockfd;
	int							is_tls;
	transport_bk_tls_t			*bk_ssl;
	bk_event_handler_t			ws_event_handler;
	beken_mutex_t 				mutex;
}websocket_transport_info_t;

typedef struct {
	const char					*uri;
//	  bool						  disable_auto_reconnect;
	int							rx_retry;
	void						*user_context;
	int							buffer_size;
	const char					*subprotocol;
	const char					*user_agent;
	const char					*headers;
//	bool						disable_pingpong_discon;
	bk_event_handler_t			ws_event_handler;
} websocket_client_input_t;

typedef websocket_transport_info_t *transport;
typedef void (*websocket_event_cb)(int32_t event_id, char *event_data, int data_len);

/**
 * @brief Websocket event data
 */
typedef struct {
    char *data_ptr;
    int data_len;
    uint8_t op_code;
    transport client;
    void *user_context;
    int payload_len;
    int payload_offset;
} bk_websocket_event_data_t;

transport websocket_client_init(const websocket_client_input_t *input);
bk_err_t websocket_client_destroy(transport client);
int websocket_client_start(transport client);
bool websocket_client_is_connected(transport client);
int websocket_client_send_text(transport client, const char *data, int len, int timeout);
int websocket_client_send_binary(transport client, const char *data, int len, int timeout);

