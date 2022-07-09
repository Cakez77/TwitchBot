#pragma once

#include "defines.h"
#include "huffman_tables.h"
#include "iso_spec_talbes.h"

//##############################################################
//          Stuctures representing the Data Layout
//##############################################################
struct BitStream
{
    uint8_t *start;
    uint32_t numberOfBits;
    uint32_t bitsRead;
    uint32_t byteIdx;
    uint32_t currentBitInByte;
};

#define MASK(n) ((1 << n) - 1)

uint32_t get_bits(BitStream *bitStream, int numberObBitsToRead)
{
    uint32_t result = 0;
    if (bitStream->byteIdx * 8 + bitStream->currentBitInByte + numberObBitsToRead < bitStream->numberOfBits)
    {
        bitStream->bitsRead += numberObBitsToRead;

        for (uint32_t i = 0; i < numberObBitsToRead; i++)
        {
            uint8_t currentByte = *(bitStream->start + bitStream->byteIdx);

            // Read the most significant Bit
            result <<= 1;
            result += (((currentByte >> (7 - bitStream->currentBitInByte++)) & 1));

            if (bitStream->currentBitInByte >= 8)
            {
                bitStream->currentBitInByte = 0;
                bitStream->byteIdx++;
            }
        }
    }
    return result;
}

struct Id3v2Tag
{
    uint8_t id3[3];
    uint8_t majorVersion;
    uint8_t reversion;
    uint8_t unsycronization : 1;
    uint8_t extendedHeader : 1;
    uint8_t experimentalIndicator : 1;
    uint8_t footerPresent : 1;
    uint8_t mustBeCleared : 4;
    uint8_t tagSize[4];
};

struct MP3FrameHeader
{
    // Byte 1
    uint8_t frameSyncFirst8Bits;

    // Byte 2
    uint8_t protectionBit : 1;
    uint8_t layer : 2;
    uint8_t version : 2;
    uint8_t frameSyncLast3Bits : 3;

    // Byte 3
    uint8_t privateBit : 1;
    uint8_t paddingBit : 1;
    uint8_t frequencyIndex : 2;
    uint8_t bitRateIndex : 4;

    // Byte 4
    uint8_t emphasis : 2;
    uint8_t originalBit : 1;
    uint8_t copyrightBit : 1;
    uint8_t stereo : 2;
    uint8_t channel : 2;
};

struct MP3SideInformation
{
    // Byte 1
    uint8_t mainDataBeginFirst8Bits;

    // Byte 2
    // uint8_t byteTwo;
    uint8_t part2_3_lengthFirst7Bits : 7;
    uint8_t mainDataBeginLastBit : 1;

    // Byte 3
    uint8_t bigValuesFirst3Bits : 3;
    uint8_t part2_3_lengthLast5Bits : 5;

    // Byte 4
    uint8_t globalGainFirstTwoBits : 2;
    uint8_t bigValuesLast6Bits : 6;

    // Byte5
    uint8_t scaleFacCompFirstTwoBits : 2;
    uint8_t globalGainLast6Bits : 6;

    // Byte6
    uint8_t padding : 1;
    uint8_t scaleFacCompLast7Bits : 7;

    // // Byte7
    // uint8_t padding : 4;
    // uint8_t windowSwitchingBit : 1;
    // uint8_t scaleFactorCompressionLast3Bits:3;
};

//##############################################################
//                  Self defined Data Structures
//##############################################################
enum MP3Version
{
    MP3_VERSION_2_5,
    MP3_VERSION_RESERVED,
    MP3_VERSION_2,
    MP3_VERSION_1,
};

enum MP3Layer
{
    MP3_LAYER_RESERVED,
    MP3_LAYER_III,
    MP3_LAYER_II,
    MP3_LAYER_I
};

enum MP3ChannelMode
{
    CHANNEL_MODE_STEREO,
    CHANNEL_MODE_JOINT_STEREO,
    CHANNEL_MODE_DUAL_CHANNEL,
    CHANNEL_MODE_SINGLE_CHANNEL
};

enum MP3Emphasis
{
    EMPHASIS_NONE,
    EMPHASIS_50_15_MS,
    EMPHASIS_RESERVED,
    EMPHASIS_CCIT_J_17
};

