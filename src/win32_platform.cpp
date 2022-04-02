#include "defines.h"

#include "util.cpp"

#include "platform.h"

#include "memory.cpp"
#include "app_data.h"

#include "connect_to_twitch_chat.h"

#include <windows.h>
#include <winhttp.h>

constexpr uint32_t MAX_CONNECITONS = 5;
constexpr uint32_t MAX_REQUESTS = 50;
constexpr uint32_t FILE_IO_MEMORY_SIZE = MB(1);
constexpr uint32_t HTTP_RESPONSE_BUFFER_SIZE = KB(2);

struct WIN32HTTPState
{
    HINTERNET globalInstance;

    uint32_t connectionCount;
    HINTERNET connections[MAX_CONNECITONS];

    uint32_t requestCount;
    HINTERNET requests[MAX_REQUESTS];
};

global_variable WIN32HTTPState win32HTTPState;
global_variable uint8_t *fileIOMemory;

#include "mp3_parser.h"

#define MINIMP3_IMPLEMENTATION
#include "mp3_lib.h"

int main()
{
    // Allocate App Memory
    Memory memory = {};
    memory.memory = (uint8_t *)malloc(MB(10));
    memory.size = MB(10);

    // Memory for File IO, last Allocation, ALWAYS!
    fileIOMemory = allocate_memory(&memory, FILE_IO_MEMORY_SIZE);

    uint32_t inputBufferSize = MB(1);
    const uint8_t *inputBuffer = allocate_memory(&memory, inputBufferSize);

    // MP3 Stuff
    uint32_t fileSize = 0;
    char *mp3File = platform_read_file("assets/sounds/speech.mp3", &fileSize);
    parse_mp3(mp3File, fileSize);

    uint8_t *file = (uint8_t *)mp3File;

    // MP3 Lib Stuff
    {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);

        while (fileSize != 0)
        {
            mp3dec_frame_info_t info;
            short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

            int samples = mp3dec_decode_frame(&mp3d, file, fileSize, pcm, &info);
            printf("samples = %d\n", samples);

            fileSize -= info.frame_bytes;
            file += info.frame_bytes;
        }
    }

    // Create Windows handle to open HTTP Connections
    if (!(win32HTTPState.globalInstance = WinHttpOpen(L"PleaseJustLetMeCreateThisOkay", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, 0, 0, 0)))
    {
        CAKEZ_ASSERT(0, "Failed to open HTTP connection");

        CAKEZ_FATAL("Failed to open HTTP connection");
        return -1;
    }

    const HTTPConnection twitchID = platform_connect_to_server("id.twitch.tv");
    const HTTPConnection twitchApi = platform_connect_to_server("api.twitch.tv");

    // char *code = get_code();
    // Null terminate;
    // code[30] = 0;

    // sprintf(oAuthReq, "oauth2/token?client_id=opq32vgncw5znxeyhz4746u4v9mcet&client_secret=pxxbthehkexttz6o856uws6qtnw9n9&code=%s&grant_type=authorization_code&redirect_uri=http://localhost:49152/Test",
    //         code);

    char buffer[1024] = {};
    sprintf(buffer, URL_REFRESH_TWITCH_OAUTH_TOKEN, REFRESH_TOKEN, CLIENT_ID, CLIENT_SECRET);
    const Request refreshToken = platform_send_http_request(twitchID, buffer, 0);
    if (!platform_recieve_http_response(refreshToken))
    {
        CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", refreshToken.url);
        return -1;
    }
    if (!platform_recieve_http_data(refreshToken, buffer, 1024))
    {
        CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", refreshToken.url);
        return -1;
    }

    char token[31];
    char *data = buffer;
    while (char c = *(data++))
    {
        if (c == ':')
        {
            memcpy(token, data + 1, 30);
            break;
        }
    }

    token[30] = 0;

    // open_socket(token);

    connect_to_chat(token);

    return 0;
}

void platform_log(const char *msg, TextColor color)
{
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    uint32_t colorBits = 0;

    switch (color)
    {
    case TEXT_COLOR_WHITE:
        colorBits = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        break;

    case TEXT_COLOR_GREEN:
        colorBits = FOREGROUND_GREEN;
        break;

    case TEXT_COLOR_YELLOW:
        colorBits = FOREGROUND_GREEN | FOREGROUND_RED;
        break;

    case TEXT_COLOR_RED:
        colorBits = FOREGROUND_RED;
        break;

    case TEXT_COLOR_LIGHT_RED:
        colorBits = FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
    }

    SetConsoleTextAttribute(consoleHandle, colorBits);

#ifdef DEBUG
    OutputDebugStringA(msg);
#endif

    WriteConsoleA(consoleHandle, msg, strlen(msg), 0, 0);
}

