/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * WICED Configuration Mode Application
 *
 * This application demonstrates how to use WICED Configuration Mode
 * to automatically configure application parameters and Wi-Fi settings
 * via a softAP and webserver
 *
 * Features demonstrated
 *  - WICED Configuration Mode
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. After the download completes, the terminal displays WICED startup
 *      information and starts WICED configuration mode.
 *
 * In configuration mode, application and Wi-Fi configuration information
 * is entered via webpages using a Wi-Fi client (eg. your computer)
 *
 * Use your computer to step through device configuration using WICED Config Mode
 *   - Connect the computer using Wi-Fi to the config softAP "WICED Config"
 *     The config AP name & passphrase is defined in the file <WICED-SDK>/include/default_wifi_config_dct.h
 *     The AP name/passphrase is : Wiced Config / 12345678
 *   - Open a web browser and type wiced.com in the URL
 *     (or enter 192.168.0.1 which is the IP address of the softAP interface)
 *   - The Application configuration webpage appears. This page enables
 *     users to enter application specific information such as contact
 *     name and address details for device registration
 *   - Change one of more of the fields in the form and then click 'Save settings'
 *   - Click the Wi-Fi Setup button
 *   - The Wi-Fi configuration page appears. This page provides several options
 *     for configuring the device to connect to a Wi-Fi network.
 *   - Click 'Scan and select network'. The device scans for Wi-Fi networks in
 *     range and provides a webpage with a list.
 *   - Enter the password for your Wi-Fi AP in the Password box (top left)
 *   - Find your Wi-Fi AP in the list, and click the 'Join' button next to it
 *
 * Configuration mode is complete. The device stops the softAP and webserver,
 * and attempts to join the Wi-Fi AP specified during configuration. Once the
 * device completes association, application configuration information is
 * printed to the terminal
 *
 * The wiced.com URL reference in the above text is configured in the DNS
 * redirect server. To change the URL, edit the list in
 * <WICED-SDK>/Library/daemons/dns_redirect.c
 * URLs currently configured are:
 *      # http://www.broadcom.com , http://broadcom.com ,
 *      # http://www.facebook.com , http://facebook.com ,
 *      # http://www.google.com   , http://google.com   ,
 *      # http://www.bing.com     , http://bing.com     ,
 *      # http://www.apple.com    , http://apple.com    ,
 *      # http://www.wiced.com    , http://wiced.com    ,
 *
 *  *** IMPORTANT NOTE ***
 *   The config mode API will be integrated into Wi-Fi Easy Setup when
 *   WICED-SDK-3.0.0 is released.
 *
 */

#include "wiced.h"
#include "photo_frame.h"
#include "photo_frame_dct.h"
#include "http_download.h"
#include "cJSON.h"
#include "utils.h"

/******************************************************
 *                      Macros
 ******************************************************/


/******************************************************
 *                    Constants
 ******************************************************/
#define QRCODE_ADDR_PREFIX "http://mp.weixin.qq.com/cgi-bin/showqrcode?ticket="
#define GET_TICKET_URL "http://www.yekertech.com:8080/weixinbestidear/api/device/saveDeviceAndGetTicket?ApiKey=test"
#define E200_UART WICED_UART_2
#define NULL_MODE       0x00
#define STATION_MODE    0x01
#define SOFTAP_MODE     0x02
#define STATIONAP_MODE  0x03

#define DEBUG_UART_RECEIVE
//#define TRANSFER_FILE_RESPONSE

#define RX_WAIT_TIMEOUT        (1*SECONDS)

//#define MAX_RECV_SIZE    8192
#define TCP_PACKET_MAX_DATA_LENGTH        30
#define TCP_CLIENT_INTERVAL               2
#define TCP_CLIENT_CONNECT_TIMEOUT        500
#define TCP_CLIENT_RECEIVE_TIMEOUT        300
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3
#define RX_BUFFER_SIZE    256

#define UART_RECEIVE_THREAD_STACK_SIZE      5*1024
#define UART_SEND_THREAD_STACK_SIZE    5*1024
#define HTTP_DOWN_THREAD_STACK_SIZE    6*1024

#define HTTP_DOWNLOAD_THREAD_PRIORITY WICED_NETWORK_WORKER_PRIORITY - 1
#define UART_TRANSPORT_THREAD_PRIORITY WICED_APPLICATION_PRIORITY
#define WIFI_SETUP_THREAD_PRIORITY WICED_APPLICATION_PRIORITY

#define HTTP_DOWNLOAD_STACK_SIZE      4096
#define UART_TRANSPORT_STACK_SIZE      4096
#define WIFI_SETUP_STACK_SIZE      4096

