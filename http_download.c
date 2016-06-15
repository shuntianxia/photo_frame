#include "wiced.h"
#include "http_download.h"
#include "photo_frame.h"
#include "utils.h"
#include "cJSON.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define MAX_RECV_SIZE    8192
#define TCP_CLIENT_INTERVAL               2
#define TCP_CLIENT_CONNECT_TIMEOUT        500
#define TCP_CONNECTION_NUMBER_OF_RETRIES  3
#define MINIMUM_REQUEST_LINE_LENGTH   (18)
#define COMPARE_MATCH                 (0)


/******************************************************
 *                    Constants
 ******************************************************/

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


/******************************************************
 *               Variable Definitions
 ******************************************************/
static url_info_t url_info;
//static queue_t queue;
static http_message_t http_message;
extern dev_info_t dev_info;

/******************************************************
 *               Function Definitions
 ******************************************************/
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

wiced_result_t parse_url(const char*url, url_info_t *url_info)
{
	const	char*postemp;
	const	char*postemp2;
	const	char*poshost;//IP定位
	const	char*pospath;//filepath定位
	postemp=strstr(url,"://");
	if (postemp==NULL){
		strcpy(url_info->proto,"http");
		poshost=url;
	}else{
		memcpy(url_info->proto,url,postemp-url);
		poshost=postemp+3;
	}
	
	postemp=strstr(poshost,"/");
	postemp2=strstr(poshost,":");
	if (postemp==NULL){
		if (postemp2==NULL){
			strcpy(url_info->host,poshost);
			url_info->port = 80;
		}else{
			strncpy(url_info->host,poshost,postemp2-poshost);
			sscanf(postemp2+1,"%d", &url_info->port);
		}
		strcpy(url_info->path,"/");
		//strcpy(url_info->filename,"undefine");
	}else{
		pospath=postemp;
		if (postemp2==NULL||(postemp2>pospath)){
			strncpy(url_info->host,poshost,postemp-poshost);
			url_info->port = 80;
		}else{
			strncpy(url_info->host,poshost,postemp2-poshost);
			sscanf(postemp2+1,"%d",&url_info->port);
		}
		strcpy(url_info->path,pospath);
#if 0
		postemp+=1;
		postemp2=strstr(postemp,"/");
		while(postemp2!=NULL){
			postemp=postemp2+1;
			postemp2=strstr(postemp,"/");
		}
		strcpy(url_info->filename,postemp);
#endif
	}
	wiced_time_get_time(&url_info->timestamp);
	
	WICED_VERIFY(wiced_hostname_lookup(url_info->host, &url_info->ip_addr, 1000));

	sprintf(url_info->ip,"%hhu.%hhu.%hhu.%hhu",
		(unsigned char) (GET_IPV4_ADDRESS(url_info->ip_addr) >> 24),
		(unsigned char) (GET_IPV4_ADDRESS(url_info->ip_addr) >> 16),
		(unsigned char) (GET_IPV4_ADDRESS(url_info->ip_addr) >> 8),
		(unsigned char) (GET_IPV4_ADDRESS(url_info->ip_addr) >> 0));
	//WPRINT_APP_INFO(("PRORT:%s\nhostname:%s\nIP:%s\nPort:%d\npath:%s\n", \
		url_info->proto,url_info->host,url_info->ip,url_info->port,url_info->path));
	return 0;
}

wiced_result_t send_http_request(wiced_tcp_socket_t* socket)
{
	char send_buf[1024];
	char port[10];
	
	memset(send_buf, 0x0, sizeof(send_buf));		   
	sprintf(send_buf, "GET %s",url_info.path);
	strcat(send_buf," HTTP/1.1\r\n");
	strcat(send_buf, "Host: ");
	strcat(send_buf, url_info.host);
	//strcat(send_buf, ":");
	//sprintf(port, "%d", url_info.port);
	//strcat(send_buf, port);
	strcat(send_buf, "\r\nKeep-Alive: 200");
	strcat(send_buf,"\r\nConnection: Keep-Alive\r\n\r\n");
	
	WICED_VERIFY(wiced_tcp_send_buffer(socket, send_buf, strlen(send_buf)));
	return WICED_SUCCESS;
}

