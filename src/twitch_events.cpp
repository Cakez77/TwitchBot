#include "url_encode.h"
#include "twitch_env.h"

//#######################################################################
//                      Internal Globals
//#######################################################################
constexpr uint32_t REDEMPTION_BUFFER_SIZE = KB(10);
constexpr uint32_t REDEMPTION_TEXT_LENGTH = 4096;
constexpr uint32_t ENCODED_REDEMPTION_TEXT_LENGTH = 4096;
constexpr uint32_t TWITCH_ID_LENGTH = 37;
constexpr uint32_t MAX_TTS_REQUESTS = 100;
constexpr uint32_t MP3_BUFFER_SIZE = MB(10);
constexpr uint32_t TTS_REQUEST_QUEUE_SIZE = 100;
constexpr uint32_t MAX_REDEMPTION_IDS = 50;
constexpr uint32_t INDEX_HTML_FILE_SIZE = KB(2);
constexpr uint32_t MAX_REQUEST_VIDEOS = 10;

//#######################################################################
//                      Internal Structures
//#######################################################################
struct TTSRequest
{
	char rewardID[TWITCH_ID_LENGTH];
	char ID[TWITCH_ID_LENGTH];
	char text[REDEMPTION_TEXT_LENGTH];
};

enum RedemptionTypeID
{
	REDEMPTION_TYPE_TTS_BRIAN,
	REDEMPTION_TYPE_TTS_CHIPMUNK,
	REDEMPTION_TYPE_VIDEO,
	
	REDEMPTION_COUNT
};

struct Redemption
{
	RedemptionTypeID type;
	char* twitchRewardID;
};

struct TwitchState
{
	char oAuthToken[31];
	
	uint32_t ttsRequestCount;
	TTSRequest ttsRequestQueue[TTS_REQUEST_QUEUE_SIZE];
	
	uint32_t videoIdx = 0;
	uint32_t requestVideoIdx;
	char* requestVideos[MAX_REQUEST_VIDEOS];
	
	HTTPConnection twitchID;
	HTTPConnection twitchAPI;
	HTTPConnection streamelementsAPI;
	char* mp3ResponseBuffer;
	
	char* indexHtmlFormat;
	char* indexHtmlResult;
};

//#######################################################################
//                      Internal Data
//#######################################################################
global_variable TwitchState* twitchState;

global_variable Redemption redemptionTable[REDEMPTION_COUNT] =
{
	{REDEMPTION_TYPE_TTS_BRIAN, "713be8e1-266a-4b5c-bf37-c42882ddc845"},
	{REDEMPTION_TYPE_TTS_CHIPMUNK, "de05a92e-380d-415d-83ed-963808c394cb"},
	{REDEMPTION_TYPE_VIDEO, "452b2256-bf69-4d41-9e66-d34b9454f5c3"},
};		

char* videos[]
{
	"Coding_Principles.mp4",
	"Comparing_Shaders.mp4",
	"I_Drank_A_Potion.mp4",
	"Programming_In_Vain.mp4",
	"Relax.mp4",
	"POE Invincibility Technique.mp4"
};

//#######################################################################
//                      Internal Functions
//#######################################################################
internal char* get_o_auth_token()
{
	return twitchState->oAuthToken;
}

internal bool refresh_o_auth_token()
{
	char refreshUrl[4096] = {};
	
	sprintf(refreshUrl, URL_REFRESH_TWITCH_OAUTH_TOKEN, REFRESH_TOKEN, CLIENT_ID, CLIENT_SECRET);
	Request refreshToken = 
		platform_send_http_request(twitchState->twitchID, refreshUrl, 0);
	
	if (!platform_recieve_http_response(refreshToken))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", refreshToken.url);
		return 0;
	}
	
	char resultBuffer[1024] = {};
	if (!platform_receive_http_data(refreshToken, resultBuffer, 1024))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", refreshToken.url);
		return 0;
	}
	
	platform_close_http_request(refreshToken);
	
	char *data = resultBuffer;
	while (char c = *(data++))
	{
		if (c == ':')
		{
			// Token is 30 Chars long
			memcpy(twitchState->oAuthToken, data + 1, 30);
			break;
		}
	}
	
	// 0 terminate the token
	twitchState->oAuthToken[30] = 0;
	
	return true;
}

