/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/storage.h>

#include <engine/shared/config.h>

#include "sound.h"

#include <math.h>

int CSound::Init()
{
	return 0;
}

int CSound::Update()
{
	return 0;
}

int CSound::Shutdown()
{
	return 0;
}

int CSound::AllocID()
{
	return 1;
}

void CSound::RateConvert(int SampleID)
{
}

int CSound::ReadData(void *pBuffer, int Size)
{
	return 0;
}

int CSound::LoadWV(const char *pFilename)
{
	return 1;
}

void CSound::SetListenerPos(float x, float y)
{
}


void CSound::SetChannel(int ChannelID, float Vol, float Pan)
{
}

int CSound::Play(int ChannelID, int SampleID, int Flags, float x, float y)
{
	return 1;
}

int CSound::PlayAt(int ChannelID, int SampleID, int Flags, float x, float y)
{
	return Play(ChannelID, SampleID, Flags|ISound::FLAG_POS, x, y);
}

int CSound::Play(int ChannelID, int SampleID, int Flags)
{
	return Play(ChannelID, SampleID, Flags, 0, 0);
}

void CSound::Stop(int SampleID)
{
}

void CSound::StopAll()
{
}

IEngineSound *CreateEngineSound() { return new CSound; }

