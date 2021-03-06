/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#if defined(__ANDROID__)
#include <SDL.h>
#endif

#include <base/math.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>
#include <engine/graphics.h>

#include "controls.h"


enum {	LEFT_JOYSTICK_X = 0, LEFT_JOYSTICK_Y = 1,
		RIGHT_JOYSTICK_X = 2, RIGHT_JOYSTICK_Y = 3,
		SECOND_RIGHT_JOYSTICK_X = 20, SECOND_RIGHT_JOYSTICK_Y = 21,
		ORIENTATION_X = 8, ORIENTATION_Y = 9, ORIENTATION_Z = 10,
		ACCELEROMETER_X = 0, ACCELEROMETER_Y = 1,
		NUM_JOYSTICK_AXES = 22 };


CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));

#if defined(__ANDROID__)
	SDL_Init(SDL_INIT_JOYSTICK);
	m_Joystick = SDL_JoystickOpen(0);
	if( m_Joystick && SDL_JoystickNumAxes(m_Joystick) < NUM_JOYSTICK_AXES )
	{
		SDL_JoystickClose(m_Joystick);
		m_Joystick = NULL;
	}

	m_Gamepad = SDL_JoystickOpen(2);

	m_Accelerometer = NULL;

	SDL_JoystickEventState(SDL_QUERY);

	m_UsingGamepad = false;
	if( getenv("OUYA") )
		m_UsingGamepad = true;
#endif
}

void CControls::OnReset()
{
	m_LastData.m_Direction = 0;
	m_LastData.m_Hook = 0;
	// simulate releasing the fire button
	if((m_LastData.m_Fire&1) != 0)
		m_LastData.m_Fire++;
	m_LastData.m_Fire &= INPUT_STATE_MASK;
	m_LastData.m_Jump = 0;
	m_InputData = m_LastData;

	m_InputDirectionLeft = 0;
	m_InputDirectionRight = 0;

	m_JoystickFirePressed = false;
	m_JoystickRunPressed = false;
	m_JoystickTapTime = 0;
	for( int i = 0; i < NUM_WEAPONS; i++ )
		m_AmmoCount[i] = 0;
	m_OldMouseX = m_OldMouseY = 0.0f;
	m_Hook = 0;
	m_RelaunchHook = false;
	// Prevent launching hook right after respawning
	if( g_Config.m_ClTouchscreenMode == TOUCHSCREEN_ACCELEROMETER )
		m_RelaunchHook = true;
#if defined(__ANDROID__)
	if(!m_Accelerometer)
		m_Accelerometer = SDL_JoystickOpen(1);
#endif
}

void CControls::OnRelease()
{
	OnReset();
}

void CControls::OnPlayerDeath()
{
	m_LastData.m_WantedWeapon = m_InputData.m_WantedWeapon = 0;
	for( int i = 0; i < NUM_WEAPONS; i++ )
		m_AmmoCount[i] = 0;

	// Prevent launching hook right after respawning
	if( g_Config.m_ClTouchscreenMode == TOUCHSCREEN_ACCELEROMETER )
		m_RelaunchHook = true;
}

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	int *v = (int *)pUserData;
	if(((*v)&1) != pResult->GetInteger(0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_pVariable;
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
		*pSet->m_pVariable = pSet->m_Value;
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet->m_pVariable);
	pSet->m_pControls->m_InputData.m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionLeft, "Move left");
	Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionRight, "Move right");
	Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Jump, "Jump");
	Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &m_Hook, "Hook");
	Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, &m_InputData.m_Fire, "Fire");

	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 1}; Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to hammer"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 2}; Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to gun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 3}; Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to shotgun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 4}; Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to grenade"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 5}; Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to rifle"); }

	{ static CInputSet s_Set = {this, &m_InputData.m_NextWeapon, 0}; Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to next weapon"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_PrevWeapon, 0}; Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to previous weapon"); }
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
		if(g_Config.m_ClAutoswitchWeapons)
			m_InputData.m_WantedWeapon = pMsg->m_Weapon+1;
		// We don't really know ammo count, until we'll switch to that weapon, but any non-zero count will suffice here
		m_AmmoCount[pMsg->m_Weapon%NUM_WEAPONS] = 10;
	}
}