#define HTTP_DOWNLOAD_QUEUE_SIZE      10
#define UART_TRANSPORT_QUEUE_SIZE      10
#define WIFI_SETUP_QUEUE_SIZE      1

#define UDP_MAX_DATA_LENGTH         256
#define UART_QUEUE_DEPTH	5
#define DOWN_QUEUE_DEPTH    10
#define UART_FRAME_INTERVAL	5
#define UART_FRAME_TIMEOUT	(UART_FRAME_INTERVAL/2)
#define PORTNUM 8088
#define TCP_SERVER_PORT 9000

#define CMD_HEAD_BYTES 5

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/


/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/
//static wiced_result_t dev_init();
static void dev_login_sent(void* socket);
/******************************************************
 *               Variable Definitions
 ******************************************************/
dev_info_t dev_info;

static wiced_uart_config_t uart_config =
{
    .baud_rate    = 1036800,
    //.baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};

wiced_ring_buffer_t rx_buffer;
uint8_t             rx_data[RX_BUFFER_SIZE];
uart_data_info uart_msg;
wiced_semaphore_t uart_response_sem;
int uart_response_result;
static char device_ticket[192];
/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t wifi_setup();
#if 0
static wiced_result_t tcp_sent_beacon(void* socket)
{
    if (dev_info.tcp_conn_state == 1) {
        if (dev_info.app_dct.activeflag == 0) {
            printf("plese check device is activated.\n");
            dev_login_sent(socket);
        } else {
			char pbuf[128] = {0};
			pbuf[0] = 0xb1;
			pbuf[1] = 0x03;

            //tcp_send_data(socket, pbuf, 2);
			wiced_tcp_send_buffer(socket, pbuf, 2);
        }
    } else {
        printf("tcp_sent_beacon sent fail!\n");
        //user_esp_platform_discon(pespconn);
    }
    return WICED_SUCCESS;
}
#endif
static int tcp_connect(wiced_tcp_socket_t *socket, wiced_ip_address_t *ip, uint16_t port)
{
	wiced_result_t			 result;
	int 					 connection_retries;

	/* Connect to the remote TCP server, try several times */
	connection_retries = 0;
	do
	{
		result = wiced_tcp_connect( socket, ip, port, TCP_CLIENT_CONNECT_TIMEOUT );
		connection_retries++;
	}
	while( ( result != WICED_SUCCESS ) && ( connection_retries < TCP_CONNECTION_NUMBER_OF_RETRIES ) );
	if( result != WICED_SUCCESS)
	{
		WPRINT_APP_INFO(("Unable to connect to the server!\n"));
		return WICED_ERROR;
	}
	
	return WICED_SUCCESS;
}

static int tcp_disconnect(wiced_tcp_socket_t* socket)
{
	return wiced_tcp_disconnect(socket);
}

static wiced_result_t tcp_client_connect()
{
	wiced_result_t result;
	int connection_retries;
	
	/* Connect to the remote TCP server, try several times */
	connection_retries = 0;
	do
	{ 
		if(wiced_hostname_lookup("api.yekertech.com", &dev_info.server_ip, 1000) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("hostname look up failed\n"));
			continue;
		}
		result = wiced_tcp_connect( &dev_info.tcp_client_socket, &dev_info.server_ip, TCP_SERVER_PORT, TCP_CLIENT_CONNECT_TIMEOUT );
		connection_retries++;
	}
	while( ( result != WICED_SUCCESS ) && ( connection_retries < TCP_CONNECTION_NUMBER_OF_RETRIES ) );
	if( result != WICED_SUCCESS)
	{
		WPRINT_APP_INFO(("Unable to connect to the server, retry a later. result = %d\n", result));
		//wiced_tcp_unregister_callbacks(&dev_info.tcp_client_socket);
		//wiced_tcp_delete_socket(&dev_info.tcp_client_socket);
		return WICED_ERROR;
	} else {
		dev_info.tcp_conn_state = 1;
		dev_login_sent(&dev_info.tcp_client_socket);
		return WICED_SUCCESS;
		//wiced_rtos_register_timed_event( &dev_info.beacon_event, WICED_NETWORKING_WORKER_THREAD, &tcp_sent_beacon, 50*SECONDS, &dev_info.tcp_socket);
	}
}

static wiced_result_t tcp_connect_callback( void* socket)
{
	printf("tcp_connect_callback\n");
	
    return WICED_SUCCESS;
}