static wiced_result_t receive_http_response(wiced_tcp_socket_t* socket)
{
	uint8_t*                      rx_data;
	uint16_t                      rx_data_length;
	uint16_t                      available_data_length;
    char*                         message_data_length_string;
    char*                         mime;
    wiced_result_t                result                 = WICED_ERROR;
	uint16_t amount_to_read;
	//uint32_t timeout;
	cJSON *root, *file_info;
	char *out;
	int head_len;
	uint32_t total_send_byte = 0;

	result = wiced_tcp_receive(socket, &http_message.rx_packet, 500);
	if ( result != WICED_TCPIP_SUCCESS)
	{
		WPRINT_APP_INFO(("TCP receive failed\n"));
		return result;
	}
	WICED_VERIFY( wiced_packet_get_data(http_message.rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length));
	
	/* Verify we have enough data to start processing */
	if ( rx_data_length < MINIMUM_REQUEST_LINE_LENGTH )
	{
		WPRINT_APP_INFO(("rx_data_length < MINIMUM_REQUEST_LINE_LENGTH\n"));
		return WICED_ERROR;
	}
	
	/* Now extract packet payload info such as data, data length, data type and message length */
	http_message.data = (uint8_t*)strstr( (char*)rx_data, CRLF_CRLF );
	
	/* This indicates start of data/end of header was not found, so exit */
	if ( http_message.data  == NULL)
	{
		WPRINT_APP_INFO(("http_message.data == NULL\n"));
		return WICED_ERROR;
	}
	else
	{
		/* Payload starts just after the header */
		http_message.data += strlen( CRLF_CRLF );
		http_message.message_head_length = http_message.data - rx_data;
	}
	
	mime = strstr((char*)rx_data, HTTP_HEADER_CONTENT_TYPE );
	if ( ( mime != NULL ) && ( mime < (char*)http_message.data ) )
	{
		mime+=strlen(HTTP_HEADER_CONTENT_TYPE);
		//http_message.mime_type = wiced_get_mime_type( mime );
	}
	else
	{
		//http_message.mime_type = MIME_TYPE_ALL;
		//待添加代码
	}

	message_data_length_string = strstr((char*)rx_data, HTTP_HEADER_CONTENT_LENGTH );
	
	if ( ( message_data_length_string != NULL ) &&
		 ( message_data_length_string < (char*)http_message.data ) )
	{
		message_data_length_string += ( sizeof( HTTP_HEADER_CONTENT_LENGTH ) -1 );
		http_message.total_message_data = strtoul( message_data_length_string, NULL, 10 );
		WPRINT_APP_INFO(("total_message_data = %lu\n", http_message.total_message_data));
		
		/* Otherwise update the start of the data for the next read request */
		wiced_packet_set_data_start(http_message.rx_packet, rx_data + http_message.message_head_length);
	}
	else
	{
		return WICED_ERROR;
		//http_message.message_data_length = 0;
	}

    while ( http_message.byte_pos < http_message.total_message_data)
	{
		/* Check if we don't have a packet */
        if (http_message.rx_packet == NULL)
        {
            result = wiced_tcp_receive(socket, &http_message.rx_packet, 3000);
            if ( result != WICED_TCPIP_SUCCESS)
            {
				WPRINT_APP_INFO(("wiced_tcp_receive error, result = %d\n", result));
                continue;
            } else if (result == WICED_TIMEOUT) {

            } else if (result == WICED_SUCCESS) {

            }
        }
		if((result = wiced_packet_get_data(http_message.rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length)) != WICED_SUCCESS) {
				WPRINT_APP_INFO(("wiced_packet_get_data error. result = %d\n", result));
		}

		/* Read data */
        amount_to_read = MIN(sizeof(http_message.buffer) - (http_message.byte_pos - http_message.byte_start), rx_data_length);
        //buffer         = MEMCAT((uint8_t*)buffer, rx_data, amount_to_read);
		memcpy(&http_message.buffer[http_message.byte_pos - http_message.byte_start], rx_data, amount_to_read);
		http_message.byte_pos += amount_to_read;
		//WPRINT_APP_INFO(("byte_pos = %d\n", http_message.byte_pos));

        /* Update buffer length */
        //buffer_length = (uint16_t)(buffer_length - amount_to_read);

        /* Check if we need a new packet */
        if ( amount_to_read == rx_data_length )
        {
			//WPRINT_APP_INFO(("amount_to_read == rx_data_length\n"));
            wiced_packet_delete( http_message.rx_packet );
            http_message.rx_packet = NULL;
        }
        else
        {
			//WPRINT_APP_INFO(("amount_to_read != rx_data_length\n"));
            /* Otherwise update the start of the data for the next read request */
            wiced_packet_set_data_start(http_message.rx_packet, rx_data + amount_to_read);
        }
        //http_message.data = (uint8_t*)rx_data;

		if((http_message.byte_pos - http_message.byte_start) == sizeof(http_message.buffer) || (http_message.byte_pos == http_message.total_message_data)) {
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, "method", "transfer_file");
			cJSON_AddItemToObject(root, "file_info", file_info =cJSON_CreateObject());
			cJSON_AddNumberToObject(file_info, "timestamp", url_info.timestamp);
			cJSON_AddNumberToObject(file_info, "file_size", http_message.total_message_data);
			//cJSON_AddStringToObject(file_info, "file_type", url_info->filetype);
			cJSON_AddNumberToObject(file_info, "qrcode_flag", http_message.qrcode_flag);
			cJSON_AddNumberToObject(file_info, "byte_start", http_message.byte_start);
			cJSON_AddNumberToObject(file_info, "byte_end", http_message.byte_pos - 1);
			
			out=cJSON_Print(root);
			cJSON_Delete(root);
			//printf("%s\n",out);
			//*len = strlen(out) + 4;
			uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
			if(uart_frame != NULL) {
				memset(uart_frame, 0, sizeof(uart_frame_t));
				sprintf(uart_frame->data, "%s\r\n\r\n", out);
				head_len = strlen(uart_frame->data);
				memcpy(uart_frame->data + head_len, http_message.buffer, http_message.byte_pos - http_message.byte_start);
				uart_frame->len = head_len + http_message.byte_pos - http_message.byte_start;
				//WPRINT_APP_INFO(("head_len = %u, data_len = %u\n", head_len, http_message.byte_pos - http_message.byte_start));
				total_send_byte += uart_frame->len;
				WPRINT_APP_INFO(("downloaded : %u/%u, total_send_byte : %u.\n", http_message.byte_pos, http_message.total_message_data, total_send_byte));
			} else {
				WPRINT_APP_INFO(("malloc failed\n"));
				free(out);
				return WICED_ERROR;
			}
			free(out);
			if(wiced_rtos_push_to_queue(&dev_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT) != WICED_SUCCESS) {
				WPRINT_APP_INFO(("push to uart_send_queue error\n"));
			}
			http_message.byte_start = http_message.byte_pos;
		}
    }
	WPRINT_APP_INFO(("file transfer finished.\n"));
    return WICED_SUCCESS;
}