int CControls::SnapInput(int *pData)
{
	static int64 LastSendTime = 0;
	bool Send = false;

	// update player state
	if(m_pClient->m_pChat->IsActive())
		m_InputData.m_PlayerFlags = PLAYERFLAG_CHATTING;
	else if(m_pClient->m_pMenus->IsActive())
		m_InputData.m_PlayerFlags = PLAYERFLAG_IN_MENU;
	else
		m_InputData.m_PlayerFlags = PLAYERFLAG_PLAYING;

	if(m_pClient->m_pScoreboard->Active())
		m_InputData.m_PlayerFlags |= PLAYERFLAG_SCOREBOARD;

	if(m_LastData.m_PlayerFlags != m_InputData.m_PlayerFlags)
		Send = true;

	m_LastData.m_PlayerFlags = m_InputData.m_PlayerFlags;

	// we freeze the input if chat or menu is activated
	if(!(m_InputData.m_PlayerFlags&PLAYERFLAG_PLAYING))
	{
		OnReset();

		mem_copy(pData, &m_InputData, sizeof(m_InputData));

		// send once a second just to be sure
		if(time_get() > LastSendTime + time_freq())
			Send = true;
	}
	else
	{

		m_InputData.m_TargetX = (int)m_MousePos.x;
		m_InputData.m_TargetY = (int)m_MousePos.y;
		if(!m_InputData.m_TargetX && !m_InputData.m_TargetY)
		{
			m_InputData.m_TargetX = 1;
			m_MousePos.x = 1;
		}

		// set direction
		m_InputData.m_Direction = 0;
		if(m_InputDirectionLeft && !m_InputDirectionRight)
			m_InputData.m_Direction = -1;
		if(!m_InputDirectionLeft && m_InputDirectionRight)
			m_InputData.m_Direction = 1;

		m_InputData.m_Hook = m_Hook && !m_RelaunchHook;

		// stress testing
		if(g_Config.m_DbgStress)
		{
			float t = Client()->LocalTime();
			mem_zero(&m_InputData, sizeof(m_InputData));

			m_InputData.m_Direction = ((int)t/2)&1;
			m_InputData.m_Jump = ((int)t);
			m_InputData.m_Fire = ((int)(t*10));
			m_InputData.m_Hook = ((int)(t*2))&1;
			m_InputData.m_WantedWeapon = ((int)t)%NUM_WEAPONS;
			m_InputData.m_TargetX = (int)(sinf(t*3)*100.0f);
			m_InputData.m_TargetY = (int)(cosf(t*3)*100.0f);
		}

		// check if we need to send input
		if(m_InputData.m_Direction != m_LastData.m_Direction) Send = true;
		else if(m_InputData.m_Jump != m_LastData.m_Jump) Send = true;
		else if(m_InputData.m_Fire != m_LastData.m_Fire) Send = true;
		else if(m_InputData.m_Hook != m_LastData.m_Hook) Send = true;
		else if(m_InputData.m_WantedWeapon != m_LastData.m_WantedWeapon) Send = true;
		else if(m_InputData.m_NextWeapon != m_LastData.m_NextWeapon) Send = true;
		else if(m_InputData.m_PrevWeapon != m_LastData.m_PrevWeapon) Send = true;

		// send at at least 10hz
		if(time_get() > LastSendTime + time_freq()/25)
			Send = true;
	}

	// copy and return size
	m_LastData = m_InputData;

	if(!Send)
		return 0;

	LastSendTime = time_get();
	mem_copy(pData, &m_InputData, sizeof(m_InputData));
	return sizeof(m_InputData);
}