static wiced_result_t tcp_disconnect_callback( void* socket)
{
	wiced_result_t result;
	
	printf("tcp_disconnect_callback\n");
	dev_info.tcp_conn_state = 0;

	//wiced_rtos_deregister_timed_event(&dev_info.beacon_event);

	if((result = wiced_tcp_disconnect(socket)) != WICED_SUCCESS) {
		WPRINT_APP_INFO(("wiced_tcp_disconnect error. result = %d\n", result));
	}
	do
	{
		if((result = tcp_client_connect()) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("tcp_client_connect failed, retry a later\n"));
			host_rtos_delay_milliseconds( 1000 );
		}
	}while(result != WICED_SUCCESS);
	
    return WICED_SUCCESS;
}

static void dev_login_sent(void* socket)
{
	char pbuf[64];

	WPRINT_APP_INFO(("dev_login_sent dev_id is %u\n", dev_info.device_id));

	memset(pbuf, 0x0, sizeof(pbuf));         
    sprintf(pbuf, "device:%ueof", dev_info.device_id);
	
	wiced_tcp_send_buffer(socket, pbuf, strlen(pbuf));
}

static wiced_result_t http_download_handler_bak()
{
	char *url;
	int qrcode_flag = 0;
	if(wiced_rtos_pop_from_queue(&dev_info.http_down_queue, &url, WICED_NEVER_TIMEOUT) == WICED_SUCCESS) {
		if(url != NULL) {
			WPRINT_APP_INFO(("%s\n", url));
			if(strncmp(url, QRCODE_ADDR_PREFIX, strlen(QRCODE_ADDR_PREFIX)) == 0) {
				qrcode_flag = 1;
			}
			http_download(url, qrcode_flag);
			free(url);
			url = NULL;
		}
	}
	return WICED_SUCCESS;
}

static wiced_result_t http_download_handler()
{
	char *url;
	int qrcode_flag;

	while(1) {
		if(wiced_rtos_pop_from_queue(&dev_info.http_down_queue, &url, WICED_NEVER_TIMEOUT) == WICED_SUCCESS) {
			if(url != NULL) {
				WPRINT_APP_INFO(("received the file link address : %s\n", url));
				if(strncmp(url, QRCODE_ADDR_PREFIX, strlen(QRCODE_ADDR_PREFIX)) == 0) {
					qrcode_flag = 1;
				} else {
					qrcode_flag = 0;
				}
				if(http_download(url, qrcode_flag) != WICED_SUCCESS) {
					WPRINT_APP_INFO(("http_download error\n"));
				}
				WPRINT_APP_INFO(("http_download success\n"));
				free(url);
				url = NULL;
			}
		}
	}
}

wiced_result_t parse_tcp_msg(void *socket, char *buffer, unsigned short length)
{
	cJSON *json, *json_url, *json_type;
	char *url_buf;
	int url_len;

#if 0
	if(buffer[0] != '{') {
		return WICED_ERROR;
	}
#endif
    // 解析数据包  
    json = cJSON_Parse(buffer);  
    if (!json) {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
    }
	//wiced_tcp_send_buffer(socket, "OKeof", 5);
	//WPRINT_APP_INFO(("after send OKeof\n"));
	
    json_type = cJSON_GetObjectItem( json , "type");  
    if( json_type->type == cJSON_String && (strcmp(json_type->valuestring, "pic") == 0))  
    {
		json_url = cJSON_GetObjectItem(json, "msg");
		if( json_url->type == cJSON_String )  
		{
			url_len = strlen(json_url->valuestring);
			url_buf = malloc(url_len + 1);
			if(url_buf != NULL) {
				memset(url_buf, 0, url_len + 1);
				memcpy(url_buf, json_url->valuestring, url_len);
				WICED_VERIFY(wiced_rtos_push_to_queue(&dev_info.http_down_queue, &url_buf, WICED_NEVER_TIMEOUT));
				WPRINT_APP_INFO(("wiced_rtos_push_to_queue\n"));
				//wiced_rtos_send_asynchronous_event(&dev_info.http_download_thread, http_download_handler, NULL);
			}
        }
    }
    // 释放内存空间  
    cJSON_Delete(json);  
	return WICED_SUCCESS;
}

static wiced_result_t tcp_receive_callback(void* socket)
{
    wiced_result_t           result;
    //wiced_packet_t*          packet;
    wiced_packet_t*          rx_packet;
    char*                    rx_data;
    uint16_t                 rx_data_length;
    uint16_t                 available_data_length;

	WPRINT_APP_INFO(("tcp_receive_callback\n"));

    /* Receive a response from the server and print it out to the serial console */
    result = wiced_tcp_receive(socket, &rx_packet, TCP_CLIENT_RECEIVE_TIMEOUT);
   	if ( ( result == WICED_ERROR ) || ( result == WICED_TIMEOUT ) )
	{
		return result;
	}

    /* Get the contents of the received packet */
    wiced_packet_get_data(rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length);

	parse_tcp_msg(socket, rx_data, rx_data_length);

	wiced_tcp_send_buffer(socket, "OKeof", 5);

    /* Delete the packet */
    wiced_packet_delete(rx_packet);

    return WICED_SUCCESS;
}

