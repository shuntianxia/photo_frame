/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "photo_frame_dct.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define UART_BUFF_SIZE 4*1024

/******************************************************
 *                   Enumerations
 ******************************************************/
enum {
	DEVICE_CONNECTING = 40,
	DEVICE_ACTIVE_DONE,
	DEVICE_ACTIVE_FAIL,
	DEVICE_CONNECT_SERVER_FAIL
};

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*parse_socket_msg_fun_t)(char *, uint16_t);

typedef wiced_result_t (*parse_uart_msg_fun_t)();

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct {
	wiced_mac_t mac;
	uint8_t work_mode;
	uint8_t wlan_status;
	//uint8_t internet_status;
	uint8_t tcp_conn_state;
	uint8_t ssid[32];
	//uint8_t device_id[12];
	unsigned int device_id;
	//uint8_t ping_status;
	uint32_t retry_ms;

	wiced_tcp_socket_t tcp_client_socket;
	wiced_tcp_socket_t http_download_socket;
	//wiced_tcp_stream_t tcp_stream;
	wiced_ip_address_t server_ip;	
	wiced_timed_event_t tcp_conn_event;
	//wiced_timed_event_t beacon_event;
	wiced_thread_t uart_send_thread;
	wiced_thread_t uart_recv_thread;
	wiced_thread_t http_down_thread;
	//wiced_worker_thread_t http_download_thread;
	wiced_worker_thread_t wifi_setup_thread;
	wiced_queue_t uart_send_queue;
	wiced_queue_t http_down_queue;

	photo_frame_app_dct_t app_dct;
	platform_dct_wifi_config_t wifi_dct;
} dev_info_t;

typedef struct uart_frame{
	uint32_t len;
	char data[UART_BUFF_SIZE];
}uart_frame_t;

typedef enum {
	DATA_IDLE = 0x0,
	DATA_READY = 0x1
} uart_data_state;

typedef struct {
	uint8_t msg_buf[512];
	uint32_t pos;
	uint32_t data_len;
	uart_data_state state;
	wiced_timer_t timer;
} uart_data_info;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t tcp_connect_handler(void *arg);

#ifdef __cplusplus
} /*extern "C" */
#endif
