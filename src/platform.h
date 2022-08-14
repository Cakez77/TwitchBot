#pragma once

#include "sound.h"
#include "defines.h"

enum TextColor
{
	TEXT_COLOR_WHITE,
	TEXT_COLOR_GREEN,
	TEXT_COLOR_YELLOW,
	TEXT_COLOR_RED,
	TEXT_COLOR_LIGHT_RED,
};

struct HTTPConnection
{
	char *name;
	uint32_t connectionID;
};

struct Request
{
	char *url;
	char *method;
	char *header;
	void* httpHandle;
};

HTTPConnection platform_connect_to_server(char *serverName, bool https = true);

Request platform_send_http_request(HTTPConnection connection, char *url, 
																	 char *header, char *method = "POST", char *data = 0,
																	 bool secure = true);

bool platform_recieve_http_response(Request request);

bool platform_receive_http_data(Request request, char *outBuffer, uint32_t bufferSize, 
																uint32_t *outReceivedBytes = 0);

void platform_close_http_request(Request request);

bool platform_add_http_poll_url(char* url);

void platform_log(char* msg, TextColor color);

bool platform_file_exists(char *path);
char *platform_read_file(char *path, uint32_t *length);
bool platform_delete_file(char *path);

unsigned long platform_write_file(char *path,
																	char *buffer,
																	uint32_t size,
																	bool overwrite);

void platform_sleep(int ms);
uint64_t platform_get_performance_tick_count();
