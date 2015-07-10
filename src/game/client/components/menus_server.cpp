/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>

#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>

#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

#include <game/client/ui.h>

#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include "menus.h"

#include <stdlib.h>
#include <SDL/SDL_android.h>
#include <sys/types.h>
#include <sys/wait.h>


void CMenus::ServerCreatorInit()
{
}

void CMenus::ServerCreatorProcess(CUIRect MainView)
{
	// background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 20.0f);
	MainView.Margin(10.0f, &MainView);

	MainView.HSplitTop(10, 0, &MainView);
	CUIRect MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, Localize("Local server"), 20.0f, 0);

	static int64 LastUpdateTime = 0;
	static bool ServerRunning = false;
	if (time_get() / time_freq() > LastUpdateTime + 3)
	{
		LastUpdateTime = time_get() / time_freq();
		ServerRunning = WEXITSTATUS(system("$SECURE_STORAGE_DIR/busybox sh -c 'ps | grep teeworlds_srv'")) == 0;
	}

	MainView.HSplitTop(30, 0, &MainView);
	MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, ServerRunning ? Localize("Server is running") : Localize("Server is stopped"), 20.0f, 0);

	MainView.HSplitTop(30, 0, &MainView);

	CUIRect Button;

	MainView.VSplitLeft(50, 0, &Button);
	Button.h = 50;
	Button.w = 300;
	static int s_StartServerButton = 0;
	if(DoButton_Menu(&s_StartServerButton, Localize("Start server"), 0, &Button))
	{
		//system("logwrapper $SECURE_STORAGE_DIR/busybox sh -c \"$SECURE_STORAGE_DIR/teeworlds_srv -f $UNSECURE_STORAGE_DIR/server-config.txt &\"");
		system("$SECURE_STORAGE_DIR/teeworlds_srv -f $UNSECURE_STORAGE_DIR/server-config.txt &");
		LastUpdateTime = 0;
	}

	MainView.VSplitRight(350, 0, &Button);
	Button.h = 50;
	Button.w = 300;
	static int s_StopServerButton = 0;
	if(DoButton_Menu(&s_StopServerButton, Localize("Stop server"), 0, &Button))
	{
		system("$SECURE_STORAGE_DIR/busybox killall teeworlds_srv");
		LastUpdateTime = time_get() / time_freq() - 2;
	}

	MainView.HSplitTop(60, 0, &MainView);

	MainView.VSplitLeft(50, 0, &Button);
	Button.h = 50;
	Button.w = 300;
	static int s_EditServerConfigButton = 0;
	if(DoButton_Menu(&s_EditServerConfigButton, Localize("Edit server config"), 0, &Button))
	{
		system("am start --user -3 -a android.intent.action.VIEW -t 'text/*' -d file://$UNSECURE_STORAGE_DIR/server-config.txt");
	}

	MainView.VSplitRight(350, 0, &Button);
	Button.h = 50;
	Button.w = 300;
	static int s_JoinServerButton = 0;
	if(DoButton_Menu(&s_JoinServerButton, Localize("Join server"), 0, &Button))
	{
		strcpy(g_Config.m_UiServerAddress, "127.0.0.1");
		Client()->Connect(g_Config.m_UiServerAddress);
	}

	MainView.HSplitTop(60, 0, &MainView);
	MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, Localize("If you don't have Wi-Fi connection, go to Android Settings,"), 20.0f, 0);
	MainView.HSplitTop(30, 0, &MainView);
	MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, Localize("and in Tethering & Hotspot menu enable Wi-Fi hotspot,"), 20.0f, 0);
	MainView.HSplitTop(30, 0, &MainView);
	MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, Localize("then ask other players to connect to Wi-Fi network you created,"), 20.0f, 0);
	MainView.HSplitTop(30, 0, &MainView);
	MsgBox = MainView;
	UI()->DoLabelScaled(&MsgBox, Localize("open TeeWorlds, and connect to your server from LAN menu."), 20.0f, 0);

	MainView.HSplitTop(30, 0, &MainView);
	MainView.VMargin(50.0f, &Button);
	Button.h = 50;
	static int s_ShareAppButton = 0;
	if(DoButton_Menu(&s_ShareAppButton, Localize("Share TeeWorlds app over Bluetooth to other devices"), 0, &Button))
	{
		system("$SECURE_STORAGE_DIR/busybox cp -f $ANDROID_MY_OWN_APP_FILE $UNSECURE_STORAGE_DIR/Teeworlds.apk");
		system("am start --user -3 -a android.intent.action.SEND -t application/vnd.android.package-archive "
				"--eu android.intent.extra.STREAM file://$UNSECURE_STORAGE_DIR/Teeworlds.apk");
	}

}