wiced_result_t tcp_connect_handler(void *arg)
{
    wiced_result_t           result;
	int connection_retries;
	
	/* Bring up the network interface */
	if(wiced_network_is_up(WICED_STA_INTERFACE) == WICED_FALSE) {
		result = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
		if(result != WICED_SUCCESS) {
			return WICED_ERROR;
		}
	}
	dev_info.wlan_status = 1;

	/* Create a TCP socket */
	if (wiced_tcp_create_socket(&dev_info.tcp_client_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
	{
		WPRINT_APP_INFO(("TCP socket creation failed\n"));
		return WICED_ERROR;
	}

	/* Connect to the remote TCP server, try several times */
	connection_retries = 0;
	do
	{ 
		if(wiced_hostname_lookup("api.yekertech.com", &dev_info.server_ip, 1000) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("hostname look up failed\n"));
			wiced_tcp_delete_socket(&dev_info.tcp_client_socket);
			return WICED_ERROR;
		}
		result = wiced_tcp_connect( &dev_info.tcp_client_socket, &dev_info.server_ip, TCP_SERVER_PORT, TCP_CLIENT_CONNECT_TIMEOUT );
		connection_retries++;
	}
	while( ( result != WICED_SUCCESS ) && ( connection_retries < TCP_CONNECTION_NUMBER_OF_RETRIES ) );
	if( result != WICED_SUCCESS)
	{
		WPRINT_APP_INFO(("Unable to connect to the server, retry a later. result = %d\n", result));
		wiced_tcp_delete_socket(&dev_info.tcp_client_socket);
		return WICED_ERROR;
	}
	dev_info.tcp_conn_state = 1;
	wiced_rtos_deregister_timed_event(&dev_info.tcp_conn_event);
	wiced_tcp_register_callbacks(&dev_info.tcp_client_socket, tcp_connect_callback, tcp_receive_callback, tcp_disconnect_callback);
	dev_login_sent(&dev_info.tcp_client_socket);
	//wiced_rtos_register_timed_event( &dev_info.beacon_event, WICED_NETWORKING_WORKER_THREAD, &tcp_sent_beacon, 50*SECONDS, &dev_info.tcp_socket);
	return WICED_SUCCESS;
}

wiced_result_t tcp_client_enable()
{
	wiced_bool_t exit_flag = WICED_FALSE;
	
	while(!exit_flag) {
		/* Bring up the network interface */
		if(wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL ) != WICED_SUCCESS) {
			host_rtos_delay_milliseconds( 1000 );
			continue;
		}
		dev_info.wlan_status = 1;
		
		/* Create a TCP socket */
		if (wiced_tcp_create_socket(&dev_info.tcp_client_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
		{
			WPRINT_APP_INFO(("TCP socket creation failed\n"));
			continue;
		}
		wiced_tcp_register_callbacks(&dev_info.tcp_client_socket, tcp_connect_callback, tcp_receive_callback, tcp_disconnect_callback);
		if(tcp_client_connect() == WICED_SUCCESS) {
			exit_flag = WICED_TRUE;
		} else {
			wiced_tcp_unregister_callbacks(&dev_info.tcp_client_socket);
			wiced_tcp_delete_socket(&dev_info.tcp_client_socket);
		}
	}
	return WICED_SUCCESS;
}

static void tcp_client_disable()
{
	dev_info.tcp_conn_state = 0;
	
	//wiced_rtos_deregister_timed_event(&dev_info.beacon_event);

	//wiced_rtos_deregister_timed_event(&dev_info.tcp_conn_event);

	wiced_tcp_unregister_callbacks(&dev_info.tcp_client_socket);
	
	wiced_tcp_delete_socket(&dev_info.tcp_client_socket);
}

static void link_up( void )
{

	WPRINT_APP_INFO( ("And we're connected again!\n") );
	
	/* Set a semaphore to indicate the link is back up */
	//wiced_rtos_set_semaphore( &link_up_semaphore );
	dev_info.wlan_status = 1;

	//wiced_rtos_send_asynchronous_event(&dev_info.http_download_thread, http_download_handler, NULL);

	tcp_client_enable();
	//wiced_rtos_register_timed_event( &dev_info.tcp_conn_event, WICED_NETWORKING_WORKER_THREAD, &tcp_connect_handler, 1*SECONDS, 0 );
}

static void link_down( void )
{
	WPRINT_APP_INFO( ("Network connection is down.\n") );

	dev_info.wlan_status = 0;

	tcp_client_disable();
}

wiced_result_t send_get_ticket_request(wiced_tcp_socket_t* socket, url_info_t *url_info)
{
	cJSON *root;
	char *out;
	char send_buf[512];
	char port[10];
	char device_id_str[10];
	char content_length[64];
	
	memset(device_id_str, 0, sizeof(device_id_str));
	sprintf(device_id_str, "%u", dev_info.device_id);
	
	root=cJSON_CreateObject();	
 	cJSON_AddStringToObject(root,"Tag", device_id_str);
	out=cJSON_Print(root);

	
	memset(send_buf, 0x0, sizeof(send_buf));		   
	sprintf(send_buf, "POST %s",url_info->path);
	strcat(send_buf," HTTP/1.1\r\n");
	strcat(send_buf, "Host: ");
	strcat(send_buf, url_info->host);
	strcat(send_buf, ":");
	sprintf(port, "%d", url_info->port);
	strcat(send_buf, port);
	strcat(send_buf, "\r\nContent-Type: application/json");
	sprintf(content_length, "\r\nContent-Length: %d\r\n\r\n", strlen(out));
	strcat(send_buf, content_length);
	strcat(send_buf, out);
	
	cJSON_Delete(root);
	free(out);
	WICED_VERIFY(wiced_tcp_send_buffer(socket, send_buf, strlen(send_buf)));
	return WICED_SUCCESS;
}

static wiced_result_t receive_get_ticket_response(wiced_tcp_socket_t* socket)
{
	uint8_t*                      rx_data;
	uint16_t                      rx_data_length;
	wiced_packet_t*               rx_packet;
	uint16_t                      available_data_length;
    char*                         message_data_length_string;
    wiced_result_t                result                 = WICED_ERROR;
	cJSON *root, *ticket;
	char *data, *json_start, *ptmp;

	result = wiced_tcp_receive(socket, &rx_packet, 500);
	if ( result != WICED_TCPIP_SUCCESS)
	{
		WPRINT_APP_INFO(("TCP receive failed\n"));
		return result;
	}
	WICED_VERIFY( wiced_packet_get_data(rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length));
	
	/* Verify we have enough data to start processing */
	if ( rx_data_length < 18 )
	{
		WPRINT_APP_INFO(("rx_data_length < MINIMUM_REQUEST_LINE_LENGTH\n"));
		return WICED_ERROR;
	}
	
	/* Now extract packet payload info such as data, data length, data type and message length */
	data = (uint8_t*)strstr( (char*)rx_data, CRLF_CRLF );
	
	/* This indicates start of data/end of header was not found, so exit */
	if ( data  == NULL)
	{
		WPRINT_APP_INFO(("http_message.data == NULL\n"));
		return WICED_ERROR;
	}
	else
	{
		/* Payload starts just after the header */
		data += strlen( CRLF_CRLF );
	}
	ptmp = strchr(data, '\r');
    if (NULL==ptmp)
    {
        printf("error\n");
        return WICED_ERROR;
    }
    json_start = ptmp + 2;
	root = cJSON_Parse(json_start);
	if (!root) {
		WPRINT_APP_INFO(("Error before: [%s]\n",cJSON_GetErrorPtr()));
		return WICED_ERROR;
	}
	ticket = cJSON_GetObjectItem( root , "Ticket");
	if(!ticket) {
		printf("no ticket\n");
		return WICED_ERROR;
	}
	if( ticket->type == cJSON_String ) {
		memset(&device_ticket, 0, sizeof(device_ticket));
		strcpy(device_ticket, ticket->valuestring);
	}
	cJSON_Delete(root);
    return WICED_SUCCESS;
}

static wiced_result_t get_device_ticket()
{
	url_info_t ticket_url;
	wiced_tcp_socket_t ticket_socket;
	
	memset(&ticket_url, 0, sizeof(ticket_url));
    WICED_VERIFY(parse_url(GET_TICKET_URL, &ticket_url));

	/* Create a TCP socket */
    if (wiced_tcp_create_socket(&ticket_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("TCP socket creation failed\n"));
		return WICED_ERROR;
    }
	
    //建立连接
    if(tcp_connect(&ticket_socket, &ticket_url.ip_addr, ticket_url.port)!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("TCP connect failed\n"));
		wiced_tcp_delete_socket(&ticket_socket);
		return WICED_ERROR;
    }

	send_get_ticket_request(&ticket_socket, &ticket_url);

	if(receive_get_ticket_response(&ticket_socket) == WICED_ERROR) {
		WPRINT_APP_INFO(("get ticket failed\n"));
		tcp_disconnect(&ticket_socket);
		wiced_tcp_delete_socket(&ticket_socket);
		return WICED_ERROR;
	}

	//断开连接
    tcp_disconnect(&ticket_socket);

	/* Delete a TCP socket */
	wiced_tcp_delete_socket(&ticket_socket);
	return WICED_SUCCESS;

}

wiced_result_t report_device_status()
{
	cJSON *root, *params;
	char *out;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "method", "report_device_status");
	cJSON_AddItemToObject(root, "params", params = cJSON_CreateObject());
	cJSON_AddNumberToObject(params, "work_mode", dev_info.work_mode);
	cJSON_AddNumberToObject(params, "wlan_status", dev_info.wlan_status);
	cJSON_AddNumberToObject(params, "internet_status", dev_info.tcp_conn_state);
	cJSON_AddStringToObject(params, "ssid", (const char*)dev_info.ssid);
	cJSON_AddNumberToObject(params, "device_id", dev_info.device_id);

	out = cJSON_Print(root);
	uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
	if(uart_frame != NULL) {
		memset(uart_frame, 0, sizeof(uart_frame_t));
		sprintf(uart_frame->data, "%s\r\n\r\n", out);
		uart_frame->len = strlen(uart_frame->data);
	}
	wiced_uart_transmit_bytes( E200_UART, uart_frame->data, uart_frame->len);
	free(uart_frame);
	free(out);
}

