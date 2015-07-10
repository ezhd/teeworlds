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

#include "menus.h"


void CMenus::ServerCreatorInit()
{
}

void CMenus::ServerCreatorProcess(CUIRect MainView)
{
	CUIRect ServerList, ToolBox, StatusBox, TabBar;

	// background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);

	// create server list, status box, tab bar and tool box area
	MainView.VSplitRight(205.0f, &ServerList, &ToolBox);
	ServerList.HSplitBottom(70.0f, &ServerList, &StatusBox);
	StatusBox.VSplitRight(100.0f, &StatusBox, &TabBar);
	ServerList.VSplitRight(5.0f, &ServerList, 0);
}
