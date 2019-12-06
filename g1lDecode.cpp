// Copyright (C) 2019  CBH <maodatou88@163.com>
// Licensed under the terms of MIT license.
// See file LICENSE for detail.

#include "g1lDecode.h"

using namespace std;

int g1lInit(g1lFile &g1l, char *fileName);
int g1lClose(g1lFile &g1l);
int steamDecode(g1lFile &g1l, u32 index);
int steamConvert(g1lFile &g1l, u32 index, char *wavName);

u16 Low2Big_u16(u16 a);
u32 Low2Big_u32(u32 a);
u16 fread16(FILE *f, u8 endian = 0);
u32 fread32(FILE *f, u8 endian = 0);
void fwrite16(u16 i, FILE *f, u8 endian = 0);
void fwrite32(u32 i, FILE *f, u8 endian = 0);

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage:\n");
		printf("g1lDecode <g1l_file> [wav_file]\n");
		return 0;
	}

	g1lFile g1l;
	if (g1lInit(g1l, argv[1]) != 0) return -1;
	printf("decoding: %s\n", argv[1]);

	char dirName[260];
	u32 tia = 0, tib = 0;
	for (u32 i = strlen(argv[1]) - 1; i > 0; i--) if (argv[1][i] == '.') { tia = i; break; }
	for (u32 i = 0; i < tia; i++) dirName[tib++] = argv[1][i];
	dirName[tib] = '\0';

	char wavName[260];
	for (u32 i = 0; i < g1l.num; i++)
	{
		if (steamDecode(g1l, i) == -1) continue;
		if (argc > 2) sprintf(wavName, "%s", argv[2]); else sprintf(wavName, "%s_%d.wav", dirName, i);
		steamConvert(g1l, i, wavName);
	}
	g1lClose(g1l);
	return 0;
}