static wiced_result_t receive_http_response_test(wiced_tcp_socket_t* socket)
{
	uint8_t*                      rx_data;
	uint16_t                      rx_data_length;
	uint16_t                      available_data_length;
    char*                         message_data_length_string;
    char*                         mime;
    wiced_result_t                result                 = WICED_ERROR;
	uint16_t amount_to_read;
	//uint32_t timeout;
	cJSON *root, *file_info;
	char *out;
	int head_len;
	int i = '0';
	uint32_t total_send_byte = 0;

	result = wiced_tcp_receive(socket, &http_message.rx_packet, 500);
	if ( result != WICED_TCPIP_SUCCESS)
	{
		WPRINT_APP_INFO(("TCP receive failed\n"));
		return result;
	}
	WICED_VERIFY( wiced_packet_get_data(http_message.rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length));
	
	/* Verify we have enough data to start processing */
	if ( rx_data_length < MINIMUM_REQUEST_LINE_LENGTH )
	{
		WPRINT_APP_INFO(("rx_data_length < MINIMUM_REQUEST_LINE_LENGTH\n"));
		return WICED_ERROR;
	}
	
	/* Now extract packet payload info such as data, data length, data type and message length */
	http_message.data = (uint8_t*)strstr( (char*)rx_data, CRLF_CRLF );
	
	/* This indicates start of data/end of header was not found, so exit */
	if ( http_message.data  == NULL)
	{
		WPRINT_APP_INFO(("http_message.data == NULL\n"));
		return WICED_ERROR;
	}
	else
	{
		/* Payload starts just after the header */
		http_message.data += strlen( CRLF_CRLF );
		http_message.message_head_length = http_message.data - rx_data;
	}
	
	mime = strstr((char*)rx_data, HTTP_HEADER_CONTENT_TYPE );
	if ( ( mime != NULL ) && ( mime < (char*)http_message.data ) )
	{
		mime+=strlen(HTTP_HEADER_CONTENT_TYPE);
		//http_message.mime_type = wiced_get_mime_type( mime );
	}
	else
	{
		//http_message.mime_type = MIME_TYPE_ALL;
		//待添加代码
	}

	message_data_length_string = strstr((char*)rx_data, HTTP_HEADER_CONTENT_LENGTH );
	
	if ( ( message_data_length_string != NULL ) &&
		 ( message_data_length_string < (char*)http_message.data ) )
	{
		message_data_length_string += ( sizeof( HTTP_HEADER_CONTENT_LENGTH ) -1 );
		http_message.total_message_data = strtoul( message_data_length_string, NULL, 10 );
		WPRINT_APP_INFO(("total_message_data = %lu\n", http_message.total_message_data));
		
		/* Otherwise update the start of the data for the next read request */
		wiced_packet_set_data_start(http_message.rx_packet, rx_data + http_message.message_head_length);

		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "method", "transfer_file");
		cJSON_AddItemToObject(root, "file_info", file_info =cJSON_CreateObject());
		//cJSON_AddNumberToObject(file_info, "timestamp", url_info.timestamp);
		cJSON_AddNumberToObject(file_info, "file_size", http_message.total_message_data);
		//cJSON_AddStringToObject(file_info, "file_type", url_info->filetype);
		cJSON_AddNumberToObject(file_info, "qrcode_flag", http_message.qrcode_flag);
		
		out=cJSON_Print(root);
		cJSON_Delete(root);
		//printf("%s\n",out);
		//*len = strlen(out) + 4;
		uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
		if(uart_frame != NULL) {
			memset(uart_frame, 0, sizeof(uart_frame_t));
			sprintf(uart_frame->data, "%s\r\n\r\n", out);
			head_len = strlen(uart_frame->data);
			uart_frame->len = head_len ;
			total_send_byte += uart_frame->len;
		} else {
			WPRINT_APP_INFO(("malloc failed\n"));
			free(out);
			return WICED_ERROR;
		}
		free(out);
		if(wiced_rtos_push_to_queue(&dev_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT) != WICED_SUCCESS) {
			WPRINT_APP_INFO(("push to uart_send_queue error\n"));
		}
	}
	else
	{
		return WICED_ERROR;
		//http_message.message_data_length = 0;
	}

    while ( http_message.byte_pos < http_message.total_message_data)
	{
		/* Check if we don't have a packet */
        if (http_message.rx_packet == NULL)
        {
            result = wiced_tcp_receive(socket, &http_message.rx_packet, 3000);
            if ( result != WICED_TCPIP_SUCCESS)
            {
				WPRINT_APP_INFO(("wiced_tcp_receive error, result = %d\n", result));
                continue;
            } else if (result == WICED_TIMEOUT) {

            } else if (result == WICED_SUCCESS) {

            }
        }
		if((result = wiced_packet_get_data(http_message.rx_packet, 0, (uint8_t**)&rx_data, &rx_data_length, &available_data_length)) != WICED_SUCCESS) {
				WPRINT_APP_INFO(("wiced_packet_get_data error. result = %d\n", result));
		}

		/* Read data */
        amount_to_read = MIN(sizeof(http_message.buffer) - (http_message.byte_pos - http_message.byte_start), rx_data_length);
        //buffer         = MEMCAT((uint8_t*)buffer, rx_data, amount_to_read);
		memcpy(&http_message.buffer[http_message.byte_pos - http_message.byte_start], rx_data, amount_to_read);
		http_message.byte_pos += amount_to_read;
		//WPRINT_APP_INFO(("byte_pos = %d\n", http_message.byte_pos));

        /* Update buffer length */
        //buffer_length = (uint16_t)(buffer_length - amount_to_read);

        /* Check if we need a new packet */
        if ( amount_to_read == rx_data_length )
        {
			//WPRINT_APP_INFO(("amount_to_read == rx_data_length\n"));
            wiced_packet_delete( http_message.rx_packet );
            http_message.rx_packet = NULL;
        }
        else
        {
			//WPRINT_APP_INFO(("amount_to_read != rx_data_length\n"));
            /* Otherwise update the start of the data for the next read request */
            wiced_packet_set_data_start(http_message.rx_packet, rx_data + amount_to_read);
        }
        //http_message.data = (uint8_t*)rx_data;

		if((http_message.byte_pos - http_message.byte_start) == sizeof(http_message.buffer) || (http_message.byte_pos == http_message.total_message_data)) {
			uart_frame_t *uart_frame = malloc(sizeof(uart_frame_t));
			if(uart_frame != NULL) {
				//memset(uart_frame, 0, sizeof(uart_frame_t));
				//memcpy(uart_frame->data, http_message.buffer, http_message.byte_pos - http_message.byte_start);
				memset(uart_frame, i++, sizeof(uart_frame_t));
				if(i>'9')
					i = '0';
				uart_frame->len = http_message.byte_pos - http_message.byte_start;
				total_send_byte += uart_frame->len;
				WPRINT_APP_INFO(("downloaded : %u/%u, total_send_byte : %u.\n", http_message.byte_pos, http_message.total_message_data, total_send_byte));
				//WPRINT_APP_INFO(("downloaded : %u/%u\n", http_message.byte_pos, http_message.total_message_data));
			} else {
				WPRINT_APP_INFO(("malloc failed\n"));
				return WICED_ERROR;
			}
			if(wiced_rtos_push_to_queue(&dev_info.uart_send_queue, &uart_frame, WICED_NEVER_TIMEOUT) != WICED_SUCCESS) {
				WPRINT_APP_INFO(("push to uart_send_queue error\n"));
			}
			http_message.byte_start = http_message.byte_pos;
		}
    }
	WPRINT_APP_INFO(("file transfer finished.\n\n"));
    return WICED_SUCCESS;
}

