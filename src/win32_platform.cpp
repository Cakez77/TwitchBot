
#include "defines.h"
#include "logger.h"
#include "util.cpp"
#include "json_parser.cpp"
#include "memory.cpp"

#include "platform.h"

#include "twitch_events.cpp"

#include "connect_to_twitch_chat.h"

#include "get_code.h"

#include "sound.h"

// Windows
#include <windows.h>
#include <winhttp.h>
#include <dsound.h>

#include <thread>
#include <time.h>


//#######################################################################
//                      Internal Structures
//#######################################################################

// File I/O
constexpr uint32_t FILE_IO_MEMORY_SIZE = MB(1);

// HTTP 
constexpr uint32_t HTTP_RESPONSE_BUFFER_SIZE = KB(2);
constexpr uint32_t MAX_SERVER_CONNECTIONS = 10;
constexpr uint32_t MAX_REQUESTS = 50;
constexpr uint32_t MAX_POLL_URL_COUNT = 5;

// Sound
constexpr uint32_t SOUND_BUFFER_SIZE = MB(10);
constexpr uint32_t MAX_ALLOCATED_SOUNDS = 20;
constexpr uint32_t MAX_PLAYING_SOUNDS = 5;

struct WIN32HTTPState
{
	HANDLE HTTPHandle;
	
	uint32_t pollingUrlCount;
	char* pollingUrls[MAX_POLL_URL_COUNT];
	
	uint32_t requestIDCounter;
	HINTERNET globalInstance;
	
	uint32_t connectionCount;
	HINTERNET connections[MAX_SERVER_CONNECTIONS];
	
	uint32_t requestCount;
	Request requests[MAX_REQUESTS];
};

struct SoundState
{
	uint32_t allocatedSoundCount;
	Sound allocatedSounds[MAX_ALLOCATED_SOUNDS];
	
	uint32_t playingSoundCount;
	Sound playingSounds[MAX_PLAYING_SOUNDS];
	uint32_t oldPlayCursor;
	uint32_t oldWriteCursor;
	
	// Currently 
	uint32_t soundBufferSize;
	uint32_t allocatedByteCount;
	char* soundBuffer;
	
  uint32_t mixBufferSize;
	char* mixBuffer;
};

//#######################################################################
//                      Internal Data
//#######################################################################
global_variable WIN32HTTPState win32HTTPState;
global_variable uint8_t *fileIOMemory;

// Sound
global_variable SoundState* soundState;
global_variable HTTPConnection localApi;

LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_HOTKEY:
		{
			if (wParam == VK_F2)
			{
				if (soundState->playingSoundCount)
				{
					soundState->playingSoundCount = 0;
				}
			}
		}
	}
	
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