int g1lInit(g1lFile &g1l, char *fileName)
{
	g1l.file = fopen(fileName, "rb");
	if (g1l.file == NULL) return -1;
	u32 identifier = fread32(g1l.file);
	if (identifier == 0x47314C5F) g1l.endian = 0;
	else if (identifier == 0x5F4C3147) g1l.endian = 1;
	else
	{
		fclose(g1l.file);
		return -1;
	}
	g1l.version = fread32(g1l.file, g1l.endian);
	g1l.fileSize = fread32(g1l.file, g1l.endian);
	g1l.headSize = fread32(g1l.file, g1l.endian);
	g1l.type = fread32(g1l.file, g1l.endian);
	if (g1l.type != 0x09)
	{
		fclose(g1l.file);
		return -1;
	}
	g1l.num = fread32(g1l.file, g1l.endian);
	for (u32 i = 0; i < g1l.num; i++)
	{
		g1lStream stream;
		stream.offset = fread32(g1l.file, g1l.endian);
		g1l.streams.push_back(stream);
	}
	for (u32 i = 0; i < g1l.num; i++)
	{
		fseek(g1l.file, g1l.streams[i].offset, 0);
		char name[9] = {};
		for (u32 j = 0; j < 8; j++) name[j] = getc(g1l.file);
		if (!strcmp(name, "WiiVoice")) g1l.streams[i].type = WiiVoice;
		if (!strcmp(name, "WiiBGM")) g1l.streams[i].type = WiiBGM;
		switch (g1l.streams[i].type)
		{
		case WiiVoice:
			g1l.streams[i].dataOffset = g1l.streams[i].offset + 0x80;
			g1l.streams[i].headSize = fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			g1l.streams[i].streamSize = fread32(g1l.file, g1l.endian);
			g1l.streams[i].streamSizeRel = fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleNum = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleSize = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleRate = fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			g1l.streams[i].channel = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleSizeRel = fread32(g1l.file, g1l.endian);
			g1l.streams[i].channelRel = fread32(g1l.file, g1l.endian);
			for (u32 j = 0; j < 0x10; j++) g1l.streams[i].coef[0][j] = fread16(g1l.file, g1l.endian);
			g1l.streams[i].SampleNumRel = g1l.streams[i].SampleNum;
			if (g1l.streams[i].SampleNumRel % 0x0E != 0) g1l.streams[i].SampleNumRel += 0x0E - g1l.streams[i].SampleNumRel % 0x0E;
			g1l.streams[i].frameNum = g1l.streams[i].SampleNumRel / 0x0E;
			g1l.streams[i].dataSize = g1l.streams[i].frameNum * 0x08;
			if (g1l.streams[i].channel != 2 || g1l.streams[i].dataSize != g1l.streams[i].streamSizeRel - 0x60)
			{
				printf("incorrect parameters in stream %d!\n", i);
				g1lClose(g1l);
				return -1;
			}
			g1l.streams[i].channel = 1;
			fseek(g1l.file, g1l.streams[i].dataOffset, 0);
			vector<u8>(g1l.streams[i].dataSize).swap(g1l.streams[i].data);
			for (u32 j = 0; j < g1l.streams[i].dataSize; j++) g1l.streams[i].data[j] = getc(g1l.file);
			vector<s32>(g1l.streams[i].SampleNumRel * g1l.streams[i].channel).swap(g1l.streams[i].PCM);
			break;
		case WiiBGM:
			g1l.streams[i].dataOffset = g1l.streams[i].offset + 0x800;
			g1l.streams[i].headSize = fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			fseek(g1l.file, 0x30, 1);
			g1l.streams[i].SampleNum = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleSize = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleRate = fread32(g1l.file, g1l.endian);
			fread32(g1l.file, g1l.endian);
			g1l.streams[i].channel = fread32(g1l.file, g1l.endian);
			g1l.streams[i].SampleSizeRel = fread32(g1l.file, g1l.endian);
			g1l.streams[i].channelRel = fread32(g1l.file, g1l.endian);
			for (u32 j = 0; j < 0x10; j++) g1l.streams[i].coef[0][j] = fread16(g1l.file, g1l.endian);
			fseek(g1l.file, 0x40, 1);
			for (u32 j = 0; j < 0x10; j++) g1l.streams[i].coef[1][j] = fread16(g1l.file, g1l.endian);
			g1l.streams[i].SampleNumRel = g1l.streams[i].SampleNum;
			if (g1l.streams[i].SampleNumRel % 0x0E != 0) g1l.streams[i].SampleNumRel += 0x0E - g1l.streams[i].SampleNumRel % 0x0E;
			g1l.streams[i].frameNum = g1l.streams[i].SampleNumRel / 0x0E;
			g1l.streams[i].dataSize = g1l.streams[i].frameNum * 0x10;
			if (g1l.streams[i].channel != 2)
			{
				printf("incorrect parameters in stream %d!\n", i);
				g1lClose(g1l);
				return -1;
			}
			fseek(g1l.file, g1l.streams[i].dataOffset, 0);
			vector<u8>(g1l.streams[i].dataSize).swap(g1l.streams[i].data);
			for (u32 j = 0; j < g1l.streams[i].dataSize; j++) g1l.streams[i].data[j] = getc(g1l.file);
			vector<s32>(g1l.streams[i].SampleNumRel * g1l.streams[i].channel).swap(g1l.streams[i].PCM);
			break;
		default:;
		}
	}
	return 0;
}

int steamConvert(g1lFile &g1l, u32 index, char *wavName)
{
	FILE *wav = fopen(wavName, "wb");
	if (wav == NULL) return -1;
	u32 dataSize = g1l.streams[index].SampleNum * 4;
	u8 endian = 1;
	fwrite32(0x46464952, wav, endian);
	fwrite32(dataSize + 0x24, wav, endian);
	fwrite32(0x45564157, wav, endian);
	fwrite32(0x20746D66, wav, endian);
	fwrite32(0x10, wav, endian);
	fwrite16(0x01, wav, endian);
	fwrite16(0x02, wav, endian);
	fwrite32(g1l.streams[index].SampleRate, wav, endian);
	fwrite32(2 * g1l.streams[index].SampleRate * 0x10 / 8, wav, endian);
	fwrite16(2 * 0x10 / 8, wav, endian);
	fwrite16(0x10, wav, endian);
	fwrite32(0x61746164, wav, endian);
	fwrite32(dataSize, wav, endian);
	u32 data = 0;
	for (u32 j = 0; j < g1l.streams[index].SampleNum; j++)
	{
		int PCML = g1l.streams[index].PCM[j * g1l.streams[index].channel];
		int PCMR = g1l.streams[index].channel == 2 ? g1l.streams[index].PCM[j * g1l.streams[index].channel + 1] : g1l.streams[index].PCM[j];
		fwrite16(PCML, wav, endian);
		fwrite16(PCMR, wav, endian);
	}
	fclose(wav);
	return 0;
}