wiced_result_t parse_uart_msg(char *buffer, uint32_t length)
{
	cJSON *root, *method, *params, *response, *item, *json_send = NULL;
	int i, array_size;
	char *out;
	char *url_buf;
	int url_len;
	int result;

    root = cJSON_Parse(buffer);
    if (!root) {  
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return WICED_ERROR;
    }
	
    method = cJSON_GetObjectItem( root , "method");
	if(!method) {
		printf("no method\n");
		return WICED_ERROR;
	}
    if( method->type == cJSON_String ) {
		if(strcmp(method->valuestring, "get_device_status") == 0) {
			//json_send = cJSON_CreateObject();
			cJSON_DeleteItemFromObject(root, "request");
			cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
			params = cJSON_GetObjectItem(root, "params");
			if(!params) {
				printf("no params\n");
				return WICED_ERROR;
			}
			cJSON *attrset = cJSON_GetObjectItem(params, "attrset");
			if(!attrset) {
				printf("no attrset\n");
				return WICED_ERROR;
			}
			if(attrset->type == cJSON_Array) {
				array_size = cJSON_GetArraySize(attrset);
				for(i = 0; i < array_size; i++) {
					item = cJSON_GetArrayItem(attrset, i);
					if(item->type == cJSON_String) {
						if(strcmp(item->valuestring, "work_mode") == 0) {
							cJSON_AddNumberToObject(params, "work_mode", dev_info.work_mode);
						} else if(strcmp(item->valuestring, "wlan_status") == 0) {
							cJSON_AddNumberToObject(params, "wlan_status", dev_info.wlan_status);
						} else if(strcmp(item->valuestring, "internet_status") == 0) {
							cJSON_AddNumberToObject(params, "internet_status", dev_info.tcp_conn_state);
						} else if(strcmp(item->valuestring, "ssid") == 0) {
							cJSON_AddStringToObject(params, "ssid", (const char*)dev_info.ssid);
						} else if(strcmp(item->valuestring, "device_id") == 0) {
							cJSON_AddNumberToObject(params, "device_id", dev_info.device_id);
						}
					}
				}
			}
			json_send = root;
		} else if(strcmp(method->valuestring, "set_device_status") == 0) {
			cJSON_DeleteItemFromObject(root, "request");
			cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
			cJSON_AddNumberToObject(response, "result", 0);
			params = cJSON_GetObjectItem(root, "params");
			if(!params) {
				printf("no params\n");
				return WICED_ERROR;
			}
			item = cJSON_GetObjectItem(params, "work_mode");
			if(!item) {
				printf("no work_mode\n");
			}
			if(item->type == cJSON_Number && item->valueint == 1) {
				if(dev_info.app_dct.work_mode != 1) {
					//WPRINT_APP_INFO(("enter wifi_setup function\n"));
					//wiced_rtos_deregister_timed_event(&dev_info.tcp_conn_event);
					//WPRINT_APP_INFO(("after deregister timed event\n"));
					dev_info.app_dct.work_mode = 1;
					//tcp_client_disable();
					if(wiced_network_is_up(WICED_STA_INTERFACE) == WICED_TRUE) {
						WPRINT_APP_INFO(("wiced_network_is_up == WICED_TRUE\n"));
						dev_info.app_dct.work_mode = 1;
						dev_info.wlan_status = 0;
						dev_info.tcp_conn_state = 0;
						store_app_data(&dev_info.app_dct);
						//reboot();
						wiced_rtos_send_asynchronous_event(&dev_info.wifi_setup_thread, reboot, NULL);
					} else {
						WPRINT_APP_INFO(("wiced_network_is_up != WICED_TRUE\n"));
						wiced_rtos_send_asynchronous_event(&dev_info.wifi_setup_thread, wifi_setup, NULL);
					}
				}
			}
			json_send = root;
		} else if(strcmp(method->valuestring, "get_wechat_qrcode") == 0) {
			if(get_device_ticket() == WICED_SUCCESS) {
				result = 0;
				url_len = strlen(QRCODE_ADDR_PREFIX) + strlen(device_ticket);
				url_buf = malloc(url_len + 1);
				if(url_buf != NULL) {
					memset(url_buf, 0, url_len + 1);
					sprintf(url_buf, "%s", QRCODE_ADDR_PREFIX);
					strcat(url_buf, device_ticket);
					WICED_VERIFY(wiced_rtos_push_to_queue(&dev_info.http_down_queue, &url_buf, WICED_NEVER_TIMEOUT));
					//wiced_rtos_send_asynchronous_event(&dev_info.http_download_thread, http_download_handler, NULL);
				}
			} else {
				result = -1;
			}
			cJSON_DeleteItemFromObject(root, "request");
			cJSON_AddItemToObject(root, "response", response = cJSON_CreateObject());
			cJSON_AddNumberToObject(response, "result", result);
			json_send = root;
		}
#ifdef TRANSFER_FILE_RESPONSE
		else if(strcmp(method->valuestring, "transfer_file") == 0) {
			response = cJSON_GetObjectItem(root, "response");
			if(!response) {
				printf("no response\n");
			}
			item = cJSON_GetObjectItem(response, "result");
			if(!item) {
				printf("no result\n");
			}
			if(item->type == cJSON_Number) {
				uart_response_result = item->valueint;
				wiced_rtos_set_semaphore(&uart_response_sem);
				WPRINT_APP_INFO(("continue send file\n"));
			}
		}
#endif
		if(json_send != NULL) {
			out = cJSON_Print(json_send);
			uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
			if(uart_frame != NULL) {
				memset(uart_frame, 0, sizeof(uart_frame_t));
				sprintf(uart_frame->data, "%s\r\n\r\n", out);
				uart_frame->len = strlen(uart_frame->data);
			}
			//WICED_VERIFY(wiced_rtos_push_to_queue(&dev_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT));
			wiced_uart_transmit_bytes( E200_UART, uart_frame->data, uart_frame->len);
			free(uart_frame);
			free(out);
			json_send = NULL;
		}
    }
    // 释放内存空间  
    cJSON_Delete(root);
    return WICED_SUCCESS;
}