int platform_convert_to_wchar(char *str, wchar_t *buffer, uint32_t bufferLength)
{
    CAKEZ_ASSERT(buffer, "No buffer supplied!");

    int length = str_length((const char *)str);
    if (length && (length < bufferLength))
    {
        length = MultiByteToWideChar(CP_UTF8, 0, str, length, buffer, length);
        buffer[length] = 0;
    }

    return length;
}

const HTTPConnection platform_connect_to_server(char *serverName)
{
    if (win32HTTPState.connectionCount >= MAX_CONNECITONS)
    {
        CAKEZ_ASSERT(0, "Reached maximum amount of connections!");
        return {0, INVALID_IDX};
    }

    // Conversion bullshit here
    wchar_t wServerName[50];
    platform_convert_to_wchar(serverName, wServerName, 50);
    CAKEZ_TRACE("Connecting to Server: %s", serverName);

    if (HINTERNET connection = WinHttpConnect(win32HTTPState.globalInstance,
                                              wServerName, INTERNET_DEFAULT_HTTPS_PORT, 0))
    {
        // Store connection
        win32HTTPState.connections[win32HTTPState.connectionCount] = connection;

        // increase connecction count
        return {serverName, win32HTTPState.connectionCount++};
    }
    else
    {
        CAKEZ_ERROR("Failed to connect to Server: %s", serverName);
        DWORD err = GetLastError();

        switch (err)
        {
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
            CAKEZ_ASSERT(0, "Incorrect Handle Type");
            break;
        case ERROR_WINHTTP_INTERNAL_ERROR:
            CAKEZ_ASSERT(0, "Winhttp Internal Error");
            break;
        case ERROR_WINHTTP_INVALID_URL:
            CAKEZ_ASSERT(0, "Invalid URL");
            break;
        case ERROR_WINHTTP_OPERATION_CANCELLED:
            CAKEZ_ASSERT(0, "Operation cancelled");
            break;
        case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
            CAKEZ_ASSERT(0, "Unrecognized Scheme");
            break;
        case ERROR_WINHTTP_SHUTDOWN:
            CAKEZ_ASSERT(0, "Winhttp Shutdown");
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            CAKEZ_ASSERT(0, "Not enough Memory");
            break;
        }

        return {0, INVALID_IDX};
    }
}

const Request platform_send_http_request(const HTTPConnection connection, char *url, char *header, char *method, char *data)
{
    CAKEZ_ASSERT(url, "URL required");

    if (connection.connectionIdx >= MAX_CONNECITONS)
    {
        CAKEZ_ASSERT(0, "Connection Index: %d out of Bounds", connection.connectionIdx);
        return {0, 0, 0, INVALID_IDX};
    }

    if (win32HTTPState.requestCount >= MAX_REQUESTS)
    {
        CAKEZ_ASSERT(0, "Reached maximum Amount of Requests");
        return {0, 0, 0, INVALID_IDX};
    }

    // Conversion bullshit here
    wchar_t wUrl[300];
    int length = platform_convert_to_wchar(url, wUrl, 300);

    wchar_t wHeader[300];
    int headerLength = platform_convert_to_wchar(header, wHeader, 300);

    wchar_t wMethod[300];
    int methodLength = platform_convert_to_wchar(method, wMethod, 300);

    wchar_t wData[500];
    int dataLength = platform_convert_to_wchar(data, wData, 500);

    HINTERNET connectionHandle = win32HTTPState.connections[connection.connectionIdx];

    // This uses HTTP/1.1
    HINTERNET winRequest = 0;
    if (!(winRequest = WinHttpOpenRequest(connectionHandle, wMethod, wUrl, 0,
                                          WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          WINHTTP_FLAG_SECURE)))
    {
        CAKEZ_ASSERT(0, "Couldn't open HTTP Request");
        return {0, 0, 0, INVALID_IDX};
    }

    if (WinHttpSendRequest(winRequest, headerLength ? wHeader : 0,
                           0, dataLength ? data : 0, dataLength, dataLength, 0))
    {
        win32HTTPState.requests[win32HTTPState.requestCount] = winRequest;
        return {url, method, header, win32HTTPState.requestCount++};
    }
    else
    {
        DWORD err = GetLastError();
        switch (err)
        {
        case ERROR_WINHTTP_CANNOT_CONNECT:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_CONNECTION_ERROR:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_INTERNAL_ERROR:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_INVALID_URL:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_LOGIN_FAILURE:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_NAME_NOT_RESOLVED:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_OPERATION_CANCELLED:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_SECURE_FAILURE:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_SHUTDOWN:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_TIMEOUT:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_INVALID_PARAMETER:
            CAKEZ_ASSERT(0, 0);
            break;
        case ERROR_WINHTTP_RESEND_REQUEST:
            CAKEZ_ASSERT(0, 0);
            break;
        }
        CAKEZ_ASSERT(0, 0);
        return {0, 0, 0, INVALID_IDX};
    }
}

