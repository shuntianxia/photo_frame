#ifndef _HTTP_DOWNLOAD_H_
#define _HTTP_DOWNLOAD_H_

#define URL_LEN 512
#define FILENAME_LEN 512
#define PATH_LEN 512
#define DEBUG_HTTP
#define DEBUG_HTTP_RECV

#define NO_CONTENT_LENGTH 0

#define HTTP_HEADER_200                "HTTP/1.1 200 OK"
#define HTTP_HEADER_204                "HTTP/1.1 204 No Content"
#define HTTP_HEADER_207                "HTTP/1.1 207 Multi-Status"
#define HTTP_HEADER_301                "HTTP/1.1 301"
#define HTTP_HEADER_400                "HTTP/1.1 400 Bad Request"
#define HTTP_HEADER_403                "HTTP/1.1 403"
#define HTTP_HEADER_404                "HTTP/1.1 404 Not Found"
#define HTTP_HEADER_405                "HTTP/1.1 405 Method Not Allowed"
#define HTTP_HEADER_406                "HTTP/1.1 406"
#define HTTP_HEADER_429                "HTTP/1.1 429 Too Many Requests"
#define HTTP_HEADER_444                "HTTP/1.1 444"
#define HTTP_HEADER_470                "HTTP/1.1 470 Connection Authorization Required"
#define HTTP_HEADER_500                "HTTP/1.1 500 Internal Server Error"
#define HTTP_HEADER_CONTENT_LENGTH     "Content-Length: "
#define HTTP_HEADER_CONTENT_TYPE       "Content-Type: "
#define HTTP_HEADER_CHUNKED            "Transfer-Encoding: chunked"
#define HTTP_HEADER_LOCATION           "Location: "
#define HTTP_HEADER_ACCEPT             "Accept: "
#define HTTP_HEADER_KEEP_ALIVE         "Connection: Keep-Alive"
#define HTTP_HEADER_CLOSE              "Connection: close"
#define NO_CACHE_HEADER                "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\n"\
                                       "Pragma: no-cache"

#define CRLF                            "\r\n"
#define CRLF_CRLF                       "\r\n\r\n"

typedef enum
{
    HTTP_HEADER_AND_DATA_FRAME_STATE,
    HTTP_DATA_ONLY_FRAME_STATE,
} http_packet_state_t;

typedef struct url_info {
	char proto[16];
	char host[128];
	char ip[56];
	wiced_ip_address_t ip_addr;
	int port;
	char path[256];
	wiced_time_t timestamp;
}url_info_t;

typedef struct
{
	char                   buffer[3*1024];
    const uint8_t*            data;                      /* packet data in message body      */
	uint32_t                  message_head_length;       /* data length in current packet    */
    uint32_t                  total_message_data;		 /* total data size  */
	//wiced_tcp_socket_t        socket;
    wiced_packet_t*           rx_packet;
	uint32_t byte_start;
	uint32_t byte_pos;
	uint8_t  qrcode_flag;
    //wiced_bool_t              chunked_transfer;          /* chunked data format              */
    //wiced_packet_mime_type_t  mime_type;                 /* mime type                        */
} http_message_t;

wiced_result_t http_download(char *url, int qrcode_flag);


#endif
