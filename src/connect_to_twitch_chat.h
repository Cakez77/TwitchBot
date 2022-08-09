#pragma once

#include "defines.h"
#include "logger.h"

#include <ws2tcpip.h>
#include <chrono>

uint32_t constexpr MAX_MSG_LENGTH = 1024;
uint32_t constexpr MAX_RUDE_PERSONS = 100;
uint32_t constexpr MAX_NAME_LENGTH = 256;
constexpr char *RUDE_PERSON_FILE_NAME = "rudePersonCollection.txt";
global_variable SOCKET twitchSocket;

struct Message
{
	char *buffer;
	uint32_t length;
};

bool str_cmp( char *a,  char *b);
void send_recieve_and_print_msg(char *msg);
void send_message(char *msg, int msgLength);
Message recieve_message(char *buffer);

struct RudePerson
{
	uint32_t nameLength;
	char name[MAX_NAME_LENGTH];
	uint32_t rudeCount;
};

struct RudePersonCollection
{
	uint32_t rudePersonCount;
	RudePerson rudePersons[100];
};

void add_rude_person(RudePersonCollection *rudePersonCollection,  char *name, uint32_t nameLength)
{
	CAKEZ_ASSERT(rudePersonCollection, "No RudePersonCollection supplied: %d", rudePersonCollection);
	CAKEZ_ASSERT(name, "No name supplied: %d", name);
	
	if (rudePersonCollection->rudePersonCount < MAX_RUDE_PERSONS)
	{
		RudePerson *rp = &rudePersonCollection->rudePersons[rudePersonCollection->rudePersonCount++];
		*rp = {};
		rp->nameLength = nameLength;
		memcpy(rp->name, name, nameLength);
		rp->rudeCount = 1;
	}
	else
	{
		CAKEZ_ASSERT(0, "Reached maximum amount of rude Persons");
	}
	
	platform_write_file(RUDE_PERSON_FILE_NAME, ( char *)rudePersonCollection, sizeof(RudePersonCollection), true);
}

void connect_to_localhost()
{
}