void uart_receive_handler(uint32_t arg)
{
	char c;
	wiced_result_t result;
	
	while ( 1 )
	{
		result = wiced_uart_receive_bytes( E200_UART, &c, 1, UART_FRAME_TIMEOUT);
		if(result != WICED_SUCCESS) {
			uart_msg.pos = 0;
			continue;
		}
#ifdef DEBUG_UART_RECEIVE
		WPRINT_APP_INFO(("%c", c));
#endif
		if(uart_msg.pos < sizeof(uart_msg.msg_buf)) {
			uart_msg.msg_buf[uart_msg.pos++] = c;
			if(c == '\n' && uart_msg.pos >= 6) {
				if(strncmp((char*)&uart_msg.msg_buf[uart_msg.pos - 4], "\r\n\r\n", 4) == 0) {
					parse_uart_msg((char*)uart_msg.msg_buf, uart_msg.pos);
					uart_msg.pos = 0;
					continue;
				}
			}
		}
	}
}

static int get_uart_response()
{
	WPRINT_APP_INFO(("waiting for uart response\n"));
	wiced_rtos_get_semaphore(&uart_response_sem, WICED_NEVER_TIMEOUT);
	return uart_response_result;
}

void uart_send_handler()
{
	uart_frame_t *uart_frame;

	while(1) {
		if(wiced_rtos_pop_from_queue(&dev_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT) == WICED_SUCCESS) {
			wiced_uart_transmit_bytes( E200_UART, uart_frame->data, uart_frame->len);
#ifdef DEBUG_UART_SEND
			WPRINT_APP_INFO(("uart send: uart_frame->len = %lu\n", uart_frame->len));
#endif
#ifdef TRANSFER_FILE_RESPONSE
			while(get_uart_response() == -1) {
				wiced_uart_transmit_bytes( E200_UART, uart_frame->data, uart_frame->len);
			}
#endif
			free(uart_frame);
			uart_frame = NULL;
			host_rtos_delay_milliseconds(200);
		}
	}
}

