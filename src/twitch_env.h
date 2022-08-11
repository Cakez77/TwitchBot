#pragma once

char* REFRESH_TOKEN = "39v4curvlhwhu2yw2vfemgigo2fmga2w5l5ti06b5zgfofawgz";
char *CLIENT_ID = "opq32vgncw5znxeyhz4746u4v9mcet";
char *CLIENT_SECRET = "pxxbthehkexttz6o856uws6qtnw9n9";
uint32_t BROADCASTER_ID = 80953740;

char *URL_REFRESH_TWITCH_OAUTH_TOKEN = "oauth2/token?grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s";
char *URL_QUERY_REDEMPTION_ID = "helix/channel_points/custom_rewards/redemptions?broadcaster_id=%d&reward_id=%s&status=UNFULFILLED";