struct SideInformation
{
    uint32_t dataByteOffset;
};

struct MP3Frame
{
    SideInformation sideInformation;
    MP3Version version;
    MP3Layer layer;
    MP3ChannelMode channelMode;
    uint32_t kbitsPerSecondBitrate;
    uint32_t samplesPerSecond;
    uint32_t sampleCount;
    uint32_t slotSizeInBytes;
    uint32_t sizeInBytes;
    char *dataBegin;
};

struct MP3File
{
    uint32_t frameCount;
    MP3Frame frames[1000];
};

bool parse_mp3(char *data, uint32_t lengthInBytes, float *pcmBuffer)
{
    MP3Frame frame = {};
    Id3v2Tag tag = *(Id3v2Tag *)data;
    // Find the FrameSynchronizer
    {
        uint32_t frameCounter = 0;

        for (uint32_t i = 0; i < lengthInBytes - sizeof(MP3FrameHeader); i++)
        {
            MP3FrameHeader possibleHeader = *((MP3FrameHeader *)(data + i));
            MP3SideInformation sideInformation = *((MP3SideInformation *)(data + i + sizeof(MP3FrameHeader)));

            // 11 Bits need to be set
            if (possibleHeader.frameSyncFirst8Bits == 0xff && possibleHeader.frameSyncLast3Bits == 0x7)
            {
                // Check Version
                {
                    frame.version = (MP3Version)possibleHeader.version;
                    switch (possibleHeader.version)
                    {
                    case MP3_VERSION_2_5:
                        CAKEZ_ERROR("MPEG Version 2.5 (unofficial extension of MPEG 2)");
                        break;

                    case MP3_VERSION_RESERVED:
                        CAKEZ_ERROR("MPEG Version Reserved: %d", possibleHeader);
                        break;

                    case MP3_VERSION_2:
                        CAKEZ_TRACE("MPEG Version 2 (ISO/IEC 13818-3)");
                        break;

                    case MP3_VERSION_1:
                        CAKEZ_TRACE("MPEG Version 1 (ISO/IEC 11172-3)");
                        break;
                    }
                }

                // Check Layer
                {
                    frame.layer = (MP3Layer)possibleHeader.layer;
                    switch (possibleHeader.layer)
                    {
                    case MP3_LAYER_RESERVED:
                        CAKEZ_ERROR("MPEG LAYER RESERVED!");
                        break;
                    case MP3_LAYER_III:
                        CAKEZ_TRACE("MPEG LAYER III");
                        break;
                    case MP3_LAYER_II:
                        CAKEZ_TRACE("MPEG LAYER II");
                        break;
                    case MP3_LAYER_I:
                        CAKEZ_TRACE("MPEG LAYER I");
                        break;
                    }
                }

                // Check for Padding Bit
                {
                    if (possibleHeader.paddingBit)
                    {
                        CAKEZ_TRACE("Frame is padded by 1 Byte");
                    }
                }

                // Check for Protection Bit
                {
                    if (!possibleHeader.protectionBit)
                    {
                        CAKEZ_TRACE("MPEG Protected by 16 bit CRC following header");
                    }
                    else
                    {
                        CAKEZ_TRACE("No CRC");
                    }
                }

                // Extract the Bitrate
                {
                    if (possibleHeader.bitRateIndex == 0)
                    {
                        CAKEZ_ERROR("Bitrate Index FREE is not allowed!");
                    }
                    else if (possibleHeader.bitRateIndex == 15)
                    {
                        CAKEZ_ERROR("Bitrate Index 15 is RESERVED/BAD!");
                    }
                    else
                    {
                        uint16_t free = -1;
                        uint16_t bitrateTable[5][15] =
                            {
                                {free, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
                                {free, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
                                {free, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320},
                                {free, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
                                {free, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
                            };

                        uint32_t colIdx = INVALID_IDX;

                        switch (frame.version)
                        {
                        case MP3_VERSION_1:
                            switch (frame.layer)
                            {
                            case MP3_LAYER_I:
                                colIdx = 0;
                                break;
                            case MP3_LAYER_II:
                                colIdx = 1;
                                break;
                            case MP3_LAYER_III:
                                colIdx = 2;
                                break;
                            }
                            break;

                        case MP3_VERSION_2:
                        case MP3_VERSION_2_5:
                            colIdx = frame.layer == MP3_LAYER_I ? 3 : 4;
                            break;
                        }

                        frame.kbitsPerSecondBitrate = bitrateTable[colIdx][possibleHeader.bitRateIndex];
                    }
                }

                // Extract the sampling rade Index
                {
                    if (possibleHeader.frequencyIndex < 3)
                    {
                        uint32_t samplingRateTable[3][3] =
                            {
                                {44100, 48000, 32000},
                                {22050, 24000, 16000},
                                {11025, 12000, 8000},
                            };

                        uint32_t colIdx = INVALID_IDX;
                        switch (frame.version)
                        {
                        case MP3_VERSION_1:
                            colIdx = 0;
                            break;
                        case MP3_VERSION_2:
                            colIdx = 1;
                            break;
                        case MP3_VERSION_2_5:
                            colIdx = 2;
                            break;
                        }

                        frame.samplesPerSecond = samplingRateTable[colIdx][possibleHeader.frequencyIndex];
                    }
                    else
                    {
                        CAKEZ_ASSERT(0, "Reserved Frequency Index");
                    }
                }

                // Calculate Frame Sample Count
                {
                    frame.sampleCount = 384;

                    if (frame.layer == MP3_LAYER_II)
                    {
                        frame.sampleCount = 1152;
                    }
                    else if (frame.layer == MP3_LAYER_III)
                    {
                        if (frame.version == MP3_VERSION_1)
                        {
                            frame.sampleCount = 1152;
                        }
                        else
                        {
                            frame.sampleCount = 576;
                        }
                    }
                }

                // Calculate Frame Size
                {
                    frame.slotSizeInBytes = 4;
                    if (frame.layer != MP3_LAYER_I)
                    {
                        frame.slotSizeInBytes = 1;
                    }
                    uint32_t padding = possibleHeader.paddingBit ? frame.slotSizeInBytes : 0;

                    frame.sizeInBytes = (frame.sampleCount * (frame.kbitsPerSecondBitrate * 1000 / 8)) / frame.samplesPerSecond + padding;
                }

                // Check if Copyrighted
                {
                    if (possibleHeader.copyrightBit)
                    {
                        CAKEZ_TRACE("MPEG is copyrighted");
                    }
                }

                // Check if Original
                {
                    if (possibleHeader.originalBit)
                    {
                        CAKEZ_TRACE("MPEG is original! Hell yeah brother!");
                    }
                }

                // Check the Emphasis
                {
                    switch (possibleHeader.emphasis)
                    {
                    case EMPHASIS_NONE:
                        CAKEZ_ERROR("MPEG Emphasis is NONE!");
                        break;
                    case EMPHASIS_50_15_MS:
                        CAKEZ_TRACE("MPEG Emphasis is 50/15 ms!");
                        break;
                    case EMPHASIS_RESERVED:
                        CAKEZ_ERROR("MPEG Emphasis is RESERVED!");
                        break;
                    case EMPHASIS_CCIT_J_17:
                        CAKEZ_TRACE("MPEG Emphasis is CCIT J.17!");
                        break;
                    }
                }

                // Check Channel Mode
                {
                    switch (possibleHeader.channel)
                    {
                    case CHANNEL_MODE_STEREO:
                        CAKEZ_TRACE("MPEG channel mode STEREO");
                        break;
                    case CHANNEL_MODE_JOINT_STEREO:
                        CAKEZ_TRACE("MPEG channel mode JOINT STEREO");
                        break;
                    case CHANNEL_MODE_DUAL_CHANNEL:
                        CAKEZ_TRACE("MPEG channel mode DUAL");
                        break;
                    case CHANNEL_MODE_SINGLE_CHANNEL:
                        CAKEZ_TRACE("MPEG channel mode SINGLE");
                        break;
                    }
                }

                // Extract data from Side Information
                {
                    uint8_t *sideInfoStart = (uint8_t *)(data + i + sizeof(MP3FrameHeader));
                    BitStream bitStream = {};
                    bitStream.start = sideInfoStart;
                    bitStream.numberOfBits = frame.sizeInBytes * 8;

                    if (!possibleHeader.protectionBit)
                    {
                        get_bits(&bitStream, 16);
                    }

                    uint32_t mainDataBegin = get_bits(&bitStream, 9);
                    uint32_t part2_3_length = get_bits(&bitStream, 12);
                    uint32_t bigValues = get_bits(&bitStream, 9);
                    uint32_t globalGain = get_bits(&bitStream, 8);
                    uint32_t scaleFacComp = get_bits(&bitStream, 9);

                    uint32_t windowSwitching = get_bits(&bitStream, 1);
                    uint32_t blockType = 0;
                    if (windowSwitching)
                    {
                        CAKEZ_ASSERT(0, "Window Switching Sadge!");
                    }

                    uint32_t tableIndices[3] = {};

                    // Extract the table Index for every Region
                    for (uint32_t i = 0; i < 3; i++)
                    {
                        // Are these the Indices into the Huffman Tables?
                        tableIndices[i] = get_bits(&bitStream, 5);
                    }

                    uint32_t region0Count = get_bits(&bitStream, 4);
                    uint32_t region1Count = get_bits(&bitStream, 3);
                    // Since this is MP3 Version 2 we don't get the preflag
                    uint32_t preflag = false ? get_bits(&bitStream, 1) : 0;

                    uint32_t scaleFacScale = get_bits(&bitStream, 1);
                    uint32_t count1Table = get_bits(&bitStream, 1);

                    // Now we start reading from part2_3_legnth (Scalefactors + Huffman Data)
                    uint32_t bitsReadBeforePart2_3 = bitStream.bitsRead;

                    uint32_t slen1 = (scaleFacComp >> 4) / 5;
                    uint32_t slen2 = (scaleFacComp >> 4) % 5;
                    uint32_t slen3 = (scaleFacComp % 16) >> 2;
                    uint32_t slen4 = scaleFacComp % 4;

                    /** 
                         * Since block type is 0 (because window switching flag = 0)
                         * we define the number of scale factor bands as follows
                        */
                    uint32_t nr_of_sfb_1 = 6;
                    uint32_t nr_of_sfb_2 = 5;
                    uint32_t nr_of_sfb_3 = 5;
                    uint32_t nr_of_sfb_4 = 5;

                    uint32_t scaleFacLengthTable[21] = {
                        // Lengths for Region 0
                        slen1, slen1, slen1, slen1, slen1, slen1,
                        // Lengths for Region 1
                        slen2, slen2, slen2, slen2, slen2,
                        // Lengths for Region 2
                        slen3, slen3, slen3, slen3, slen3,
                        // Lengths for Region 3
                        slen4, slen4, slen4, slen4, slen4};

                    uint32_t granuleCount = 1;
                    uint32_t channelCount = 1;
                    uint32_t scaleFacTable[21] = {};

                    for (uint32_t g = 0; g < granuleCount; g++)
                    {
                        for (uint32_t c = 0; c < channelCount; c++)
                        {
                            for (uint32_t sfb = 0; sfb < 21; sfb++)
                            {
                                scaleFacTable[sfb] = get_bits(&bitStream, scaleFacLengthTable[sfb]);
                            }
                        }
                    }

                    int somethingSamples[576] = {};
                    // Read Huffman Data
                    {

                        uint32_t sampleCount = 0;

                        // Decode Pairs from Huffman Data
                        uint32_t tableSelectIdx = 0;
                        uint32_t scaleFactorBandIdx = 0;
                        uint32_t region_counts[3] =
                            {
                                region0Count,
                                region1Count,
                                255 // region2Count goes up until bigValues, 255 always exceeds that,
                            };

                        uint32_t decodedPairCount = 0;
                        while (decodedPairCount < bigValues)
                        {
                            uint32_t scaleFactorBandCount = region_counts[tableSelectIdx];
                            uint32_t tableIdx = tableIndices[tableSelectIdx];
                            HuffmanTable huffmanTable = get_huffman_table(tableIdx);
                            uint32_t linbits = linbitsTables[tableIdx];

                            for (uint32_t i = 0; i <= scaleFactorBandCount; i++)
                            {
                                if (decodedPairCount >= bigValues)
                                {
                                    break;
                                }

                                uint32_t numberOfSamplesToDecode = scaleFactorBands[scaleFactorBandIdx].width;
                                uint32_t numberOfPairs = numberOfSamplesToDecode / 2;

                                // Limit the pairs/samples to bigValues * 2
                                if (numberOfPairs > bigValues - decodedPairCount)
                                {
                                    numberOfPairs = bigValues - decodedPairCount;
                                }

                                for (uint32_t j = 0; j < numberOfPairs; j++)
                                {
                                    int number = 0;
                                    uint32_t hlen = 0;
                                    bool foundLeaf = false;

                                    while (!foundLeaf)
                                    {
                                        hlen++;
                                        int bit = get_bits(&bitStream, 1);
                                        number = (number << 1) + bit;

                                        for (uint32_t k = 0; k < huffmanTable.entryCount; k++)
                                        {
                                            if (huffmanTable.huffmanEntries[k].hlen == hlen &&
                                                huffmanTable.huffmanEntries[k].code == number)
                                            {
                                                int x = huffmanTable.huffmanEntries[k].x;
                                                int y = huffmanTable.huffmanEntries[k].y;

                                                if (x == 15)
                                                {
                                                    x += get_bits(&bitStream, linbits);
                                                }

                                                if (x > 0 && get_bits(&bitStream, 1) == 1)
                                                {
                                                    x = -x;
                                                }
                                                if (y == 15)
                                                {
                                                    y += get_bits(&bitStream, linbits);
                                                }

                                                if (y > 0 && get_bits(&bitStream, 1) == 1)
                                                {
                                                    y = -y;
                                                }

                                                somethingSamples[sampleCount++] = x;
                                                somethingSamples[sampleCount++] = y;

                                                foundLeaf = true;
                                                break;
                                            }
                                        }
                                    }

                                    // Every Huffman Entriy gives us two Samples
                                    decodedPairCount++;
                                }

                                // Look at next Scalefactor Band
                                scaleFactorBandIdx++;
                            }

                            tableSelectIdx++;
                        }

                        // Decode count 1 region
                        {
                            Count1RegionTable tableIdx = TABLE_A;
                            if (count1Table)
                            {
                                tableIdx = TABLE_B;
                            }
                            HuffmanTableCount1Region huffmanTable = count1RegionTables[tableIdx];

                            while (bitStream.bitsRead - bitsReadBeforePart2_3 < part2_3_length)
                            {
                                int number = 0;
                                uint32_t hlen = 0;
                                bool foundLeaf = false;

                                while (!foundLeaf)
                                {
                                    hlen++;
                                    int bit = get_bits(&bitStream, 1);
                                    number = (number << 1) + bit;

                                    for (uint32_t k = 0; k < huffmanTable.entryCount; k++)
                                    {
                                        if (huffmanTable.huffmanEntries[k].hlen == hlen &&
                                            huffmanTable.huffmanEntries[k].code == number)
                                        {
                                            int v = huffmanTable.huffmanEntries[k].v;
                                            int w = huffmanTable.huffmanEntries[k].w;
                                            int x = huffmanTable.huffmanEntries[k].x;
                                            int y = huffmanTable.huffmanEntries[k].y;

                                            if (v > 0 && get_bits(&bitStream, 1) == 1)
                                            {
                                                v = -v;
                                            }

                                            if (w > 0 && get_bits(&bitStream, 1) == 1)
                                            {
                                                w = -w;
                                            }

                                            if (x > 0 && get_bits(&bitStream, 1) == 1)
                                            {
                                                x = -x;
                                            }

                                            if (y > 0 && get_bits(&bitStream, 1) == 1)
                                            {
                                                y = -y;
                                            }

                                            somethingSamples[sampleCount++] = v;
                                            somethingSamples[sampleCount++] = w;
                                            somethingSamples[sampleCount++] = x;
                                            somethingSamples[sampleCount++] = y;

                                            foundLeaf = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            // Zero out last samples
                            for (uint32_t i = sampleCount; i < 576; i++)
                            {
                                somethingSamples[i] = 0;
                            }
                        }
                    }

                    uint32_t sampleIdx = 0;
                    float floatSamples[576] = {};
                    // Requantize The Samples
                    {
                        float scaleFacMultiplyer = scaleFacScale ? 1.0f : 0.5f;

                        for (uint32_t scaleFactorBandIdx = 0; scaleFactorBandIdx < 21; scaleFactorBandIdx++)
                        {
                            uint32_t sampleCount = scaleFactorBands[scaleFactorBandIdx].width;
                            float scaleFac = (float)scaleFacTable[scaleFactorBandIdx];

                            float D = scaleFacMultiplyer * scaleFac; /*+ preflag * pretab */

                            float C = 1.0f / 4.0f * ((float)globalGain - 210.0f); // 210 is per spec

                            // Scale the Samples
                            {
                                for (uint32_t i = 0; i < sampleCount; i++)
                                {
                                    int sample = somethingSamples[sampleIdx];

                                    if (sample < 0)
                                    {
                                        floatSamples[sampleIdx] =
                                            -(float)(pow(-sample, 4.0 / 3.0) * pow(2, (double)C) * pow(2, (double)D));
                                    }
                                    else
                                    {
                                        floatSamples[sampleIdx] =
                                            (float)(pow(sample, 4.0 / 3.0) * pow(2, (double)C) * pow(2, (double)D));
                                    }

                                    sampleIdx++;
                                }
                            }
                        }
                    }

                    // Reorder the Samples
                    {
                        if (windowSwitching /*blockType == 2*/)
                        {
                            CAKEZ_ASSERT(0, "We need to reorder");
                        }
                    }

                    // Maybe I need to work on the float Samples
                    float antiAliasSamples[576] = {};
                    memcpy(antiAliasSamples, floatSamples, 576 * sizeof(float));
                    // Antialiase the Samples
                    {
                        float butterflyCoefficients[2][8] =
                            {{0.85749293f, 0.88174200f, 0.94962865f, 0.98331459f, 0.99551782f, 0.99916056f, 0.99989920f, 0.99999316f},
                             {-0.51449576f, -0.47173197f, -0.31337745f, -0.18191320f, -0.09457419f, -0.04096558f, -0.01419856f, -0.00369997f}};

                        uint32_t scaleFacBandCount = 0;
                        if (blockType != 0b10)
                        {
                            scaleFacBandCount = 32;
                        }
                        else
                        {
                            CAKEZ_ASSERT(0, "Check for mixed Blocks");
                            // if(mixed_block_flag[gr][ch]==’0’)
                            // sb_amount=0;
                            // else
                            // if((IDex==’0’)&&(sampling_frequency==’10’))
                            // sb_amount=4;
                            // else
                            // sb_amount=2;
                        }

                        for (uint32_t i = 1; i < scaleFacBandCount; i++)
                        {
                            for (uint32_t j = 0; j < 8; j++)
                            {
                                // Butterfly selection of Samples
                                float a = floatSamples[i * 18 + j];
                                float b = floatSamples[i * 18 - (j + 1)];

                                antiAliasSamples[i * 18 + j] =
                                    a * butterflyCoefficients[0][j] + b * butterflyCoefficients[1][j];
                                antiAliasSamples[i * 18 - (j + 1)] =
                                    b * butterflyCoefficients[0][j] - a * butterflyCoefficients[1][j];
                            }
                        }
                    }

                    float samplesAfterIMDCT[576] = {};
                    // IMDCT
                    {
                        uint32_t sampleIdx = 0;
                        float overlappedSamples[18] = {};

                        for (uint32_t subBandIdx = 0; subBandIdx < 32; subBandIdx++)
                        {
                            uint32_t n = 12;
                            if (blockType == 2) // Short windows?
                            {
                            }
                            else // Long Windows
                            {
                                n = 36;
                                float xi = 0.0f;
                                float timeSamples[36] = {};

                                // Polyphase Filter Subband samples
                                for (uint32_t i = 0; i < n; i++)
                                {
                                    // Evaluate xi
                                    {
                                        for (uint32_t k = 0; k < 18; k++)
                                        {
                                            // 18 * 32 = 576
                                            float sample = floatSamples[subBandIdx * 18 + k];

                                            xi += sample * cos(C_PI / 72 * (2 * i + 1 + 18) * (2 * k + 1));
                                        }
                                    }

                                    timeSamples[i] = xi;
                                }

                                for (uint32_t i = 0; i < 18; i++)
                                {
                                    samplesAfterIMDCT[sampleIdx++] = floatSamples[i] + overlappedSamples[i];

                                    // Store overlapped Samples
                                    overlappedSamples[i] = floatSamples[i + 18];
                                }
                            }
                        }
                    }

                    float frequencyInvertedSamples[576] = {};
                    // Frequency Inversion
                    {
                        for (uint32_t sb = 0; sb < 32; sb += 2)
                        {
                            for (uint32_t i = 0; i < 18; i += 2)
                            {
                                frequencyInvertedSamples[sb * 18 + i] =
                                    -samplesAfterIMDCT[sb * 18 + i];
                            }
                        }
                    }

                    float pcmSamples[576] = {};
                    // Filter Bank, shifting some Barrels bro
                    {
                        // MDCT Section
                        {
                            float barrelShifter[1024] = {};

                            for (uint32_t subBandSampleIdx = 0; subBandSampleIdx < 18; subBandSampleIdx++)
                            {
                                // Shift up for every subBandSampleIdx
                                for (uint32_t i = 1023; i > 63; i--)
                                {
                                    // Doing some Barrel Shifting, bro
                                    barrelShifter[i] = barrelShifter[i - 64];
                                }

                                // Calculate Yi | 0 <= 1 < 64, and store in barrelShifter
                                for (uint32_t i = 0; i < 64; i++)
                                {
                                    float sum = 0.0f;

                                    for (uint32_t sb = 0; sb < 32; sb++)
                                    {
                                        // N ik * S k
                                        float Sk = frequencyInvertedSamples[sb * 18 + subBandSampleIdx];
                                        float Nik = cos(C_PI / 64 * (16 + i) * (2 * sb + 1));

                                        sum += Nik * Sk;
                                    }

                                    barrelShifter[i] = sum;
                                }

                                // Remove Redundancy of the MDCT Process 1024 -> 512, 50% Redundancy
                                {
                                    float uniqueValues[512] = {};

                                    /**
                                                 * Partition the Barrel Shifter into 16 Pieces,
                                                 * each consiting of 64 Values
                                                 */
                                    for (uint32_t i = 0; i < 8; i++)
                                    {
                                        // 2 * 32 = 64 Values for every 128 Values
                                        for (uint32_t j = 0; j < 32; j++)
                                        {
                                            uniqueValues[i * 64 + j] = barrelShifter[i * 128 + j];
                                            uniqueValues[i * 64 + 32 + j] = barrelShifter[i * 128 + j + 96];
                                        }
                                    }

                                    // Apply the Windowing to the Values
                                    for (uint32_t i = 0; i < 512; i++)
                                    {
                                        uniqueValues[i] *= filterBankWindowingDTable[i];
                                    }

                                    float outputSamples[32] = {};
                                    // Generate 32 Output Samples
                                    {
                                        for (uint32_t i = 0; i < 32; i++)
                                        {
                                            float sum = 0.0f;
                                            for (uint32_t j = 0; j < 16; j++)
                                            {
                                                sum += uniqueValues[j * 32 + i];
                                            }

                                            pcmSamples[subBandSampleIdx * 32 + i] = sum;

                                            // if (pcmSamples[subBandSampleIdx] < 0.0f)
                                            // {
                                            //     pcmSamples[subBandSampleIdx] *= INT16_MIN;
                                            // }
                                            // else
                                            // {
                                            //     pcmSamples[subBandSampleIdx] *= INT16_MAX;
                                            // }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    frameCounter++;
                    memcpy(pcmBuffer + frameCounter * 576, pcmSamples, 576 * sizeof(float));
                    i += frame.sizeInBytes - 1;
                }
            }
        }

        CAKEZ_TRACE("Found %d, Frames", frameCounter);
    }
    return true;
}