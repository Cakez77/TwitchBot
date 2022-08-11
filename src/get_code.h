#pragma once

#include "url_encode.h"

#include <windows.h>
#include <http.h>

uint32_t constexpr requestBufferLength = sizeof(HTTP_REQUEST) + 4096;

char *get_code()
{
	//TODO: BAD!
	char *code = new char[40];
	
	HANDLE handle = 0;
	CAKEZ_ASSERT(HttpInitialize(HTTPAPI_VERSION_1, HTTP_INITIALIZE_SERVER, 0) == NO_ERROR, "Failed to initialize HTTP");
	
	CAKEZ_ASSERT(HttpCreateHttpHandle(&handle, 0) == NO_ERROR, "Failed to create HTTP Handle");
	
	ULONG result = HttpAddUrl(handle, L"http://localhost:49152/Test", 0);
	if (result != NO_ERROR)
	{
		switch (result)
		{
			case ERROR_ACCESS_DENIED:
			CAKEZ_ASSERT(0, "ERROR_ACCESS_DENIED");
			break;
			case ERROR_DLL_INIT_FAILED:
			CAKEZ_ASSERT(0, "ERROR_DLL_INIT_FAILED");
			break;
			case ERROR_INVALID_PARAMETER:
			CAKEZ_ASSERT(0, "ERROR_INVALID_PARAMETER");
			break;
			case ERROR_ALREADY_EXISTS:
			CAKEZ_ASSERT(0, "ERROR_ALREADY_EXISTS");
			break;
			case ERROR_NOT_ENOUGH_MEMORY:
			CAKEZ_ASSERT(0, "ERROR_NOT_ENOUGH_MEMORY");
			break;
		}
	}
	
	ShellExecute(0, "open", "\"https://id.twitch.tv/oauth2/authorize?response_type=code&client_id=opq32vgncw5znxeyhz4746u4v9mcet&redirect_uri=http://localhost:49152/Test&scope=chat:edit+chat:read+channel:manage:redemptions\"",
							 0, 0, SW_SHOWMINIMIZED);
	
	HTTP_REQUEST_ID requestID;
	char buffer[requestBufferLength];
	bool running = true;
	
	while (running)
	{
		HTTP_SET_NULL_ID(&requestID);
		ULONG bytesRead;
		PHTTP_REQUEST pRequest = (PHTTP_REQUEST)buffer;
		ULONG result = HttpReceiveHttpRequest(handle, requestID, 0, pRequest, 
																					requestBufferLength, &bytesRead, 0);
		CAKEZ_ASSERT(result == NO_ERROR, "Could not recieve HTTP Request");
		
		if (pRequest->BytesReceived)
		{
			switch (pRequest->Verb)
			{
				case HttpVerbGET:
				{
					char *responseURL = (char *)pRequest->pRawUrl;
					
					while (char c = *(responseURL++))
					{
						if (c == '=')
						{
							memcpy(code, responseURL, 30);
							break;
						}
					}
					
					HTTP_DATA_CHUNK responeBody = {};
					responeBody.DataChunkType = HttpDataChunkFromMemory;
					responeBody.FromMemory.BufferLength = 21;
					responeBody.FromMemory.pBuffer = "Everything went okay!";
					
					HTTP_RESPONSE response = {};
					response.pReason = "OK";
					response.ReasonLength = 2;
					response.StatusCode = 200;
					response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/plain";
					response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = 10;
					response.EntityChunkCount = 1;
					response.pEntityChunks = &responeBody;
					ULONG bytesSend;
					
					ULONG result = HttpSendHttpResponse(handle, pRequest->RequestId, 0, 
																							&response, 0, &bytesSend, 0, 0, 0, 0);
					CAKEZ_ASSERT(result == NO_ERROR, "Failed to send HTTP Response");
					
					break;
				}
			}
			
			running = false;
		}
	}
	return code;
}