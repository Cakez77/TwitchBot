#pragma once

char REFRESH_TOKEN[DEFAULT_BUFFER_SIZE] = {};
char CLIENT_ID[DEFAULT_BUFFER_SIZE] = {};
char CLIENT_SECRET[DEFAULT_BUFFER_SIZE] = {};
uint32_t BROADCASTER_ID = 0;

char *URL_REFRESH_TWITCH_OAUTH_TOKEN = "oauth2/token?grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s";
char *URL_QUERY_REDEMPTION_ID = "helix/channel_points/custom_rewards/redemptions?broadcaster_id=%d&reward_id=%s&status=UNFULFILLED";