void test_uart_fun()
{
	char c = 1;
	char buf[1024];
	uint32_t i;
	uint32_t count = 10*1024;

	memset(buf, 1, sizeof(buf));

	for(i = 0; i < count; i++)
	{
		wiced_uart_transmit_bytes( E200_UART, buf, 1024);
		WPRINT_APP_INFO(("have send %d KByte\n", i+1));
	}
	WPRINT_APP_INFO(("test uart finished\n"));
	return;
}

wiced_result_t uart_transceiver_enable()
{
	/* Initialise ring buffer */
    ring_buffer_init(&rx_buffer, rx_data, RX_BUFFER_SIZE );

    /* Initialise UART. A ring buffer is used to hold received characters */
    wiced_uart_init( E200_UART, &uart_config, &rx_buffer );

	//host_rtos_delay_milliseconds(20*1000);
	//test_uart_fun();
#if 1
#ifdef TRANSFER_FILE_RESPONSE
	wiced_rtos_init_semaphore(&uart_response_sem);
#endif
	WICED_VERIFY( wiced_rtos_init_queue( &dev_info.uart_send_queue, NULL, sizeof(uart_frame_t *), UART_QUEUE_DEPTH ) );
	WPRINT_APP_INFO(("create uart receive thread\n"));
	wiced_rtos_create_thread(&dev_info.uart_recv_thread, WICED_APPLICATION_PRIORITY, "uart receive thread", uart_receive_handler, UART_RECEIVE_THREAD_STACK_SIZE, 0);
	WPRINT_APP_INFO(("create uart send thread\n"));
	wiced_rtos_create_thread(&dev_info.uart_send_thread, WICED_APPLICATION_PRIORITY, "uart send thread", uart_send_handler, UART_SEND_THREAD_STACK_SIZE, 0);
	return WICED_SUCCESS;
#endif
}