internal void create_channel_point_redemption()
{
	char header[512] = {};
	sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s\r\nContent-Type: application/json", 
					twitchState->oAuthToken, CLIENT_ID);
	
	Request createChannelPointRewards = 
		platform_send_http_request(twitchState->twitchAPI,
															 "helix/channel_points/custom_rewards?broadcaster_id=80953740",
															 header, "POST",
															 "{\"title\": \"New Bot Redemption\", \"cost\": 5000}");
	if (!platform_recieve_http_response(createChannelPointRewards ))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", createChannelPointRewards.url);
	}
	char resultBuffer [1024] = {};
	if (!platform_receive_http_data(createChannelPointRewards, resultBuffer, 1024))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", createChannelPointRewards.url);
	}
	
	platform_close_http_request(createChannelPointRewards);
	
	CAKEZ_TRACE(resultBuffer);
}

internal bool init_twitch_connection(AppMemory* appMemory)
{
	twitchState = (TwitchState*)allocate_memory(appMemory, sizeof(TwitchState));
	if(!twitchState)
	{
		CAKEZ_FATAL("Failed to allocate Memory for Redemptions!");
		return false;
	}
	
	twitchState->mp3ResponseBuffer = (char*)allocate_memory(appMemory, MP3_BUFFER_SIZE);
	if(!twitchState->mp3ResponseBuffer)
	{
		CAKEZ_FATAL("Failed to allocate Memory for MP3 TTS Response");
		return false;
	}
	
	twitchState->indexHtmlFormat = (char*)allocate_memory(appMemory, INDEX_HTML_FILE_SIZE);
	if(!twitchState->indexHtmlFormat)
	{
		CAKEZ_FATAL("Failed to allocate Memory for index.html format string");
		return false;
	}
	
	twitchState->indexHtmlResult = (char*)allocate_memory(appMemory, INDEX_HTML_FILE_SIZE);
	if(!twitchState->indexHtmlResult)
	{
		CAKEZ_FATAL("Failed to allocate Memory for index.html result string");
		return false;
	}
	
	twitchState->twitchID = platform_connect_to_server("id.twitch.tv");
	twitchState->twitchAPI = platform_connect_to_server("api.twitch.tv");
	twitchState->streamelementsAPI = platform_connect_to_server("api.streamelements.com");
	
	if(!refresh_o_auth_token())
	{
		return false;
	}
	
	platform_add_http_poll_url("http://localhost:8089/testRequest");
	
	// Use to create new Redemptions
	//create_channel_point_redemption();
	
	return true;
}