bool platform_recieve_http_response(const Request request)
{
    if (!(request.requestIdx < win32HTTPState.requestCount))
    {
        CAKEZ_ASSERT(0, "Request Index: %d, out of Bounds: %d", request.requestIdx, win32HTTPState.requestCount);
        return false;
    }

    HINTERNET winRequest = win32HTTPState.requests[request.requestIdx];

    return WinHttpReceiveResponse(winRequest, 0);
}

bool platform_recieve_http_data(const Request request, char *outBuffer, uint32_t bufferSize)
{
    if (!(request.requestIdx < win32HTTPState.requestCount))
    {
        CAKEZ_ASSERT(0, "Request Index: %d, out of Bounds: %d", request.requestIdx, win32HTTPState.requestCount);
        return false;
    }

    HINTERNET winRequest = win32HTTPState.requests[request.requestIdx];

    DWORD bytesRecieved;
    if (!WinHttpQueryDataAvailable(winRequest, &bytesRecieved) && bytesRecieved || !bytesRecieved)
    {
        return false;
    }

    // Clear the buffer to 0
    memset(outBuffer, 0, bufferSize);

    DWORD bytesRead;
    if (!WinHttpReadData(winRequest, outBuffer, bytesRecieved, &bytesRead))
    {
        CAKEZ_ASSERT(0, "Unable to read data");
        return false;
    }

    CAKEZ_TRACE(outBuffer);
    return true;
}

char *platform_read_file(char *path, uint32_t *length)
{
    char *buffer = 0;
    HANDLE file = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0, 0);

    if (file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(file, &fileSize))
        {
            *length = (uint32_t)fileSize.QuadPart;
            CAKEZ_ASSERT(*length <= FILE_IO_MEMORY_SIZE, "File size: %d, too large for File IO Memory: %d",
                         *length, FILE_IO_MEMORY_SIZE);

            // Clear contents of File IO Memory
            memset(fileIOMemory, 0, FILE_IO_MEMORY_SIZE);
            buffer = (char *)fileIOMemory;

            DWORD bytesRead;
            if (ReadFile(file, buffer, *length, &bytesRead, 0) &&
                *length == bytesRead)
            {
                //TODO: What can I do here?
            }
            else
            {
                CAKEZ_ASSERT(0, "Unable to read file: %s", path);
                CAKEZ_ERROR("Unable to read file: %s", path);
                buffer = 0;
            }
        }
        else
        {
            CAKEZ_ASSERT(0, "Unable to get size of file: %s", path);
            CAKEZ_ERROR("Unable to get size of file: %s", path);
        }

        CloseHandle(file);
    }
    else
    {
        CAKEZ_ASSERT(0, "Unable to open file: %s", path);
        CAKEZ_ERROR("Unable to open file: %s", path);
    }

    return buffer;
}

unsigned long platform_write_file(
    const char *path,
    const char *buffer,
    const uint32_t size,
    bool overwrite)
{
    DWORD bytesWritten = 0;

    HANDLE file = CreateFile(
        path,
        overwrite ? GENERIC_WRITE : FILE_APPEND_DATA,
        FILE_SHARE_WRITE,
        0,
        OPEN_ALWAYS,
        0, 0);

    if (file != INVALID_HANDLE_VALUE)
    {
        if (!overwrite)
        {
            DWORD result = SetFilePointer(file, 0, 0, FILE_END);
            if (result == INVALID_SET_FILE_POINTER)
            {
                CAKEZ_ASSERT(0, "Unable to set pointer in file: %s", path);
                CAKEZ_ERROR("Unable to set pointer in file: %s", path);
            }
        }

        BOOL result = WriteFile(file, buffer, size, &bytesWritten, 0);
        if (result && size == bytesWritten)
        {
            //Success
        }
        else
        {
            CAKEZ_ASSERT(0, "Unable to write file: %s", path);
            CAKEZ_ERROR("Unable to write file: %s", path);
        }

        CloseHandle(file);
    }
    else
    {
        CAKEZ_ASSERT(0, "Unable to open file: %s", path);
        CAKEZ_ERROR("Unable to open file: %s", path);
    }

    return bytesWritten;
}