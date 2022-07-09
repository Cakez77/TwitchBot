/*
 * Author: Floris Creyf
 * Date: May 2015
 * A simplistic MPEG-1 layer 3 decoder.
 */

#include "util.cpp"
#include "id3.cpp"
#include "mp3.cpp"
#include "xing.cpp"

#define ALSA_PCM_NEW_HW_PARAMS_API

/**
 * Start decoding the MP3 and let ALSA hand the PCM stream over to a driver.
 * @param decoder
 * @param buffer A buffer containing the MP3 bit stream.
 * @param offset An offset to a MP3 frame header.
 */
inline void stream(mp3 &decoder, unsigned char *buffer, uint32_t bufferSize, unsigned offset)
{
	/* Start decoding. */
	while (decoder.is_valid() && bufferSize > offset + decoder.get_header_size())
	{
		decoder.init_header_params(&buffer[offset]);
		if (decoder.is_valid())
		{
			decoder.init_frame_params(&buffer[offset]);
			offset += decoder.get_frame_size();
		}

		float *pcmFloats = decoder.get_samples();

		for (uint32_t i = 0; i < 576; i++)
		{
			if (pcmFloats[i] > 0.0f)
			{
				pcmFloats[i] *= 32767.0f;
			}
			else
			{
				pcmFloats[i] *= 32768.0f;
			}
		}
		int a = 0;
	}
}

std::vector<id3> get_id3_tags(unsigned char *buffer, unsigned &offset)
{
	std::vector<id3> tags;
	int i = 0;
	bool valid = true;

	while (valid)
	{
		id3 tag(&buffer[offset]);
		if (valid = tag.is_valid())
		{
			tags.push_back(tag);
			offset += tags[i++].get_id3_offset() + 10;
		}
	}

	return tags;
}