internal void handle_tts_request(char* redemptionBuffer, uint32_t redemptionTextLength,
																 bool chipmunk = false)
{
	TTSRequest requests[MAX_TTS_REQUESTS] = {};
	uint32_t requestCount = 0;
	
	// Parse the JSON Data, extract UNFULFILLED Requests
	{
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
			}
			
			char encodedRedemptionTextBuffer[ENCODED_REDEMPTION_TEXT_LENGTH] = {};
			sprintf(encodedRedemptionTextBuffer, "kappa/v2/speech?voice=Brian&text=%s", textEncoded);
			
			platform_write_file("tts_request.txt", encodedRedemptionTextBuffer, ENCODED_REDEMPTION_TEXT_LENGTH, true);
			
			Request getMP3FromStreamelements = 
				platform_send_http_request(twitchState->streamelementsAPI, encodedRedemptionTextBuffer, 0, "GET");
			if (!platform_recieve_http_response(getMP3FromStreamelements))
			{
				CAKEZ_ASSERT(0, "Failed to receive HTTP Response: %s", getMP3FromStreamelements.url);
			}
			
			// Clear MP3 Buffer
			memset(twitchState->mp3ResponseBuffer, 0, MP3_BUFFER_SIZE);
			
			if (!platform_receive_http_data(getMP3FromStreamelements, 
																			twitchState->mp3ResponseBuffer, 
																			MP3_BUFFER_SIZE, &mp3DataSize))
			{
				CAKEZ_ASSERT(0, "Failed to receive HTTP Data: %s", getMP3FromStreamelements.url);
			}
			
			platform_close_http_request(getMP3FromStreamelements);
		}
		
		CAKEZ_ASSERT(mp3DataSize < MP3_BUFFER_SIZE,
								 "MP3 File too large for mp3ResponseBuffer!");
		
		// THIS IF FOR DEBUG REASONS, SO I KNOW THE FILE IS CORRECT, @iammrrob0t
		platform_write_file("mp3_response.mp3", twitchState->mp3ResponseBuffer,
												mp3DataSize, true);
		
		if(chipmunk)
		{
			platform_change_sound_buffer(SOUND_BUFFER_TYPE_CHIPMUNK);
		}
		play_sound(twitchState->mp3ResponseBuffer, mp3DataSize, false, 0.7f);
		
		
		// Wait while the sound is playing
		while (sound_get_playing_sound_count())
		{
			platform_sleep(2000);
		}
		
		platform_change_sound_buffer(SOUND_BUFFER_TYPE_NORMAL);
		
		// Complete the Request using Twtich API
		{
			// Header
			char header[512] = {};
			sprintf(header, 
							"Authorization: Bearer %s\r\nClient-Id: %s\r\nContent-Type: application/json", 
							twitchState->oAuthToken, CLIENT_ID);
			
			// Link
			char link[1024] = {};
			sprintf(link, "helix/channel_points/custom_rewards/redemptions?broadcaster_id=80953740&reward_id=%s&id=%s",
							
							ttsRequest.rewardID, ttsRequest.ID);
			
			Request completeRedemption = 
				platform_send_http_request(twitchState->twitchAPI, link, header, "PATCH",
																	 "{\"status\": \"FULFILLED\"}");
			if (!platform_recieve_http_response(completeRedemption))
			{
				CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", completeRedemption.url);
			}
			
			memset(redemptionBuffer, 0, REDEMPTION_BUFFER_SIZE);
			if (!platform_receive_http_data(completeRedemption, redemptionBuffer, 
																			REDEMPTION_BUFFER_SIZE, &redemptionTextLength))
			{
				CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", completeRedemption.url);
			}
			
			platform_close_http_request(completeRedemption);
		}
	}
}

