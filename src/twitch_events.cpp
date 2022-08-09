#include "url_encode.h"

//#######################################################################
//                      Internal Globals
//#######################################################################
constexpr uint32_t REDEMPTION_BUFFER_SIZE = KB(10);
constexpr uint32_t REDEMPTION_TEXT_LENGTH = 4096;
constexpr uint32_t ENCODED_REDEMPTION_TEXT_LENGTH = 4096;
constexpr uint32_t TWITCH_ID_LENGTH = 37;
constexpr uint32_t MAX_TTS_REQUESTS = 100;
constexpr uint32_t MP3_BUFFER_SIZE = KB(50);
constexpr uint32_t TTS_REQUEST_QUEUE_SIZE = 100;
constexpr uint32_t MAX_REDEMPTION_IDS = 50;


//#######################################################################
//                      Internal Structures
//#######################################################################
struct TTSRequest
{
	char rewardID[TWITCH_ID_LENGTH];
	char ID[TWITCH_ID_LENGTH];
	char text[REDEMPTION_TEXT_LENGTH];
};

struct Redemptions
{
	uint32_t ttsRequestCount;
	TTSRequest ttsRequestQueue[TTS_REQUEST_QUEUE_SIZE];
	
	uint8_t redemptionIDCount;
	uint8_t redemptionsIDS[MAX_REDEMPTION_IDS];
};

//#######################################################################
//                      Internal Data
//#######################################################################
global_variable Redemptions* redemptions;

//#######################################################################
//                      Internal Functions
//#######################################################################
internal bool init_twitch_connection(AppMemory* appMemory)
{
	redemptions = (Redemptions*)allocate_memory(appMemory, sizeof(Redemptions));
	
	if(!redemptions)
	{
		CAKEZ_FATAL("Failed to allocate Memory for Redemptions!");
		return false;
	}
	
	// Setup twtich API Callback for Twitch Events (Subs, Follows, ...)
	{
		
	}
	
	platform_add_http_poll_url("http://localhost:49152/twitchEvents");
	
	return true;
}