void connect_to_chat(char *token, SoundState *soundState)
{
	WSAData windowsSocketData;
	WSAStartup(MAKEWORD(1, 2), &windowsSocketData);
	
	addrinfo socketInfo = {};
	addrinfo *result = 0;
	
	socketInfo.ai_family = AF_UNSPEC;
	socketInfo.ai_socktype = SOCK_STREAM;
	socketInfo.ai_protocol = IPPROTO_TCP;
	
	if (!getaddrinfo("irc.chat.twitch.tv", "6667", &socketInfo, &result))
	{
		twitchSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (twitchSocket != INVALID_SOCKET)
		{
			if (!connect(twitchSocket, result->ai_addr, result->ai_addrlen))
			{
				//Connect to twitch using anonymous connection justinfan#
				{
					char authStr[100];
					int length = sprintf(authStr, "PASS oauth:%s\r\n", token);
					send_message(authStr, length);
					// send_recieve_and_print_msg(authStr);
					send_recieve_and_print_msg("NICK Lanzelorder\r\n");
					// send_recieve_and_print_msg(twitchSocket, "NICK lanzelorder\r\n");
				}
				
				// Join channel of broadcaster to listen
				{
					send_recieve_and_print_msg("JOIN #cakez77");
				}
				
				char *periodicMessages[]{
					"PRIVMSG #cakez77 :Interested in creating your own Engine in C++? Take a look at: https://www.youtube.com/playlist?list=PLFAIgTeqcARkeHm-RimFyKET6IZPxlBSt\r\n",
					"PRIVMSG #cakez77 :If you like the Stream consider Following Kappa\r\n",
				};
				char *commands[]{
					"!cakez",
					"!yt",
					"!discord",
					"!poe",
					"!build",
					"!happy",
					"!demo",
					"!chatgame"};
				
				char *replies[]{
					"PRIVMSG #cakez77 :I'm currently working on a tower defence game CakezTD cakez7Rg %s. Idea-> https://www.youtube.com/watch?v=mC8oUPnN6Jg . Gameplay-> https://clips.twitch.tv/IcyMistyCodM4xHeh-CwaedeFFWsFZfbpO\r\n",
					"PRIVMSG #cakez77 :Shameless plug cakez7E %s cakez7E -> https://www.youtube.com/channel/UCL7bBOIdfxxM9WyxTnTk7Qg\r\n",
					"PRIVMSG #cakez77 :Come join our Discord  %s cakez7E -> https://discord.gg/KUCHXVcSFA\r\n",
					"PRIVMSG #cakez77 :Streaming Path of Exile at the end of every programming Stream, if you want to support me %s, keep watching!\r\n",
					"PRIVMSG #cakez77 :Building towards a Raider that uses Ice %s, https://pastebin.com/TDtLLsEN\r\n",
					"PRIVMSG #cakez77 :Are you happy? %s like this? -> peepoClap cakez7Rub cakez7Rub2 cakez7Rub cakez7Rub2 peepoClap LUL\r\n",
					"PRIVMSG #cakez77 :Free Demo available at -> https://cakez77.itch.io/cakeztd <- I hope you like it %s!\r\n",
					"PRIVMSG #cakez77 :You wanna build a Game with us, %s? -> https://github.com/Cakez77/CommunityGame \r\n"};
				
				uint32_t fileSize;
				RudePersonCollection *rudePersonsData = (RudePersonCollection *)platform_read_file(RUDE_PERSON_FILE_NAME, &fileSize);
				RudePersonCollection rudePersons = *rudePersonsData;
				
				std::chrono::steady_clock::time_point lastTimePoint = std::chrono::high_resolution_clock::now();
				float dt = 0.0f;
				float appTime = 0.0f;
				uint32_t msgIdx = 0;
				uint32_t seconds = 0;
				while (true)
				{
					// Take the time it took to update the program
					auto now = std::chrono::high_resolution_clock::now();
					dt = (float)std::chrono::duration<double, std::milli>(now - lastTimePoint).count();
					lastTimePoint = now;
					
					appTime += dt / 1000.0f;
					
					//TODO: Use a bot name
					// if (appTime >= 2400.0f)
					// {
					//     send_message(periodicMessages[msgIdx], strlen(periodicMessages[msgIdx]));
					//     msgIdx += 1;
					//     msgIdx %= ArraySize(periodicMessages);
					//     appTime = 0.0f;
					// }
					
					char buffer[MAX_MSG_LENGTH] = {};
					Message m = recieve_message(buffer);
					
					if (m.length > 0)
					{
						if (str_in_str(m.buffer, "PING"))
						{
							char *pong = "PONG tmi.twitch.tv\r\n";
							send_message(pong, strlen(pong));
						}
						
						CAKEZ_TRACE("%s", m.buffer);
						
						char *msgBegin = m.buffer + 1;
						char *userBegin = 0;
						uint32_t length = 0;
						char userName[200] = {};
						
						while (char c = *(msgBegin++))
						{
							if (c == '@')
							{
								userBegin = msgBegin - 1;
							}
							
							if (c == '\r' || c == '\n')
							{
								*(msgBegin - 1) = ' ';
							}
							
							if (userBegin)
							{
								length++;
							}
							
							if (userBegin && c == '.' && !userName[0])
							{
								memcpy(userName, userBegin, length - 1);
							}
							
							if (c == ':')
							{
								break;
							}
						}
						
						to_lower_case(msgBegin);
						
						for (uint32_t i = 0; i < ArraySize(commands); i++)
						{
							if (strstr(msgBegin, commands[i]))
							{
								char msg[512] = {};
								sprintf(msg, replies[i], userName);
								send_message(msg, strlen(msg));
								
								// Only the first command will be used
								break;
							}
						}
						
						if (strstr(msgBegin, "!commands"))
						{
							char msg[512] = {};
							
							char *msgPrefix = "PRIVMSG #cakez77 : Possible commands are: !rude @user, ";
							uint32_t offset = 0;
							
							uint32_t length = str_length(msgPrefix);
							memcpy(msg + offset, msgPrefix, length);
							offset += length;
							
							for (uint32_t i = 0; i < ArraySize(commands); i++)
							{
								char *command = commands[i];
								length = str_length(command);
								
								memcpy(msg + offset, command, length);
								
								offset += length;
								
								msg[offset++] = ',';
								msg[offset++] = ' ';
							}
							
							offset -= 2;
							
							msg[offset++] = '\r';
							msg[offset++] = '\n';
							
							send_message(msg, strlen(msg));
							continue;
						}
						
						if (strstr(msgBegin, "!rude"))
						{
							// Find @ Symbol and then the username
							
							char *searchString = msgBegin;
							uint32_t nameLength = 0;
							char *rudePersonName = 0;
							bool foundName = false;
							
							while (char c = *(searchString++))
							{
								if (c == '@' && *(searchString - 2) == ' ')
								{
									foundName = true;
									rudePersonName = searchString;
									continue;
								}
								
								if (foundName && (c <= ' '))
								{
									rudePersonName[nameLength] = 0;
									break;
								}
								
								if (foundName)
								{
									nameLength++;
								}
							}
							
							if (nameLength && !str_cmp(rudePersonName, ".disconnect"))
							{
								bool foundRudePerson = false;
								
								for (uint32_t i = 0; i < rudePersons.rudePersonCount; i++)
								{
									RudePerson *rp = &rudePersons.rudePersons[i];
									if (str_cmp(rp->name, rudePersonName))
									{
										char msg[512] = {};
										
										int wasRude = rand() % 25;
										
										if (!wasRude)
										{
											sprintf(msg, "PRIVMSG #cakez77 :Ah STFKUP %s, that wasn't rude!\r\n",
															userName);
											send_message(msg, strlen(msg));
										}
										else
										{
											rp->rudeCount++;
											sprintf(msg, "PRIVMSG #cakez77 :%s has been rude: %d times! Dude, @%s stop being rude!\r\n",
															rp->name, rp->rudeCount, rp->name);
											send_message(msg, strlen(msg));
										}
										foundRudePerson = true;
										platform_write_file(RUDE_PERSON_FILE_NAME, ( char *)&rudePersons, sizeof(RudePersonCollection), true);
										break;
									}
								}
								
								if (!foundRudePerson)
								{
									add_rude_person(&rudePersons, rudePersonName, nameLength);
									for (uint32_t i = 0; i < rudePersons.rudePersonCount; i++)
									{
										RudePerson *rp = &rudePersons.rudePersons[i];
										if (str_cmp(rp->name, rudePersonName))
										{
											
											char msg[512] = {};
											sprintf(msg, "PRIVMSG #cakez77 :%s has been rude for the first time! Dude, @%s you can still stop!\r\n",
															rp->name, rp->name);
											send_message(msg, strlen(msg));
											break;
										}
									}
								}
							}
							else
							{
								// TODO: Reply with User being dumb msg
							}
						}
					}
				}
			}
			else
			{
				CAKEZ_TRACE(0, "Couldn't connect");
			}
		}
		else
		{
			CAKEZ_TRACE(0, "Inavlid Socket");
		}
	}
	else
	{
		CAKEZ_TRACE(0, "Failed getting addressinfo");
	}
}