void manage_twitch_events()
{
	char header[1024] = {};
	sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s", 
					twitchState->oAuthToken, CLIENT_ID);
	
	// Initialize random Seed
	auto seed = platform_get_performance_tick_count();
	
	while (true)		
	{
		uint32_t redemptionTextLength = 0;
		char redemptionBuffer [REDEMPTION_BUFFER_SIZE] = {};
		
		// Get Channel Point Redemptions
		{
			for(uint32_t redemptionTypeID = 0;
					redemptionTypeID < REDEMPTION_COUNT;
					redemptionTypeID++)
			{
				Redemption redemption = redemptionTable[redemptionTypeID];
				
				char* rewardID = redemption.twitchRewardID;
				
				sprintf(redemptionBuffer,URL_QUERY_REDEMPTION_ID,
								BROADCASTER_ID, rewardID);
				
				Request getChannelPointRewards = 
					platform_send_http_request(twitchState->twitchAPI, redemptionBuffer,
																		 header, "GET");
				
				if (!platform_recieve_http_response(getChannelPointRewards))
				{
					CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", getChannelPointRewards.url);
				}
				
				memset(redemptionBuffer, 0, REDEMPTION_BUFFER_SIZE);
				redemptionTextLength = 0;
				if (!platform_receive_http_data(getChannelPointRewards, redemptionBuffer, 
																				REDEMPTION_BUFFER_SIZE, &redemptionTextLength))
				{
					CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", getChannelPointRewards.url);
				}
				
				platform_close_http_request(getChannelPointRewards);
				
				CAKEZ_TRACE(redemptionBuffer);
				
				if(redemptionTextLength < 100)
				{		
					bool tokenInvalid = false;
					std::vector<Token> tokens;
					if (parse_json(redemptionBuffer, redemptionTextLength, tokens))
					{
						for (uint32_t tokenIdx = 0; tokenIdx < tokens.size(); tokenIdx++)
						{
							Token t = tokens[tokenIdx];
							
							if (t.type == TOKEN_TYPE_STRING)
							{
								char *name = &redemptionBuffer[t.start];
								
								if (contains_prefix("error", name))
								{
									tokenInvalid = true;
								}
							}
						}
					}
					
					
					if(tokenInvalid)
					{
						CAKEZ_TRACE("Refreshing OAuth Token...");
						refresh_o_auth_token();
						
						memset(header, 0, 1024);
						sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s", 
										twitchState->oAuthToken, CLIENT_ID);
					}
					
					continue; 
				}
				
				switch(redemption.type)
				{
					case REDEMPTION_TYPE_TTS_BRIAN:
					{
						handle_tts_request(redemptionBuffer, redemptionTextLength);
						break;
					}
					case REDEMPTION_TYPE_TTS_CHIPMUNK:
					{
						handle_tts_request(redemptionBuffer, redemptionTextLength, true);
						break;
					}
					case REDEMPTION_TYPE_VIDEO:
					{
						if(twitchState->requestVideoIdx + 1 < MAX_REQUEST_VIDEOS)
						{
							twitchState->videoIdx = rand() % ArraySize(videos);
							twitchState->requestVideos[twitchState->requestVideoIdx++] = 
								videos[twitchState->videoIdx];
							
							char requestID[TWITCH_ID_LENGTH] = {};
							
							std::vector<Token> tokens;
							if (parse_json(redemptionBuffer, redemptionTextLength, tokens))
							{
								for (uint32_t tokenIdx = 0; tokenIdx < tokens.size(); tokenIdx++)
								{
									Token t = tokens[tokenIdx];
									if (t.type == TOKEN_TYPE_STRING)
									{
										char *name = &redemptionBuffer[t.start];
										
										if (contains_prefix("id", name))
										{
											// Store Value
											t = tokens[++tokenIdx];
											memcpy(requestID, &redemptionBuffer[t.start], t.end - t.start);
											break;
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
													memcpy(requestID, &redemptionBuffer[t.start], t.end - t.start);
													break;
												}
											}
										}
									}
								}
							}
							
							// Complete the Request using Twtich API
							{
								// Header
								char header[512] = {};
								sprintf(header, 
												"Authorization: Bearer %s\r\nClient-Id: %s\r\nContent-Type: application/json", 
												twitchState->oAuthToken, CLIENT_ID);
								
								// Link
								char link[1024] = {};
								sprintf(link, "helix/channel_points/custom_rewards/redemptions?broadcaster_id=80953740&reward_id=%s&id=%s",
												redemption.twitchRewardID, requestID);
								
								Request completeRedemption = 
									platform_send_http_request(twitchState->twitchAPI, link, header, "PATCH",
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
						break;
					}
					
					default:
					CAKEZ_ASSERT(0, "Unknown Redemption Type: %d", redemption.type);
				}
			}
		}
		
		platform_sleep(3000);
	}
}

#if 0
internal void get_broadcaster_id()
{
	int ID = 0;
	
	char urlBuffer[1024] = {};
	sprintf(urlBuffer, "Authorization: Bearer %s\r\nClient-Id: %s", 
					twitchState->oAuthToken, CLIENT_ID);
	
	Request getBroadcasterID = platform_send_http_request(twitchApi, "helix/users?login=cakez77",
																												urlBuffer, "GET");
	if (!platform_recieve_http_response(getBroadcasterID))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Response: %s", getBroadcasterID.url);
		return -1;
	}
	memset(buffer, 0, 1024);
	if (!platform_recieve_http_data(getBroadcasterID, buffer, 1024))
	{
		CAKEZ_ASSERT(0, "Failed to recieve HTTP Data: %s", getBroadcasterID.url);
		return -1;
	}
	
	//TODO: Parse JSON response and read out the ID field
	
	return ID;
}
#endif
