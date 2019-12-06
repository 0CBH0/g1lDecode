// Copyright (C) 2019  CBH <maodatou88@163.com>
// Licensed under the terms of MIT license.
// See file LICENSE for detail.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sndfile.h>

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef signed   char	s8;
typedef signed   short	s16;
typedef signed   int    s32;
typedef unsigned long	ulg;

enum StreamType
{
	WiiBGM,
	WiiVoice
};

struct g1lStream
{
	u32 offset;
	StreamType type;
	u32 headSize;
	u32 streamSize;
	u32 streamSizeRel;
	u32 dataOffset;
	u32 dataSize;
	u32 SampleNum;
	u32 SampleSize;
	u32 SampleRate;
	u32 SampleNumRel;
	u32 SampleSizeRel;
	u32 frameNum;
	u32 channelRel;
	u32 channel;
	u16 coef[2][0x10];
	std::vector<u8> data;
	std::vector<s32> PCM;
};

struct g1lFile
{
	u32 version;
	u32 type;
	u32 num;
	u8 endian;
	u32 fileSize;
	u32 headSize;
	FILE *file;
	std::vector<g1lStream> streams;
};