void manage_twitch_events(SoundState *soundState, 
													char *redemptionBuffer, 
													char *encodedRedemptionTextBuffer, 
													HTTPConnection twitchApi,
													char *token, 
													HTTPConnection streamelementsApi, 
													char *mp3Buffer, 
													short *soundBuffer)
{
	memset(encodedRedemptionTextBuffer, 0, ENCODED_REDEMPTION_TEXT_LENGTH);
	char* header = encodedRedemptionTextBuffer;
	sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s", token, CLIENT_ID);
	
	
	while (true)		
	{
		uint32_t redemptionTextLength = 0;
		// Get Channel Point Redemptions
		{
			Request getChannelPointRewards = 
				platform_send_http_request(twitchApi, "helix/channel_points/custom_rewards/redemptions?broadcaster_id=80953740&reward_id=713be8e1-266a-4b5c-bf37-c42882ddc845&status=UNFULFILLED", 
																	 header, "GET");
			if (!platform_recieve_http_response(getChannelPointRewards))
			{
				// CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", getChannelPointRewards.url);
			}
			
			memset(redemptionBuffer, 0, REDEMPTION_BUFFER_SIZE);
			if (!platform_receive_http_data(getChannelPointRewards, redemptionBuffer, REDEMPTION_BUFFER_SIZE, &redemptionTextLength))
			{
				CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", getChannelPointRewards.url);
			}
			
			platform_close_http_request(getChannelPointRewards);
			
			// TODO: Refresh Token when no longer valid!
		}
		
		CAKEZ_TRACE(redemptionBuffer);
		
		// Parse the JSON Data, extract UNFULFILLED Requests
		{
			
			uint32_t requestCount = 0;
			TTSRequest requests[MAX_TTS_REQUESTS] = {};
			
			std::vector<Token> tokens;
			
			if (parse_json(redemptionBuffer, redemptionTextLength, tokens))
			{
				TTSRequest *request = 0;
				for (uint32_t tokenIdx = 0; tokenIdx < tokens.size(); tokenIdx++)
				{
					Token t = tokens[tokenIdx];
					if (t.type == TOKEN_TYPE_STRING)
					{
						char *name = &redemptionBuffer[t.start];
						
						if (contains_prefix("id", name))
						{
							request = &requests[requestCount++];
							// Store Value
							t = tokens[++tokenIdx];
							memcpy(request->ID, &redemptionBuffer[t.start], t.end - t.start);
						}
						
						if (contains_prefix("user_input", name))
						{
							// Store Value
							t = tokens[++tokenIdx];
							memcpy(request->text, &redemptionBuffer[t.start], t.end - t.start);
						}
						
						// Store the Reward ID
						if (contains_prefix("reward", name))
						{
							for (; tokenIdx < tokens.size(); tokenIdx++)
							{
								t = tokens[tokenIdx];
								name = &redemptionBuffer[t.start];
								if (contains_prefix("id", name))
								{
									// Store Value
									t = tokens[++tokenIdx];
									memcpy(request->rewardID, &redemptionBuffer[t.start], t.end - t.start);
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				CAKEZ_ASSERT(0, "Could now parse JSON!");
			}
			
			for (uint32_t requestIdx = 0; requestIdx < requestCount; requestIdx++)
			{
				// system("start vlc.lnk");
				TTSRequest ttsRequest = requests[requestIdx];
				uint32_t mp3DataSize = 0;
				// Get MP3 File containing Brian Voice
				{
					char *text = ttsRequest.text;
					char textEncoded[ENCODED_REDEMPTION_TEXT_LENGTH] = {};
					
					// Do URL Encoding
					{
						uint32_t index = 0;
						uint32_t charCount = 0;
						while (unsigned char c = *(text++))
						{
							if (c == '\n' || c == '\\')
								continue;
							
							charCount++;
							
							if(charCount >= ENCODED_REDEMPTION_TEXT_LENGTH/2)
							{
								break;
							}
							
							int32_t code = uri_encode_tbl_codes[c];
							
							if (code)
							{
								*((int32_t *)&textEncoded[index]) = code;
								index += 3;
							}
							else
							{
								textEncoded[index++] = c;
							}
						}
						int a = 0;
					}
					memset(encodedRedemptionTextBuffer, 0, ENCODED_REDEMPTION_TEXT_LENGTH);
					sprintf(encodedRedemptionTextBuffer, "kappa/v2/speech?voice=Brian&text=%s", textEncoded);
					
					platform_write_file("tts_request.txt", encodedRedemptionTextBuffer, ENCODED_REDEMPTION_TEXT_LENGTH, true);
					
					Request getMP3FromStreamelements = platform_send_http_request(streamelementsApi, encodedRedemptionTextBuffer, 0, "GET");
					if (!platform_recieve_http_response(getMP3FromStreamelements))
					{
						CAKEZ_ASSERT(0, "Failed to receive HTTP Response: %s", getMP3FromStreamelements.url);
					}
					
					memset(mp3Buffer, 0, MP3_BUFFER_SIZE);
					if (!platform_receive_http_data(getMP3FromStreamelements, mp3Buffer, MP3_BUFFER_SIZE, &mp3DataSize))
					{
						CAKEZ_ASSERT(0, "Failed to receive HTTP Data: %s", getMP3FromStreamelements.url);
					}
					
					platform_close_http_request(getMP3FromStreamelements);
				}
				
				platform_write_file("mp3data.mp3", mp3Buffer, mp3DataSize, true);
				
				CAKEZ_ASSERT(mp3DataSize < MB(25), "Sound Buffer not large enough");
				
				memset(soundBuffer, 0, MB(25));
				
				Sound s = {};
				s.data = (char *)soundBuffer;
				s.samples = soundBuffer;
				s.volume = 0.7f;
				
				uint32_t fileSize = mp3DataSize;
				char *mp3File = mp3Buffer;
				
				if (fileSize)
				{
					// MP3 Lib Stuff
					{
						mp3dec_t mp3d;
						mp3dec_init(&mp3d);
						short *samples = soundBuffer;
						
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
					
					s.size = s.sampleCount * sizeof(short);
					
					if (s.sampleCount)
					{
						play_sound(soundState, s);
						
						while (soundState->playingSoundCount)
						{
							platform_sleep(2000);
						}
						
						// TODO: Complete the Request usind Twitch API
						// Complete the Request using Twtich API
						{
							// Header
							memset(encodedRedemptionTextBuffer, 0, ENCODED_REDEMPTION_TEXT_LENGTH);
							char* header = encodedRedemptionTextBuffer;
							sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s\r\nContent-Type: application/json", token, CLIENT_ID);
							
							// Link
							char link[1024] = {};
							sprintf(link, "helix/channel_points/custom_rewards/redemptions?broadcaster_id=80953740&reward_id=%s&id=%s",
											
											ttsRequest.rewardID, ttsRequest.ID);
							
							Request completeRedemption = platform_send_http_request(twitchApi, link, header, "PATCH",
																																			"{\"status\": \"FULFILLED\"}");
							if (!platform_recieve_http_response(completeRedemption))
							{
								CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", completeRedemption.url);
							}
							
							memset(redemptionBuffer, 0, REDEMPTION_BUFFER_SIZE);
							redemptionTextLength = 0;
							if (!platform_receive_http_data(completeRedemption, redemptionBuffer, REDEMPTION_BUFFER_SIZE, &redemptionTextLength))
							{
								CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", completeRedemption.url);
							}
							
							platform_close_http_request(completeRedemption);
						}
					}
				}
			}
		}
		
		platform_sleep(3000);
	}
}