void CControls::OnRender()
{
#if defined(__ANDROID__)
	int64 CurTime = time_get();
	bool FireWasPressed = false;

	if( m_Joystick && !m_UsingGamepad )
	{
		switch( g_Config.m_ClTouchscreenMode )
		{
			case TOUCHSCREEN_TWO_JOYSTICKS:
				TouchscreenInputTwoJoysticks(CurTime, &FireWasPressed);
				break;
			case TOUCHSCREEN_THREE_JOYSTICKS:
				TouchscreenInputThreeJoysticks(CurTime, &FireWasPressed);
				break;
			case TOUCHSCREEN_ACCELEROMETER:
				TouchscreenInputAccelerometer(CurTime, &FireWasPressed);
				break;
			case TOUCHSCREEN_DDRACE:
				TouchscreenInputDDRace(CurTime, &FireWasPressed);
				break;
			case TOUCHSCREEN_VOLUME_KEYS:
				TouchscreenInputVolumeKeys(CurTime, &FireWasPressed);
				break;
		}
	}

	if( m_Gamepad )
	{
		enum {
			GAMEPAD_DEAD_ZONE = 65536 / 8,
		};

		// Get input from left joystick
		int RunX = SDL_JoystickGetAxis(m_Gamepad, LEFT_JOYSTICK_X);
		int RunY = SDL_JoystickGetAxis(m_Gamepad, LEFT_JOYSTICK_Y);
		if( m_UsingGamepad )
		{
			//m_InputDirectionLeft = (RunX < -GAMEPAD_DEAD_ZONE);
			//m_InputDirectionRight = (RunX > GAMEPAD_DEAD_ZONE);
			static int OldRunX = 0, OldRunY = 0;
			if( RunX < -GAMEPAD_DEAD_ZONE && OldRunX >= -GAMEPAD_DEAD_ZONE )
				m_InputDirectionLeft = 1;
			if( RunX >= -GAMEPAD_DEAD_ZONE && OldRunX < -GAMEPAD_DEAD_ZONE )
				m_InputDirectionLeft = 0;
			if( RunX > GAMEPAD_DEAD_ZONE && OldRunX <= GAMEPAD_DEAD_ZONE )
				m_InputDirectionRight = 1;
			if( RunX <= GAMEPAD_DEAD_ZONE && OldRunX > GAMEPAD_DEAD_ZONE )
				m_InputDirectionRight = 0;
			OldRunX = RunX;
			OldRunY = RunY;
		}

		// Get input from right joystick
		int AimX = SDL_JoystickGetAxis(m_Gamepad, RIGHT_JOYSTICK_X);
		int AimY = SDL_JoystickGetAxis(m_Gamepad, RIGHT_JOYSTICK_Y);
		if( abs(AimX) > GAMEPAD_DEAD_ZONE || abs(AimY) > GAMEPAD_DEAD_ZONE )
		{
			m_MousePos = vec2(AimX / 30, AimY / 30);
			ClampMousePos();
		}

		if( !m_UsingGamepad && (abs(AimX) > GAMEPAD_DEAD_ZONE || abs(AimY) > GAMEPAD_DEAD_ZONE ||
			abs(RunX) > GAMEPAD_DEAD_ZONE || abs(RunY) > GAMEPAD_DEAD_ZONE) || SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT) )
		{
			UI()->AndroidShowScreenKeys(false);
			m_UsingGamepad = true;
		}
	}

	if( g_Config.m_ClAutoswitchWeaponsOutOfAmmo && m_pClient->m_Snap.m_pLocalCharacter )
	{
		// Keep track of ammo count, we know weapon ammo only when we switch to that weapon, this is tracked on server and protocol does not track that
		m_AmmoCount[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS] = m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount;
		// Autoswitch weapon if we're out of ammo
		if( (m_InputData.m_Fire % 2 != 0 || FireWasPressed) &&
			m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount == 0 &&
			m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_HAMMER &&
			m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_NINJA )
		{
			int w;
			for( w = WEAPON_RIFLE; w > WEAPON_GUN; w-- )
			{
				if( w == m_pClient->m_Snap.m_pLocalCharacter->m_Weapon )
					continue;
				if( m_AmmoCount[w] > 0 )
					break;
			}
			if( w != m_pClient->m_Snap.m_pLocalCharacter->m_Weapon )
				m_InputData.m_WantedWeapon = w+1;
		}
	}
#endif

	// update target pos
	if(m_pClient->m_Snap.m_pGameInfoObj && !m_pClient->m_Snap.m_SpecInfo.m_Active)
		m_TargetPos = m_pClient->m_LocalCharacterPos + m_MousePos;
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
		m_TargetPos = m_pClient->m_Snap.m_SpecInfo.m_Position + m_MousePos;
	else
		m_TargetPos = m_MousePos;
}

bool CControls::OnMouseMove(float x, float y)
{
	if((m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED) ||
		(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_pChat->IsActive()))
		return false;

#if defined(__ANDROID__) // No relative mouse on Android
	// We're using joystick on Android, mouse is disabled

	if( m_OldMouseX != x || m_OldMouseY != y )
	{
		m_OldMouseX = x;
		m_OldMouseY = y;
		m_MousePos = vec2((x - g_Config.m_GfxScreenWidth/2), (y - g_Config.m_GfxScreenHeight/2));
		ClampMousePos();
	}
#else
	m_MousePos += vec2(x, y); // TODO: ugly
	ClampMousePos();
#endif

	return true;
}