wiced_result_t http_download(char *url, int qrcode_flag)
{
	WPRINT_APP_INFO(("http_download start\n"));
	memset(&url_info, 0, sizeof(url_info));
    WICED_VERIFY(parse_url(url,&url_info));
	memset(&http_message, 0, sizeof(http_message));
	http_message.qrcode_flag = qrcode_flag;

	/* Create a TCP socket */
    if (wiced_tcp_create_socket(&dev_info.http_download_socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("TCP socket creation failed\n"));
		return WICED_ERROR;
    }
	
    //建立连接
    if(tcp_connect(&dev_info.http_download_socket, &url_info.ip_addr, url_info.port)!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("TCP connect failed\n"));
		return WICED_ERROR;
    }

	if(send_http_request(&dev_info.http_download_socket) != WICED_SUCCESS) {
		WPRINT_APP_INFO(("send_http_request failed\n"));
		return WICED_ERROR;
	}

	if(receive_http_response(&dev_info.http_download_socket) != WICED_SUCCESS) {
		WPRINT_APP_INFO(("receive_http_response failed\n"));
		return WICED_ERROR;
	}

	//断开连接
    tcp_disconnect(&dev_info.http_download_socket);

	/* Delete a TCP socket */
	wiced_tcp_delete_socket(&dev_info.http_download_socket);
	return WICED_SUCCESS;
}