void send_message(char *msg, int msgLength)
{
	send(twitchSocket, msg, msgLength, 0);
}

Message recieve_message(char *buffer)
{
	Message m = {};
	m.buffer = buffer;
	
	//TODO: twitchSocked coult become NULL I think, maybe reconnect then and log message
	
	m.length = recv(twitchSocket, buffer, MAX_MSG_LENGTH, 0);
	if (m.length == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		
		switch (errorCode)
		{
			case WSANOTINITIALISED:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAENETDOWN:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEFAULT:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAENOTCONN:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEINTR:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEINPROGRESS:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAENETRESET:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAENOTSOCK:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEOPNOTSUPP:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAESHUTDOWN:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEWOULDBLOCK:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEMSGSIZE:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAEINVAL:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAECONNABORTED:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAETIMEDOUT:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
			case WSAECONNRESET:
			{
				CAKEZ_ASSERT(0, 0);
				break;
			}
		}
	}
	
	if (m.length > 0)
	{
		//terminate string
		m.buffer[m.length] = 0;
	}
	
	return m;
}

void send_recieve_and_print_msg(char *msg)
{
	char buffer[MAX_MSG_LENGTH];
	int i = 0;
	
	// Copy contents of msg to the buffer
	while (char c = *msg++)
	{
		buffer[i++] = c;
	}
	
	buffer[i++] = '\r';
	buffer[i++] = '\n';
	buffer[i] = 0;
	
	send_message(buffer, i);
	Message m = recieve_message(buffer);
	if (m.length > 0)
	{
		CAKEZ_TRACE(m.buffer);
	}
}