void CControls::ClampMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_MousePos.x = clamp(m_MousePos.x, 200.0f, Collision()->GetWidth()*32-200.0f);
		m_MousePos.y = clamp(m_MousePos.y, 200.0f, Collision()->GetHeight()*32-200.0f);
	}
	else
	{
		float CameraMaxDistance = 200.0f;
		float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
		float MouseMax = min(CameraMaxDistance/FollowFactor + g_Config.m_ClMouseDeadzone, (float)g_Config.m_ClMouseMaxDistance);

		if(length(m_MousePos) > MouseMax)
			m_MousePos = normalize(m_MousePos)*MouseMax;
	}
}

#if defined(__ANDROID__)
void CControls::TouchscreenInputTwoJoysticks(int64 CurTime, bool *FireWasPressed)
{
	// Get input from left joystick
	int RunX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
	int RunY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
	bool RunPressed = (RunX != 0 || RunY != 0);
	// Get input from right joystick
	int AimX = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_X);
	int AimY = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_Y);
	bool AimPressed = (AimX != 0 || AimY != 0);

	if( m_JoystickRunPressed != RunPressed )
	{
		if( RunPressed && RunY < 0 )
			m_InputData.m_Jump = 1;
		else
			m_InputData.m_Jump = 0;
		m_JoystickTapTime = CurTime;
	}

	m_JoystickRunPressed = RunPressed;

	if( RunPressed )
	{
		m_InputDirectionLeft = (RunX <= 0);
		m_InputDirectionRight = (RunX > 0);
	}

	// Move 500ms in the same direction, to prevent speed bump when tapping
	if( !RunPressed && m_JoystickTapTime + time_freq() / 2 > CurTime )
	{
		m_InputDirectionLeft = 0;
		m_InputDirectionRight = 0;
	}

	if( AimPressed )
	{
		m_MousePos = vec2(AimX / 30, AimY / 30);
		ClampMousePos();
	}

	m_RelaunchHook = false;
}

void CControls::TouchscreenInputThreeJoysticks(int64 CurTime, bool *FireWasPressed)
{
	// Get input from left joystick
	int RunX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
	int RunY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
	bool RunPressed = (RunX != 0 || RunY != 0);
	// Get input from right joystick
	int AimX = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_X);
	int AimY = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_Y);
	bool AimPressed = (AimX != 0 || AimY != 0);
	// Get input from another right joystick
	int HookX = SDL_JoystickGetAxis(m_Joystick, SECOND_RIGHT_JOYSTICK_X);
	int HookY = SDL_JoystickGetAxis(m_Joystick, SECOND_RIGHT_JOYSTICK_Y);
	bool HookPressed = (HookX != 0 || HookY != 0);

	if( m_JoystickRunPressed != RunPressed )
	{
		if( RunPressed && RunY < 0 )
			m_InputData.m_Jump = 1;
		else
			m_InputData.m_Jump = 0;
		m_JoystickTapTime = CurTime;
	}

	m_JoystickRunPressed = RunPressed;

	if( RunPressed )
	{
		m_InputDirectionLeft = (RunX <= 0);
		m_InputDirectionRight = (RunX > 0);
	}

	// Move 500ms in the same direction, to prevent speed bump when tapping
	if( !RunPressed && m_JoystickTapTime + time_freq() / 2 > CurTime )
	{
		m_InputDirectionLeft = 0;
		m_InputDirectionRight = 0;
	}

	if( HookPressed )
	{
		m_MousePos = vec2(HookX / 30, HookY / 30);
		ClampMousePos();
		m_Hook = 1;
	}
	else
	{
		m_Hook = 0;
	}

	if( AimPressed )
	{
		m_MousePos = vec2(AimX / 30, AimY / 30);
		ClampMousePos();
	}

	if( AimPressed != m_JoystickFirePressed )
	{
		// Fire when releasing joystick
		if( !AimPressed )
		{
			m_InputData.m_Fire ++;
			if( m_InputData.m_Fire % 2 != AimPressed )
				m_InputData.m_Fire ++;
			*FireWasPressed = true;
		}
	}

	m_JoystickFirePressed = AimPressed;
	m_RelaunchHook = false;
}

