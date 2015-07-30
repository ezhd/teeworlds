/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>

#include "input.h"

//print >>f, "int inp_key_code(const char *key_name) { int i; if (!strcmp(key_name, \"-?-\")) return -1; else for (i = 0; i < 512; i++) if (!strcmp(key_strings[i], key_name)) return i; return -1; }"

// this header is protected so you don't include it from anywere
#define KEYS_INCLUDE
#include "keynames.h"
#undef KEYS_INCLUDE

CInput::CInput()
{
}

void CInput::Init()
{
}

void CInput::MouseRelative(float *x, float *y)
{
}

void CInput::MouseModeAbsolute()
{
}

void CInput::MouseModeRelative()
{
}

int CInput::MouseDoubleClick()
{
	return 0;
}

void CInput::ClearKeyStates()
{
}

int CInput::KeyState(int Key)
{
	return 0;
}

int CInput::Update()
{
	return 0;
}

int CInput::VideoRestartNeeded()
{
	return 0;
}

void CInput::ReadGyroscopeInput(float *x, float *y, float *z)
{
}

IEngineInput *CreateEngineInput() { return new CInput; }