int g1lClose(g1lFile &g1l)
{
	fclose(g1l.file);
	for (size_t i = 0; i < g1l.streams.size(); i++)
	{
		vector<u8>().swap(g1l.streams[i].data);
		vector<s32>().swap(g1l.streams[i].PCM);
	}
	vector<g1lStream>().swap(g1l.streams);
	return 0;
}

int steamDecode(g1lFile &g1l, u32 index)
{
	u8 frame[0x08];
	u32 frameCount = 0;
	s32 record[2][2] = {};
	s32 nibble2int[16] = { 0,1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3,-2,-1 };
	while (frameCount < g1l.streams[index].frameNum)
	{
		for (u32 ch = 0; ch < g1l.streams[index].channel; ch++)
		{
			for (u32 i = 0; i < 0x08; i++) frame[i] = g1l.streams[index].data[(frameCount * 0x08 + i) * g1l.streams[index].channel + ch];

			s16 coefIndex, coef1, coef2;
			s32 scale = 1;
			s32 hist1 = record[ch][0];
			s32 hist2 = record[ch][1];
			scale = 1 << (frame[0] & 0xf);
			coefIndex = (frame[0] >> 4) & 0xf;
			if (coefIndex > 8)
			{
				printf("incorrect coefs in stream %d at %X!\n", index, g1l.streams[index].dataOffset + frameCount * 0x08 * g1l.streams[index].channel);
				return -1;
			}
			coef1 = g1l.streams[index].coef[ch][coefIndex * 2];
			coef2 = g1l.streams[index].coef[ch][coefIndex * 2 + 1];

			u32 frameSampleNum = 0x0E;
			u32 frameSampleIndex = frameCount * 0x0E;
			u32 PCMIndex = frameSampleIndex * g1l.streams[index].channel + ch;
			if (frameSampleIndex + frameSampleNum > g1l.streams[index].SampleNum) frameSampleNum = g1l.streams[index].SampleNum - frameSampleIndex;
			for (u32 i = 0; i < frameSampleNum; i++)
			{
				u8 nibbles = frame[0x01 + i / 2];
				int sample = i & 1 ? nibble2int[nibbles & 0x0F] : nibble2int[nibbles >> 4];
				sample = (((sample * scale) << 11) + 1024 + coef1 * hist1 + coef2 * hist2) >> 11;
				if ((sample >> 15) ^ (sample >> 31)) sample = 0x7FFF ^ (sample >> 31);
				g1l.streams[index].PCM[PCMIndex] = sample;
				PCMIndex += g1l.streams[index].channel;
				hist2 = hist1;
				hist1 = sample;
			}
			record[ch][0] = hist1;
			record[ch][1] = hist2;
		}
		frameCount++;
	}
	return 0;
}

u16 Low2Big_u16(u16 a)
{
	u16 b = 0;
	b = (a & 0xFF00) >> 8 | (a & 0xFF) << 8;
	return b;
}

u32 Low2Big_u32(u32 a)
{
	u32 b = 0;
	b = (a & 0xFF000000) >> 24 | (a & 0xFF0000) >> 8 | (a & 0xFF00) << 8 | (a & 0xFF) << 24;
	return b;
}

u16 fread16(FILE *f, u8 endian)
{
	u16 p = 0;
	fread(&p, sizeof(u16), 1, f);
	return endian ? p : Low2Big_u16(p);
}

u32 fread32(FILE *f, u8 endian)
{
	u32 p = 0;
	fread(&p, sizeof(u32), 1, f);
	return endian ? p : Low2Big_u32(p);
}

void fwrite16(u16 i, FILE *f, u8 endian)
{
	u16 p = endian ? i : Low2Big_u16(i);
	fwrite(&p, sizeof(u16), 1, f);
}

void fwrite32(u32 i, FILE *f, u8 endian)
{
	u32 p = endian ? i : Low2Big_u32(i);
	fwrite(&p, sizeof(u32), 1, f);
}