void CControls::TouchscreenInputAccelerometer(int64 CurTime, bool *FireWasPressed)
{
	int X = 0, Y = 0;
	static int OldX = 0;

	if( m_Accelerometer != NULL )
	{
		X = SDL_JoystickGetAxis(m_Accelerometer, ACCELEROMETER_X);
		Y = SDL_JoystickGetAxis(m_Accelerometer, ACCELEROMETER_Y);
	}

	if( X >= g_Config.m_ClAccelerometerSensitivity && OldX < g_Config.m_ClAccelerometerSensitivity )
		m_InputDirectionRight = 1;
	if( X < g_Config.m_ClAccelerometerSensitivity && OldX >= g_Config.m_ClAccelerometerSensitivity )
		m_InputDirectionRight = 0;
	if( X <= -g_Config.m_ClAccelerometerSensitivity && OldX > -g_Config.m_ClAccelerometerSensitivity )
		m_InputDirectionLeft = 1;
	if( X > -g_Config.m_ClAccelerometerSensitivity && OldX <= -g_Config.m_ClAccelerometerSensitivity )
		m_InputDirectionLeft = 0;

	OldX = X;

	// Get input from right joystick
	int AimX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
	int AimY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
	bool AimPressed = (AimX != 0 || AimY != 0);
	static int OldAimX = 0, OldAimY = 0;
	bool OldAimPressed = (OldAimX != 0 || OldAimY != 0);

	if( AimPressed )
	{
		m_MousePos = vec2(AimX / 30, AimY / 30);
		ClampMousePos();
		m_Hook = 0;
		if( !OldAimPressed )
			m_RelaunchHook = false;
	}
	else
	{
		if (m_InputData.m_Fire % 2 == 0)
			m_Hook = 1;
	}
	OldAimX = AimX;
	OldAimY = AimY;
}

void CControls::TouchscreenInputDDRace(int64 CurTime, bool *FireWasPressed)
{
	// Get input from left joystick
	int RunX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
	int RunY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
	bool RunPressed = (RunX != 0 || RunY != 0);
	// Get input from right joystick
	int AimX = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_X);
	int AimY = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_Y);
	bool AimPressed = (AimX != 0 || AimY != 0);

	if( m_JoystickRunPressed != RunPressed )
	{
		if( RunPressed && RunY < 0 )
			m_InputData.m_Jump = 1;
		else
			m_InputData.m_Jump = 0;
		m_JoystickTapTime = CurTime;
	}

	m_JoystickRunPressed = RunPressed;

	if( RunPressed )
	{
		m_InputDirectionLeft = (RunX <= 0);
		m_InputDirectionRight = (RunX > 0);
	}

	// Move 500ms in the same direction, to prevent speed bump when tapping
	if( !RunPressed && m_JoystickTapTime + time_freq() / 2 > CurTime )
	{
		m_InputDirectionLeft = 0;
		m_InputDirectionRight = 0;
	}

	if( AimPressed )
	{
		m_MousePos = vec2(AimX / 30, AimY / 30);
		ClampMousePos();
		m_Hook = 1;
	}
	else
	{
		m_Hook = 0;
	}

	m_RelaunchHook = false;
}

void CControls::TouchscreenInputVolumeKeys(int64 CurTime, bool *FireWasPressed)
{
	// Get input from left joystick
	int RunX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
	int RunY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
	bool RunPressed = (RunX != 0 || RunY != 0);
	// Get input from right joystick
	int AimX = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_X);
	int AimY = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_Y);
	bool AimPressed = (AimX != 0 || AimY != 0);

	if( m_JoystickRunPressed != RunPressed )
	{
		if( RunPressed && RunY < 0 )
			m_InputData.m_Jump = 1;
		else
			m_InputData.m_Jump = 0;
		m_JoystickTapTime = CurTime;
	}

	m_JoystickRunPressed = RunPressed;

	if( RunPressed )
	{
		m_InputDirectionLeft = (RunX <= 0);
		m_InputDirectionRight = (RunX > 0);
	}

	// Move 500ms in the same direction, to prevent speed bump when tapping
	if( !RunPressed && m_JoystickTapTime + time_freq() / 2 > CurTime )
	{
		m_InputDirectionLeft = 0;
		m_InputDirectionRight = 0;
	}

	if( AimPressed )
	{
		m_MousePos = vec2(AimX / 30, AimY / 30);
		ClampMousePos();
	}

	m_RelaunchHook = false;

	if( AimPressed != m_JoystickFirePressed )
	{
		// Fire when releasing joystick
		if( !AimPressed )
		{
			if( m_Hook )
			{
				m_RelaunchHook = true;
			}
			else
			{
				m_InputData.m_Fire ++;
				if( m_InputData.m_Fire % 2 != AimPressed )
					m_InputData.m_Fire ++;
				*FireWasPressed = true;
			}
		}
	}

	m_JoystickFirePressed = AimPressed;

	if( !m_Hook )
	{
		m_RelaunchHook = false;
	}
}

#endif
