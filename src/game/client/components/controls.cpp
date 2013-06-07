/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <SDL.h>

#include <base/math.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>

#include "controls.h"

enum { LEFT_JOYSTICK_X = 0, LEFT_JOYSTICK_Y = 1, RIGHT_JOYSTICK_X = 2, RIGHT_JOYSTICK_Y = 3, NUM_JOYSTICK_AXES = 4 };

CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));

	SDL_Init(SDL_INIT_JOYSTICK);
	m_Joystick = SDL_JoystickOpen(0);
	if( m_Joystick && SDL_JoystickNumAxes(m_Joystick) < NUM_JOYSTICK_AXES )
	{
		SDL_JoystickClose(m_Joystick);
		m_Joystick = NULL;
	}

	m_Gamepad = SDL_JoystickOpen(2);

	SDL_JoystickEventState(SDL_QUERY);
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
	m_JoystickDoubleTap = false;
	m_JoystickSwipeJumpAccumUp = 0;
	m_JoystickSwipeJumpAccumDown = 0;
	m_JoystickSwipeJumpY = 0;
	m_JoystickSwipeJumpTime = 0;
	m_JoystickHookShot = false;
	for( int i = 0; i < NUM_WEAPONS; i++ )
		m_AmmoCount[i] = 0;
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
	Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Hook, "Hook");
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
	if( m_Joystick )
	{
		// Get input from left joystick
		int RunX = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_X);
		int RunY = SDL_JoystickGetAxis(m_Joystick, LEFT_JOYSTICK_Y);
		bool RunPressed = (RunX != 0 || RunY != 0);
		int64 CurTime = time_get();

		if( m_JoystickRunPressed != RunPressed )
		{
			if( RunPressed )
			{
				if( m_JoystickTapTime + time_freq() / 2 > CurTime ) // Half-second timeout
				{
					//if( !m_JoystickDoubleTap ) // Double-tapping doesn't really work out, so disabled for now
					{
						m_InputData.m_Hook = 1;
						m_MousePos = vec2(RunX / 100, RunY / 100);
						ClampMousePos();
						m_JoystickHookShot = true;
					}
					//m_JoystickDoubleTap = !m_JoystickDoubleTap;
				}
				//else
				//	m_JoystickDoubleTap = false;
				m_JoystickSwipeJumpY = RunY;
			}
			else
			{
				m_InputData.m_Hook = 0;
				m_JoystickSwipeJumpAccumUp = 0;
				m_JoystickSwipeJumpAccumDown = 0;
				m_JoystickSwipeJumpY = 0;
				//if( !m_pClient->m_Snap.m_pLocalCharacter ||
				//	m_pClient->m_Snap.m_pLocalCharacter->m_HookState == HOOK_IDLE ||
				//	m_pClient->m_Snap.m_pLocalCharacter->m_HookState == HOOK_RETRACTED )
				//	m_JoystickDoubleTap = false;
			}
			m_JoystickTapTime = m_JoystickSwipeJumpTime = CurTime;
		}

		m_JoystickRunPressed = RunPressed;

		if( RunPressed )
		{
			m_InputDirectionLeft = (RunX < -8192);
			m_InputDirectionRight = (RunX > 8192);
		}

		// Move 300ms in the same direction, to prevent speed bump when tapping
		if( !RunPressed && m_JoystickTapTime + time_freq() / 3 > CurTime )
		{
			m_InputDirectionLeft = 0;
			m_InputDirectionRight = 0;
		}

		// Swipe-jump
		enum { SWIPE_JUMP_DECAY = 65536, SWIPE_JUMP_THRESHOLD = 8192 }; // Decay full height of joystick per 1 second, threshold = 1/8 joystick height

		if( m_JoystickSwipeJumpAccumUp < 0 || m_JoystickSwipeJumpAccumDown < 0 )
			m_InputData.m_Jump = 0; // Cancel previous jump with joystick, but do not prevent from jumping with button

		int64 TimeDiff = CurTime - m_JoystickSwipeJumpTime;
		m_JoystickSwipeJumpTime += TimeDiff;

		if( m_JoystickSwipeJumpY > RunY )
			m_JoystickSwipeJumpAccumUp += (m_JoystickSwipeJumpY - RunY) * time_freq();
		if( RunY > m_JoystickSwipeJumpY )
			m_JoystickSwipeJumpAccumDown += (RunY - m_JoystickSwipeJumpY) * time_freq();

		m_JoystickSwipeJumpY = RunY;

		m_JoystickSwipeJumpAccumUp -= TimeDiff * SWIPE_JUMP_DECAY;
		m_JoystickSwipeJumpAccumDown -= TimeDiff * SWIPE_JUMP_DECAY;
		if( m_JoystickSwipeJumpAccumUp < 0 )
			m_JoystickSwipeJumpAccumUp += min(-m_JoystickSwipeJumpAccumUp, TimeDiff * SWIPE_JUMP_DECAY * 2);
		if( m_JoystickSwipeJumpAccumDown < 0 )
			m_JoystickSwipeJumpAccumDown += min(-m_JoystickSwipeJumpAccumDown, TimeDiff * SWIPE_JUMP_DECAY * 2);

		if( m_JoystickSwipeJumpAccumUp > SWIPE_JUMP_THRESHOLD * time_freq() )
		{
			if( m_InputData.m_Hook )
				m_InputData.m_Hook = 0; // Jump disables hook, you cannot do some tricks but it's arguably more comfortable
			else
				m_InputData.m_Jump = 1;
			m_JoystickSwipeJumpAccumUp = -SWIPE_JUMP_DECAY * time_freq() / 2;
		}
		if( m_JoystickSwipeJumpAccumDown > SWIPE_JUMP_THRESHOLD * time_freq() )
		{
			if( m_InputData.m_Hook )
				m_InputData.m_Hook = 0;
			else
				m_InputData.m_Jump = 1;
			m_JoystickSwipeJumpAccumDown = -SWIPE_JUMP_DECAY * time_freq() / 2;
		}

		if( m_JoystickHookShot )
		{
			// Wait until server acknowledges hook, before changing aiming angle back
			if( !m_pClient->m_Snap.m_pLocalPrevCharacter ||
				m_pClient->m_Snap.m_pLocalPrevCharacter->m_HookState != HOOK_IDLE )
				m_JoystickHookShot = false;
		}

		// Get input from right joystick
		if( !m_JoystickHookShot ) // Hook aiming angle comes from left joystick, do not mess it up
		{
			int AimX = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_X);
			int AimY = SDL_JoystickGetAxis(m_Joystick, RIGHT_JOYSTICK_Y);
			bool AimPressed = (AimX != 0 || AimY != 0);

			if( AimPressed )
			{
				m_MousePos = vec2(AimX / 30, AimY / 30);
				ClampMousePos();
			}

			if( AimPressed != m_JoystickFirePressed )
			{
				if( m_pClient->m_Snap.m_pLocalCharacter &&
					m_pClient->m_Snap.m_pLocalCharacter->m_Weapon == 4 )
				{
					// Rifle - fire when releasing joystick
					if( !AimPressed )
					{
						m_InputData.m_Fire ++;
						if( m_InputData.m_Fire % 2 != AimPressed )
							m_InputData.m_Fire ++;
					}
				}
				else
				{
					if( m_InputData.m_Fire % 2 != AimPressed )
						m_InputData.m_Fire ++;
				}
			}

			m_JoystickFirePressed = AimPressed;
		}
		else
		{
			if( m_InputData.m_Fire % 2 != 0 )
				m_InputData.m_Fire ++;
		}
	}

	if( m_Gamepad )
	{
		// Get input from left joystick
		int RunX = SDL_JoystickGetAxis(m_Gamepad, LEFT_JOYSTICK_X);
		int RunY = SDL_JoystickGetAxis(m_Gamepad, LEFT_JOYSTICK_Y);
		if( abs(RunX) > 8192 )
		{
			m_InputDirectionLeft = (RunX < -8192);
			m_InputDirectionRight = (RunX > 8192);
		}
		if( abs(RunY) > 8192 )
		{
			m_InputData.m_Jump = (RunY < -16384);
			m_InputData.m_Hook = (RunY > 16384);
		}

		// Get input from right joystick
		int AimX = SDL_JoystickGetAxis(m_Gamepad, RIGHT_JOYSTICK_X);
		int AimY = SDL_JoystickGetAxis(m_Gamepad, RIGHT_JOYSTICK_Y);
		if( abs(AimX) > 8192 || abs(AimY) > 8192 )
		{
			m_MousePos = vec2(AimX / 50, AimY / 50);
			ClampMousePos();
			UI()->AndroidShowScreenKeys(false);
		}
	}

	if( g_Config.m_ClAutoswitchWeaponsOutOfAmmo && m_pClient->m_Snap.m_pLocalCharacter )
	{
		// Keep track of ammo count, we know weapon ammo only when we switch to that weapon, this is tracked on server and protocol does not track that
		m_AmmoCount[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS] = m_pClient->m_Snap.m_pLocalCharacter->m_AmmoCount;
		// Autoswitch weapon if we're out of ammo
		if( m_InputData.m_Fire % 2 != 0 &&
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
	//unsigned w = g_Config.m_GfxScreenWidth;
	//unsigned h = g_Config.m_GfxScreenHeight;
	//m_MousePos = vec2((x - w/2) / w * 2000.0f, (y - h/2) / h * 2000.0f);
	//m_MousePos = vec2(x, y);
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