global_variable HWND window;
bool platform_create_window(int width, int height, char *title)
{
	HINSTANCE instance = GetModuleHandleA(0);
	
	// Setup and register window class
	HICON icon = LoadIcon(instance, IDI_APPLICATION);
	WNDCLASS wc = {};
	wc.lpfnWndProc = window_callback;
	wc.hInstance = instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); // NULL; => Manage the cursor manually
	wc.hCursor = LoadCursorFromFileA("cursor.cur");
	wc.lpszClassName = "cakez_window_class";
	
	if (!RegisterClassA(&wc))
	{
		MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	
	// Create window
	uint32_t client_x = 100;
	uint32_t client_y = 100;
	uint32_t client_width = width;
	uint32_t client_height = height;
	
	uint32_t window_x = client_x;
	uint32_t window_y = client_y;
	uint32_t window_width = client_width;
	uint32_t window_height = client_height;
	
	uint32_t window_style =
		WS_OVERLAPPED |
		WS_SYSMENU |
		WS_CAPTION |
		WS_THICKFRAME |
		WS_MINIMIZEBOX |
		WS_MAXIMIZEBOX;
	
	uint32_t window_ex_style = WS_EX_APPWINDOW;
	
	// Obtain the size of the border
	RECT border_rect = {};
	AdjustWindowRectEx(&border_rect,
										 (DWORD)window_style,
										 0,
										 (DWORD)window_ex_style);
	
	window_x += border_rect.left;
	window_y += border_rect.top;
	
	window_width += border_rect.right - border_rect.left;
	window_height += border_rect.bottom - border_rect.top;
	
	window = CreateWindowExA(
													 (DWORD)window_ex_style, "cakez_window_class", title,
													 (DWORD)window_style, window_x, window_y, window_width, window_height,
													 0, 0, instance, 0);
	
	if (window == 0)
	{
		MessageBoxA(NULL, "Window creation failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	
	DragAcceptFiles(window, true);
	
	// Show the window
	// ShowWindow(window, SW_SHOW);
	
	// Register a global Hotkey
	{
		if (RegisterHotKey(window, VK_F2, MOD_SHIFT, VK_F2))
		{
		}
		else
		{
			CAKEZ_ASSERT(0, "Failed");
		}
	}
	
	return true;
}

void platform_update_window()
{
	MSG msg;
	
	while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

global_variable IDirectSound8 *directSound;
global_variable IDirectSoundBuffer *soundBufferNormal;
global_variable IDirectSoundBuffer *soundBufferChipmunk;
global_variable IDirectSoundBuffer *soundBuffer;

bool platform_init_sound(AppMemory* appMemory)
{
	// Global Sound State initialization
	{
		soundState = (SoundState*)allocate_memory(appMemory, sizeof(SoundState));
		
		if(!soundState)
		{
			CAKEZ_FATAL("Failed allocating SoundState");
			return false;
		}
		
		
		soundState->mixBuffer = (char *)allocate_memory(appMemory, KB(200));
		
		// Buffer to hold playing sounds
		// TODO: Use memory arenas to have dynamic memory allocation???j
		soundState->soundBufferSize = SOUND_BUFFER_SIZE;
		soundState->soundBuffer = (char*)allocate_memory(appMemory, soundState->soundBufferSize);
		
		if(!soundState->soundBuffer)
		{
			CAKEZ_FATAL("Failed allocating SoundBuffer");
			return false;
		}
	}
	
	IDirectSoundBuffer *primaryBuffer = 0;
	
	if (DirectSoundCreate8(0, &directSound, 0) != DS_OK)
	{
		CAKEZ_ASSERT(0, "Failed to Create Direct Sound");
		return false;
	}
	
	if (directSound->SetCooperativeLevel(window, DSSCL_PRIORITY) != DS_OK)
	{
		CAKEZ_ASSERT(0, "Failed to Set Cooperative Level for Sound");
		return false;
	}
	
	// Setup primary Buffer
	{
		DSBUFFERDESC bufferDesc = {};
		bufferDesc.dwSize = sizeof(DSBUFFERDESC);
		bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
		bufferDesc.dwBufferBytes = 0;
		bufferDesc.dwReserved = 0;
		bufferDesc.lpwfxFormat = NULL;
		bufferDesc.guid3DAlgorithm = GUID_NULL;
		
		if (directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to Create primary Sound Buffer");
			return false;
		}
	}
	
	// Setup the Format of the primary Sound Buffer
	{
		WAVEFORMATEX waveFormat = {};
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nSamplesPerSec = 22050;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nChannels = 1;
		waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;
		
		if (primaryBuffer->SetFormat(&waveFormat) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to set Format for Primary Buffer");
			return false;
		}
	}
	
	// Setup the secondary Buffer
	{
		// Set the wave format of secondary buffer that this wave file will be loaded onto.
		WAVEFORMATEX waveFormat = {};
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nSamplesPerSec = 22050;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nChannels = 1;
		waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;
		
		// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
		DSBCAPS bufferCaps = {};
		bufferCaps.dwSize = sizeof(DSBCAPS);
		primaryBuffer->GetCaps(&bufferCaps);
		
		DSBUFFERDESC bufferDesc = {};
		bufferDesc.dwSize = sizeof(DSBUFFERDESC);
		// TODO: This might be a nice option to have in the game??
		bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
		bufferDesc.dwBufferBytes = bufferCaps.dwBufferBytes;
		bufferDesc.dwReserved = 0;
		bufferDesc.lpwfxFormat = &waveFormat;
		bufferDesc.guid3DAlgorithm = GUID_NULL;
		
		// Create a secondar Sound Buffer that can be copied to
		if (directSound->CreateSoundBuffer(&bufferDesc, &soundBufferNormal, 0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to create Secondary Sound Buffer");
			return false;
		}
	}
	
	// Setup the chipmunk Buffer
	{
		// Set the wave format of secondary buffer that this wave file will be loaded onto.
		WAVEFORMATEX waveFormat = {};
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nSamplesPerSec = 35000;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nChannels = 1;
		waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;
		
		// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
		DSBCAPS bufferCaps = {};
		bufferCaps.dwSize = sizeof(DSBCAPS);
		primaryBuffer->GetCaps(&bufferCaps);
		
		DSBUFFERDESC bufferDesc = {};
		bufferDesc.dwSize = sizeof(DSBUFFERDESC);
		// TODO: This might be a nice option to have in the game??
		bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
		bufferDesc.dwBufferBytes = bufferCaps.dwBufferBytes;
		bufferDesc.dwReserved = 0;
		bufferDesc.lpwfxFormat = &waveFormat;
		bufferDesc.guid3DAlgorithm = GUID_NULL;
		
		// Create a secondar Sound Buffer that can be copied to
		if (directSound->CreateSoundBuffer(&bufferDesc, &soundBufferChipmunk, 0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to create Secondary Sound Buffer");
			return false;
		}
	}
	
	// Play the Sound Buffer
	{
		// Set position at the beginning of the sound buffer.
		if (soundBufferNormal->SetCurrentPosition(0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to set Cursor Postion of Secondary Buffer");
			return false;
		}
		
		// Set volume of the Buffer, -10000 -> 0 -> 10000?
		if (soundBufferNormal->SetVolume(0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to set Volume of Secondary Buffer");
			return false;
		}
		
		// Play the contents of the secondary sound buffer.
		if (soundBufferNormal->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to play Secondary Buffer");
			return false;
		}
	}
	
	// Play the Chipmunk Sound Buffer
	{
		// Set position at the beginning of the sound buffer.
		if (soundBufferChipmunk->SetCurrentPosition(0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to set Cursor Postion of Secondary Buffer");
			return false;
		}
		
		// Set volume of the Buffer, -10000 -> 0 -> 10000?
		if (soundBufferChipmunk->SetVolume(0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to set Volume of Secondary Buffer");
			return false;
		}
		
		// Play the contents of the secondary sound buffer.
		if (soundBufferChipmunk->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to play Secondary Buffer");
			return false;
		}
	}
	
	DSBCAPS bufferCaps = {};
  bufferCaps.dwSize = sizeof(DSBCAPS);
  soundBufferNormal->GetCaps(&bufferCaps);
	
	soundState->mixBufferSize = 0;
  soundState->mixBufferSize = bufferCaps.dwBufferBytes;
	
	if(!soundState->mixBufferSize)
	{
		CAKEZ_FATAL("Could not get size of Sound Buffer!");
		return false;
	}
	
	// Use normal sound Buffer
	soundBuffer = soundBufferNormal;
	
	return true;
}

void platform_update_sound()
{
	uint32_t playCursor;
	uint32_t writeCursor;
	char* mixBuffer = soundState->mixBuffer;
	
	soundBuffer->GetCurrentPosition((LPDWORD)&playCursor, (LPDWORD)&writeCursor);
	if (playCursor == writeCursor)
	{
		CAKEZ_ASSERT(0, "Dunno bro? Why?");
		
		// Try playing the soundBuffer again??
		if (soundBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to play Secondary Buffer");
		}
	}
	DSBCAPS bufferCaps = {};
	bufferCaps.dwSize = sizeof(DSBCAPS);
	soundBuffer->GetCaps(&bufferCaps);
	
	void *firstRegion;
	uint32_t firstRegionByteCount;
	void *secondRegion;
	uint32_t secondRegionByteCount;
	uint32_t copyByteCount = bufferCaps.dwBufferBytes;
	
	memset(mixBuffer, 0, copyByteCount);
	
	if (soundState->playingSoundCount)
	{
		
		uint32_t sampleCopyCount = copyByteCount / sizeof(int16_t);
		int16_t *mixBufferSamples = (int16_t *)mixBuffer;
		
		bool clampedToMin = false;
		bool clampedToMax = false;
		
		uint32_t playCursorAdvance = 0;
		if (playCursor >= soundState->oldPlayCursor)
		{
			playCursorAdvance = playCursor - soundState->oldPlayCursor;
		}
		else
		{
			playCursorAdvance = bufferCaps.dwBufferBytes - soundState->oldPlayCursor + playCursor;
		}
		uint32_t sampleAdvance = playCursorAdvance / sizeof(int16_t);
		
		for (uint32_t i = 0; i < soundState->playingSoundCount; i++)
		{
			Sound *sound = &soundState->playingSounds[i];
			
			// Increase the Play Cursor for the Sound if it is not 0
			if (sound->playing)
			{
				sound->playCursor += playCursorAdvance;
				sound->sampleIdx += sampleAdvance;
			}
			
			// Boundary Check
			{
				if (sound->sampleIdx > sound->sampleCount)
					// if (sound->playCursor > sound->size)
				{
					if (sound->loop)
					{
						while (sound->sampleIdx > sound->sampleCount)
						{
							sound->playCursor = sound->playCursor - sound->size;
							sound->sampleIdx = sound->sampleIdx - sound->sampleCount;
						}
					}
					else
					{
						// If the Sound is not the last one
						if (i < soundState->playingSoundCount - 1)
						{
							// Copy over the following Sounds
							for (uint32_t j = i; j < soundState->playingSoundCount - 1; j++)
							{
								Sound *a = &soundState->playingSounds[j];
								Sound *b = &soundState->playingSounds[j + 1];
								
								*a = *b;
							}
						}
						
						// Descrese Sound Count
						soundState->playingSoundCount--;
						
						i--;
						continue;
					}
				}
			}
			
			// Mix the Sounds together
			{
				float volume = 1.0f;
				uint32_t sampleIdx = sound->sampleIdx;
				for (uint32_t j = 0; j < sampleCopyCount; j++)
				{
					int32_t sample = 0;
					int16_t *soundSamples = 0;
					// Get Sample for copy
					{
						if (sampleIdx >= sound->sampleCount)
						{
							if (sound->loop)
							{
								// Start from the beginning again
								sampleIdx = 0;
							}
							else
							{
								// End of Data, nothing more to copy, break
								break;
							}
						}
						sample = (int32_t)sound->samples[sampleIdx++];
					}
					
					int32_t mixedValue = (int32_t)mixBufferSamples[j];
					int32_t mixedSample = mixedValue + sample;
					
					if (mixedSample > INT16_MAX)
					{
						mixedSample = INT16_MAX;
						clampedToMax = true;
					}
					else if (mixedSample < INT16_MIN)
					{
						mixedSample = INT16_MIN + 1;
						clampedToMin = true;
						// CAKEZ_TRACE("Clamping Sound to MIN");
					}
					
					mixBufferSamples[j] = (int16_t)mixedSample;
				}
			}
			
			if (!sound->playing)
			{
				sound->playing = true;
			}
		}
		
#ifdef DEBUG
		if (clampedToMin)
		{
			CAKEZ_TRACE("Clamping Sound to MIN");
		}
		if (clampedToMax)
		{
			CAKEZ_TRACE("Clamping Sound to MAX");
		}
#endif
	}
	
	// Copy data to Sound Buffer
	{
		soundState->oldPlayCursor = playCursor;
		soundState->oldWriteCursor = writeCursor;
		
		// Locking the Buffer returns us two Regions to write to
		if (auto result = soundBuffer->Lock(writeCursor, copyByteCount,
																				&firstRegion, (LPDWORD)&firstRegionByteCount,
																				&secondRegion, (LPDWORD)&secondRegionByteCount, 0) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to lock the Sound Buffer");
		}
		
		if (secondRegion && secondRegionByteCount)
		{
			memcpy(firstRegion, mixBuffer, firstRegionByteCount);
			memcpy(secondRegion, (mixBuffer + firstRegionByteCount), copyByteCount - firstRegionByteCount);
		}
		else
		{
			memcpy(firstRegion, mixBuffer, copyByteCount);
		}
		
		if (soundBuffer->Unlock(firstRegion, firstRegionByteCount,
														secondRegion, secondRegionByteCount) != DS_OK)
		{
			CAKEZ_ASSERT(0, "Failed to unlock the Sound Buffer");
		}
	}
}

void platform_listen_poll_urls()
{
	while(true)
	{
		// HUH?
		HTTP_REQUEST_ID requestID;
		HTTP_SET_NULL_ID(&requestID);
		
		ULONG bytesRead;
		char buffer[requestBufferLength];
		PHTTP_REQUEST pRequest = (PHTTP_REQUEST)buffer;
		
		ULONG result = HttpReceiveHttpRequest(win32HTTPState.HTTPHandle, requestID, 0, pRequest, 
																					requestBufferLength, &bytesRead, 0);
		CAKEZ_ASSERT(result == NO_ERROR, "Could not recieve HTTP Request");
		
		if(result == NO_ERROR)
		{
			if (pRequest->BytesReceived)
			{
				switch (pRequest->Verb)
				{
					case HttpVerbGET:
					{
						char *responseURL = (char *)pRequest->pRawUrl;
						
						HTTP_DATA_CHUNK responeBody = {};
						responeBody.DataChunkType = HttpDataChunkFromMemory;
						responeBody.FromMemory.BufferLength = 21;
						responeBody.FromMemory.pBuffer = "Everything went okay!";
						
						if(str_cmp("/testRequest", responseURL))
						{
							if(platform_file_exists("local_server/videoName.txt"))
							{
								while(!platform_delete_file("local_server/videoName.txt"))
								{
									platform_sleep(1000);
								}
							}
							
							if(twitchState->requestVideoIdx)
							{
								char* videoName = twitchState->requestVideos[--twitchState->requestVideoIdx];
								
								
								platform_write_file("local_server/videoName.txt",
																		videoName, strlen(videoName), false);
								
								platform_sleep(5000);
							}
						}
						
						HTTP_RESPONSE response = {};
						response.pReason = "OK";
						response.ReasonLength = 2;
						response.StatusCode = 200;
						response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/plain";
						response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = 10;
						response.EntityChunkCount = 1;
						response.pEntityChunks = &responeBody;
						ULONG bytesSend;
						
						ULONG result = 
							HttpSendHttpResponse(win32HTTPState.HTTPHandle, pRequest->RequestId, 
																	 0, &response, 0, &bytesSend, 0, 0, 0, 0);
						CAKEZ_ASSERT(result == NO_ERROR, "Failed to send HTTP Response");
						
						break;
					}
				}
			}
		}
		else
		{
			CAKEZ_ASSERT(0, "Could not recieve on polling URLS");
		}
	}
}

int main()
{
	// Allocate App Memory
	AppMemory memory = {};
	memory.memory = (uint8_t *)malloc(MB(50));
	memory.size = MB(50);
	
	if(!platform_create_window(100, 100, "Test"))
	{
		CAKEZ_FATAL("Failed to Create Window!");
		return -1;
	}
	
	if(!platform_init_sound(&memory))
	{
		CAKEZ_FATAL("Failed to initialize Sound!");
		return -1;
	}
	
	// Iinitialize HTTP
	{
		if(HttpInitialize(HTTPAPI_VERSION_1, HTTP_INITIALIZE_SERVER, 0) != NO_ERROR)
		{
			CAKEZ_FATAL("Failed to initialize HTTP");
			return -1;
		}
		
		if(HttpCreateHttpHandle(&win32HTTPState.HTTPHandle, 0) != NO_ERROR)
		{
			CAKEZ_FATAL("Failed to create HTTP Handle");
			return -1;
		}
		
		// Create Windows handle to open HTTP Connections
		if (!(win32HTTPState.globalInstance = WinHttpOpen(L"PleaseJustLetMeCreateThisOkay", 
																											WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, 0, 0, 0)))
		{
			CAKEZ_FATAL("Failed to open HTTP connection");
			return -1;
		}
		
		// TODO: Mabye useful in the future
		//localApi = platform_connect_to_server("localhost", false);
	}
	
	if(!init_twitch_connection(&memory))
	{
		CAKEZ_FATAL("Failed to init Twtich API Connection");
		return -1;
	}
	
	// Memory for File IO, last Allocation, ALWAYS!
	fileIOMemory = allocate_memory(&memory, FILE_IO_MEMORY_SIZE);
	
	// Threads of the Program
	{
		std::thread manageTwitchEvents(manage_twitch_events);
		
		char* token = get_o_auth_token();
		std::thread read_chat(connect_to_chat, token);
		
		// Listhen on HTTP
		std::thread listenPollUrls(platform_listen_poll_urls);
		
		while (true)
		{
			platform_update_window();
			platform_update_sound();
			Sleep(10);
		}
	}
	
	return 0;
}

//#######################################################################
//                      Implementations from sound.h
//#######################################################################
void play_sound(Sound sound, bool loop, float volume)
{
	if (sound.data)
	{
		if (soundState->playingSoundCount < MAX_PLAYING_SOUNDS)
		{
			soundState->playingSounds[soundState->playingSoundCount++] = sound;
		}
		else
		{
			CAKEZ_ASSERT(0, "Reached maximum playing Sounds");
		}
	}
}

#define MINIMP3_IMPLEMENTATION
#include "mp3_lib.h"

void play_sound(char* mp3File, uint32_t fileSize, bool loop, float volume)
{
	if (soundState->playingSoundCount < MAX_PLAYING_SOUNDS)
	{
		// TODO: Only allows playing one singular sound
		memset(soundState->soundBuffer, 0, soundState->soundBufferSize);
		
		Sound s = {};
		s.data = soundState->soundBuffer;
		s.samples = (int16_t*)soundState->soundBuffer;
		s.volume = 0.7f;
		
		// MP3 Lib Stuff
		{
			mp3dec_t mp3d;
			mp3dec_init(&mp3d);
			short *samples = s.samples;
			
			while (fileSize != 0)
			{
				mp3dec_frame_info_t info;
				
				int sampleCount = mp3dec_decode_frame(&mp3d, ( uint8_t *)mp3File,
																							fileSize, samples, &info);
				printf("samples = %d\n", sampleCount);
				
				fileSize -= info.frame_bytes;
				mp3File += info.frame_bytes;
				samples += sampleCount;
				s.sampleCount += sampleCount;
			}
		}
		
		soundState->playingSounds[soundState->playingSoundCount++] = s;
	}
	else
	{
		CAKEZ_ASSERT(0, "Reached maximum playing Sounds");
	}
	
}

int sound_get_playing_sound_count()
{
	return soundState->playingSoundCount;
}

void platform_change_sound_buffer(SoundBufferType type)
{
	switch(type)
	{
		case SOUND_BUFFER_TYPE_NORMAL:
		{
			soundBuffer = soundBufferNormal;
			break;
		}
		case SOUND_BUFFER_TYPE_CHIPMUNK:
		{
			soundBuffer = soundBufferChipmunk;
			break;
		}
	}
	
}

//#######################################################################
//                      Implementations from platform.h
//#######################################################################
void platform_log( char *msg, TextColor color)
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
	
	int length = str_length(str);
	if (length && (length < bufferLength))
	{
		length = MultiByteToWideChar(CP_UTF8, 0, str, length, buffer, length);
		buffer[length] = 0;
	}
	
	return length;
}

HTTPConnection platform_connect_to_server(char *serverName, bool https)
{
	if (win32HTTPState.connectionCount >= MAX_SERVER_CONNECTIONS)
	{
		CAKEZ_ASSERT(0, "Reached maximum amount of connections!");
		return {0, INVALID_IDX};
	}
	
	// Conversion bullshit here
	wchar_t wServerName[50];
	platform_convert_to_wchar(serverName, wServerName, 50);
	CAKEZ_TRACE("Connecting to Server: %s", serverName);
	
	if (HINTERNET connection = 
			WinHttpConnect(win32HTTPState.globalInstance,
										 wServerName, 
										 https? 
										 INTERNET_DEFAULT_HTTPS_PORT: 
										 INTERNET_DEFAULT_HTTP_PORT, 0))
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

Request platform_send_http_request(HTTPConnection connection, char *url, 
																	 char *header, char *method, char *data,
																	 bool secure)
{
	CAKEZ_ASSERT(url, "URL required");
	
	if (connection.connectionID >= MAX_SERVER_CONNECTIONS)
	{
		CAKEZ_ASSERT(0, "Connection Index: %d out of Bounds", connection.connectionID);
		return {0, 0, 0, 0};
	}
	
	if (win32HTTPState.requestCount >= MAX_REQUESTS)
	{
		CAKEZ_ASSERT(0, "Reached maximum Amount of Requests");
		return {0, 0, 0, 0};
	}
	
	// Conversion bullshit here
	wchar_t wUrl[ENCODED_REDEMPTION_TEXT_LENGTH] = {};
	int length = platform_convert_to_wchar(url, wUrl, ENCODED_REDEMPTION_TEXT_LENGTH );
	
	wchar_t wHeader[1024] = {};
	int headerLength = platform_convert_to_wchar(header, wHeader, 1024);
	
	wchar_t wMethod[1024] = {};
	int methodLength = platform_convert_to_wchar(method, wMethod, 1024);
	
	wchar_t wData[1024] = {};
	int dataLength = platform_convert_to_wchar(data, wData, 1024);
	
	HINTERNET connectionHandle = win32HTTPState.connections[connection.connectionID];
	
	// This uses HTTP/1.1
	HINTERNET winRequest = 0;
	if (!(winRequest = WinHttpOpenRequest(connectionHandle, wMethod, wUrl, 0,
																				WINHTTP_NO_REFERER,
																				WINHTTP_DEFAULT_ACCEPT_TYPES,
																				secure? WINHTTP_FLAG_SECURE: 0)))
	{
		CAKEZ_ASSERT(0, "Couldn't open HTTP Request");
		return {0, 0, 0, 0};
	}
	
	if (WinHttpSendRequest(winRequest, headerLength ? wHeader : 0,
												 0, dataLength ? data : 0, dataLength, dataLength, 0))
	{
		Request request = {url, method, header, winRequest};
		win32HTTPState.requests[win32HTTPState.requestCount++] = request;
		return request;
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
		return {0, 0, 0, 0};
	}
}

bool platform_recieve_http_response(Request request)
{
	if (!(request.httpHandle))
	{
		CAKEZ_ASSERT(0, "Invalid Request: URL: %s, Method: %s, Header: %s",
								 request.url, request.method, request.header);
		return false;
	}
	
	HINTERNET winRequest = (HINTERNET)request.httpHandle;
	
	return WinHttpReceiveResponse(winRequest, 0);
}

bool platform_receive_http_data(Request request, char *outBuffer, 
																uint32_t bufferSize, uint32_t *outReceivedBytes)
{
	if(outReceivedBytes)
	{
		*outReceivedBytes = 0;
	}
	
	if (!(request.httpHandle))
	{
		CAKEZ_ASSERT(0, "Invalid Request: URL: %s, Method: %s, Header: %s",
								 request.url, request.method, request.header);
		return false;
	}
	
	HINTERNET winRequest = (HINTERNET)request.httpHandle;
	
	// Clear the buffer to 0
	memset(outBuffer, 0, bufferSize);
	
	DWORD bytesRecieved;
	while (WinHttpQueryDataAvailable(winRequest, &bytesRecieved) && bytesRecieved)
	{
		DWORD bytesRead;
		if (!WinHttpReadData(winRequest, outBuffer, bytesRecieved, &bytesRead))
		{
			CAKEZ_ASSERT(0, "Unable to read data");
			return false;
		}
		
		CAKEZ_ASSERT(bytesRead == bytesRecieved, "Test");
		
		outBuffer += bytesRead;
		
		if (outReceivedBytes)
		{
			*outReceivedBytes += bytesRead;
		}
	}
	
	// CAKEZ_TRACE(outBuffer);
	return true;
}

void platform_close_http_request(Request request)
{
	uint32_t foundRequestIdx = INVALID_IDX;
	for (uint32_t requestIdx = 0; requestIdx < win32HTTPState.requestCount; requestIdx++)
	{
		if (win32HTTPState.requests[requestIdx].httpHandle == request.httpHandle)
		{
			foundRequestIdx = requestIdx;
			break;
		}
	}
	
	if (foundRequestIdx != INVALID_IDX)
	{
		win32HTTPState.requestCount--;
		for (uint32_t requestIdx = foundRequestIdx;
				 requestIdx < win32HTTPState.requestCount - 1; requestIdx++)
		{
			win32HTTPState.requests[requestIdx] = win32HTTPState.requests[requestIdx + 1];
		}
		
		for (uint32_t requestIdx = foundRequestIdx;
				 requestIdx < MAX_REQUESTS; requestIdx++)
		{
			win32HTTPState.requests[requestIdx] = {};
		}
	}
	
	if (!WinHttpCloseHandle((HINTERNET)request.httpHandle))
	{
		CAKEZ_ASSERT(0, "Failed to close HTTP Request!");
	}
}

bool platform_add_http_poll_url(char* url)
{
	wchar_t wUrl[512] = {};
	platform_convert_to_wchar(url, wUrl, 512);
	
	ULONG result = HttpAddUrl(win32HTTPState.HTTPHandle, wUrl, 0);
	if (result == NO_ERROR)
	{
		win32HTTPState.pollingUrls[win32HTTPState.pollingUrlCount++] = url;
		return true;
	}
	else
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
			CAKEZ_WARN("Couldn't add poll URL: %s, Error: ERROR_NOT_ENOUGH_MEMORY",);
			break;
		}
		
		return false;
	}
}

bool platform_file_exists(char *path)
{
  DWORD attributes = GetFileAttributes(path);
  
  return (attributes != INVALID_FILE_ATTRIBUTES &&
          !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

char *platform_read_file(char *path, uint32_t *length)
{
	char *buffer = 0;
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ,
													 0, OPEN_EXISTING, 0, 0);
	
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
				// TODO: What can I do here?
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

bool platform_delete_file(char *path)
{
  return DeleteFileA(path) != 0;
}

unsigned long platform_write_file(char *path,
																	char *buffer,
																	uint32_t size,
																	bool overwrite)
{
	DWORD bytesWritten = 0;
	
	HANDLE file = CreateFile(path,
													 overwrite ? GENERIC_WRITE : FILE_APPEND_DATA,
													 FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
	
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
			// Success
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

void platform_sleep(int ms)
{
	Sleep(ms);
}