wiced_result_t http_download_enable()
{
	WICED_VERIFY( wiced_rtos_init_queue( &dev_info.http_down_queue, NULL, sizeof(void *), DOWN_QUEUE_DEPTH ) );	
	
	/* Create http download worker thread */
    //WICED_VERIFY( wiced_rtos_create_worker_thread( &dev_info.http_download_thread, HTTP_DOWNLOAD_THREAD_PRIORITY, HTTP_DOWNLOAD_STACK_SIZE, HTTP_DOWNLOAD_QUEUE_SIZE ));

	/* Create http download thread */
	wiced_rtos_create_thread(&dev_info.http_down_thread, WICED_DEFAULT_WORKER_PRIORITY, "http download thread", http_download_handler, HTTP_DOWN_THREAD_STACK_SIZE, 0);
	return WICED_SUCCESS;
}

static unsigned int get_device_id()
{
	int i;
	unsigned int device_id = 0;
	wiced_mac_t mac;
	
	wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE );

	for(i = 3; i<6; i++) {
		device_id |= (mac.octet[i] << ((5-i)*8));
	}
	return device_id;
}

void application_start( )
{
	wiced_result_t result;
	
    /* Initialise the device and WICED framework */
    wiced_init( );

	load_app_data(&dev_info.app_dct);

	load_wifi_data(&dev_info.wifi_dct);

	memcpy(dev_info.ssid, dev_info.wifi_dct.stored_ap_list[0].details.SSID.value, \
		dev_info.wifi_dct.stored_ap_list[0].details.SSID.length);
	
	dev_info.device_id = get_device_id();
	
	WPRINT_APP_INFO(("device_id : %d\n", dev_info.device_id));

	uart_transceiver_enable();
#if 1
	wiced_rtos_create_worker_thread( &dev_info.wifi_setup_thread, WIFI_SETUP_THREAD_PRIORITY, WIFI_SETUP_STACK_SIZE, WIFI_SETUP_QUEUE_SIZE );

	if(dev_info.app_dct.work_mode == 1) {
		//wiced_rtos_send_asynchronous_event(&dev_info.wifi_setup_thread, wifi_setup, NULL);
		wifi_setup();
	}
		report_device_status();
		
		http_download_enable();
		
		/* Register callbacks */
		wiced_network_register_link_callback( link_up, link_down );
		
		//wiced_rtos_register_timed_event( &dev_info.tcp_conn_event, WICED_NETWORKING_WORKER_THREAD, &tcp_connect_handler, 1*SECONDS, 0 );
		tcp_client_enable();
#endif
}
