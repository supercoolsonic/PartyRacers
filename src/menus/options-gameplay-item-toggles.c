// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-gameplay-item-toggles.c
/// \brief Random Item Toggles

#include "../k_menu.h"
#include "../s_sound.h"

menuitem_t OPTIONS_GameplayItems[] =
{
	// Mostly handled by the drawing function.
	/*{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Ring",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Self-Propelled Bomb",	NULL, {.routine = M_HandleItemToggles}, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Hyudoro",				NULL, {.routine = M_HandleItemToggles}, KITEM_HYUDORO, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pickpocket Hyudoro",	NULL, {.routine = M_HandleItemToggles}, KITEM_PICKPOCKETHYU, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Butler Hyudoro",		NULL, {.routine = M_HandleItemToggles}, KITEM_BUTLERHYU, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker",				NULL, {.routine = M_HandleItemToggles}, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Rocket Sneaker",		NULL, {.routine = M_HandleItemToggles}, KITEM_ROCKETSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Stone Shoe",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_STONESHOE, 0},
		
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gachabom",				NULL, {.routine = M_HandleItemToggles}, KITEM_GACHABOM, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gachabom x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEGACHABOM, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut",				NULL, {.routine = M_HandleItemToggles}, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut x4",			NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADORBINAUT, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Banana",				NULL, {.routine = M_HandleItemToggles}, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Banana x3",				NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Deca-Banana",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DECABANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz",					NULL, {.routine = M_HandleItemToggles}, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz x2",				NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALJAWZ, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Proximity Mine",		NULL, {.routine = M_HandleItemToggles}, KITEM_MINE, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ballhog",				NULL, {.routine = M_HandleItemToggles}, KITEM_BALLHOG, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Toxomister",			NULL, {.routine = M_HandleItemToggles}, KITEM_TOXOMISTER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pressure Mine",			NULL, {.routine = M_HandleItemToggles}, KITEM_PRESSUREMINE, 0},	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Land Mine",				NULL, {.routine = M_HandleItemToggles}, KITEM_LANDMINE, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Invincibility",			NULL, {.routine = M_HandleItemToggles}, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Master Emerald",		NULL, {.routine = M_HandleItemToggles}, KITEM_MASTEREMERALD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Grow",					NULL, {.routine = M_HandleItemToggles}, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Shrink",				NULL, {.routine = M_HandleItemToggles}, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Time Stone",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_TIMESTONE, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pogo Spring",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_POGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Yellow Pogo Spring",	NULL, {.routine = M_HandleItemToggles}, KITEM_YOGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Blue Pogo Spring",		NULL, {.routine = M_HandleItemToggles}, KITEM_BOGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Drop Target",			NULL, {.routine = M_HandleItemToggles}, KITEM_DROPTARGET, sfx_s258},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gardentop",		 		NULL, {.routine = M_HandleItemToggles}, KITEM_GARDENTOP, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Normal Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_NORMALSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Lightning Shield",		NULL, {.routine = M_HandleItemToggles}, KITEM_LIGHTNINGSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bubble Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Flame Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Armageddon Shield",		NULL, {.routine = M_HandleItemToggles}, KITEM_ARMASHIELD, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Balkiry",				NULL, {.routine = M_HandleItemToggles}, KITEM_ABURNERJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Balkiry x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALABURNERJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Checker Wrecker",		NULL, {.routine = M_HandleItemToggles}, KITEM_WRECKINGBALL, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Mega Chopper",			NULL, {.routine = M_HandleItemToggles}, KITEM_MEGACHOPPER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Octus",					NULL, {.routine = M_HandleItemToggles}, KITEM_OCTUS, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Chameleon Blaster",		NULL, {.routine = M_HandleItemToggles}, KITEM_CHAMBLASTER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Egg Blaster",			NULL, {.routine = M_HandleItemToggles}, KITEM_EGGBLASTER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ring Gun",				NULL, {.routine = M_HandleItemToggles}, KITEM_RINGGUN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Eggmark",				NULL, {.routine = M_HandleItemToggles}, KITEM_EGGMAN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Kitchen Sink",			NULL, {.routine = M_HandleItemToggles}, KITEM_KITCHENSINK, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Jackpot",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERJACKPOT, 0},*/

//	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 0, 0}, -- next time, gadget

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Ring",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Self-Propelled Bomb",	NULL, {.routine = M_HandleItemToggles}, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Hyudoro",				NULL, {.routine = M_HandleItemToggles}, KITEM_HYUDORO, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Kitchen Sink",			NULL, {.routine = M_HandleItemToggles}, KITEM_KITCHENSINK, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker",				NULL, {.routine = M_HandleItemToggles}, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneaker x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Rocket Sneaker",		NULL, {.routine = M_HandleItemToggles}, KITEM_ROCKETSNEAKER, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Banana",				NULL, {.routine = M_HandleItemToggles}, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Banana x3",				NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Stone Shoe",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_STONESHOE, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gachabom",				NULL, {.routine = M_HandleItemToggles}, KITEM_GACHABOM, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut",				NULL, {.routine = M_HandleItemToggles}, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinaut x4",			NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gachabom x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEGACHABOM, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz",					NULL, {.routine = M_HandleItemToggles}, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz x2",				NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Proximity Mine",		NULL, {.routine = M_HandleItemToggles}, KITEM_MINE, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ballhog",				NULL, {.routine = M_HandleItemToggles}, KITEM_BALLHOG, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Toxomister",			NULL, {.routine = M_HandleItemToggles}, KITEM_TOXOMISTER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Drop Target",			NULL, {.routine = M_HandleItemToggles}, KITEM_DROPTARGET, sfx_s258},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Land Mine",				NULL, {.routine = M_HandleItemToggles}, KITEM_LANDMINE, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Eggmark",				NULL, {.routine = M_HandleItemToggles}, KITEM_EGGMAN, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Invincibility",			NULL, {.routine = M_HandleItemToggles}, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Grow",					NULL, {.routine = M_HandleItemToggles}, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Shrink",				NULL, {.routine = M_HandleItemToggles}, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Gardentop",		 		NULL, {.routine = M_HandleItemToggles}, KITEM_GARDENTOP, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Lightning Shield",		NULL, {.routine = M_HandleItemToggles}, KITEM_LIGHTNINGSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bubble Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Flame Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pogo Spring",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_POGOSPRING, 0},
	
};


menuitem_t OPTIONS_GameplayItems_Party[] =		//SCS ADD
{
		// Mostly handled by the drawing function.
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Deca-Banana",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DECABANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pressure Mine",			NULL, {.routine = M_HandleItemToggles}, KITEM_PRESSUREMINE, 0},	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pickpocket Hyudoro",	NULL, {.routine = M_HandleItemToggles}, KITEM_PICKPOCKETHYU, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Butler Hyudoro",		NULL, {.routine = M_HandleItemToggles}, KITEM_BUTLERHYU, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Master Emerald",		NULL, {.routine = M_HandleItemToggles}, KITEM_MASTEREMERALD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Yellow Pogo Spring",	NULL, {.routine = M_HandleItemToggles}, KITEM_YOGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Blue Pogo Spring",		NULL, {.routine = M_HandleItemToggles}, KITEM_BOGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Time Stone",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_TIMESTONE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Balkiry",				NULL, {.routine = M_HandleItemToggles}, KITEM_ABURNERJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Balkiry x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALABURNERJAWZ, 0},	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Normal Shield",			NULL, {.routine = M_HandleItemToggles}, KITEM_NORMALSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Armageddon Shield",		NULL, {.routine = M_HandleItemToggles}, KITEM_ARMASHIELD, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Chameleon Blaster",		NULL, {.routine = M_HandleItemToggles}, KITEM_CHAMBLASTER, 0},	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Checker Wrecker",		NULL, {.routine = M_HandleItemToggles}, KITEM_WRECKINGBALL, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Mega Chopper",			NULL, {.routine = M_HandleItemToggles}, KITEM_MEGACHOPPER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Octus",					NULL, {.routine = M_HandleItemToggles}, KITEM_OCTUS, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ring Gun",				NULL, {.routine = M_HandleItemToggles}, KITEM_RINGGUN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sol",					NULL, {.routine = M_HandleItemToggles}, KITEM_SOL, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sol x3",				NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESOL, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sol x4",				NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADSOL, 0},
	
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Egg Blaster",			NULL, {.routine = M_HandleItemToggles}, KITEM_EGGBLASTER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Icedus",				NULL, {.routine = M_HandleItemToggles}, KITEM_ICEDUS, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Icedus x3",				NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEICEDUS, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Icedus x4",				NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADICEDUS, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Slicer",				NULL, {.routine = M_HandleItemToggles}, KITEM_SLICER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Jackpot",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERJACKPOT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ring Bonus Box",		NULL, {.routine = M_HandleItemToggles}, KITEM_RINGCAPBONUS, 0},
	
};


static void init_routine(void)
{
	// Since this menu can be accessed from different
	// locations. (currentMenu has not changed yet.)
	
	if (currentMenu == &OPTIONS_GameplayItemsDef)	//SCS ADD - We should be heading to the party items menu
		OPTIONS_GameplayItemsDef_Party.prevMenu = currentMenu->prevMenu;	//Store the normal item toggle menu's previous menu in case we back out completely here
	
	else if (currentMenu != &OPTIONS_GameplayItemsDef_Party)		//SCS ADD - If we're going back to normal items menu from party menu, don't store the party menu as the previous menu!
		OPTIONS_GameplayItemsDef.prevMenu = currentMenu;
	
}

/*
static void init_routine(void)
{
	// Since this menu can be accessed from different
	// locations. (currentMenu has not changed yet.)
	OPTIONS_GameplayItemsDef.prevMenu = currentMenu;
}
*/

menu_t OPTIONS_GameplayItemsDef = {
	sizeof (OPTIONS_GameplayItems) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems,
	14, 32,
	SKINCOLOR_SCARLET, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawItemToggles,
	M_DrawOptionsCogs,
	M_OptionsTick,
	init_routine,
	NULL,
	NULL,
};

menu_t OPTIONS_GameplayItemsDef_Party = {							//SCS ADD
	sizeof (OPTIONS_GameplayItems_Party) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems_Party,
	14, 32,
	SKINCOLOR_SCARLET, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawItemToggles,
	M_DrawOptionsCogs,
	M_OptionsTick,
	init_routine,
	NULL,
	NULL,
};

boolean M_AnyItemsEnabled(void);
boolean M_AnyItemsEnabled(void)
{
	INT32 i;
	for (i = 0; i < NUMKARTRESULTS-1; i++)
	{
		if (cv_items[i].value)
			return true;
	}
	return false;
}

void ThunderDome_MenuSound(void);
void ThunderDome_MenuSound(void)
{
	if (!menuactive || (currentMenu != &OPTIONS_GameplayItemsDef && currentMenu != &OPTIONS_GameplayItemsDef_Party))	//SCS EDIT
		return;

	if (cv_thunderdome.value)
	{
		S_StartSoundAtVolume(NULL, sfx_slot02, 80);
	}
	else
	{
		S_StartSound(NULL, sfx_itrolf);
	}
}

void KartFrantic_MenuSound(void);
void KartFrantic_MenuSound(void)
{
	if (!menuactive || (currentMenu != &OPTIONS_GameplayItemsDef && currentMenu != &OPTIONS_GameplayItemsDef_Party))	//SCS EDIT
		return;

	S_StartSound(NULL, (cv_kartfrantic.value ? sfx_noooo2 : sfx_kc48));
}

void M_HandleItemToggles(INT32 choice)
{
	const INT32 width = 8, height = 4;
	//const INT32 width = 11, height = 5;						//SCS EDIT
	INT32 column = itemOn/height, row = itemOn%height;
	INT16 next;
	UINT8 i;
	boolean exitmenu = false;
	const UINT8 pid = 0;

	(void) choice;

	if (M_MenuExtraPressed(pid))
	{
		const boolean check = !M_AnyItemsEnabled();
		for (i = 0; i < NUMKARTRESULTS-1; i++)
		{
			if (cv_items[i].value != check)
				CV_SetValue(&cv_items[i], check);
		}
		// KartItem_OnChange will toggle thunderdome and play sound

		M_SetMenuDelay(pid);
	}

	else if (M_MenuButtonPressed(pid, MBT_R))
	{
		CV_AddValue(&cv_kartfrantic, 1);

		M_SetMenuDelay(pid);
	}
	
	else if (M_MenuButtonPressed(pid, MBT_L)) //SCS ADD
	{
		M_SetMenuDelay(pid);
		
		if (itemToggleGrid)	//If this is already positive, we're going back to normal item grid
			M_SetupNextMenu(&OPTIONS_GameplayItemsDef, false);
		else
			M_SetupNextMenu(&OPTIONS_GameplayItemsDef_Party, false);
			
		itemToggleGrid = !itemToggleGrid;	//Flip it
	}

	else if (menucmd[pid].dpad_lr > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column++;
		if (((column*height)+row) >= currentMenu->numitems)
			column = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_lr < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column--;
		if (column < 0)
			column = width-1;
		if (((column*height)+row) >= currentMenu->numitems)
			column--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row+1) % height;
		if (((column*height)+row) >= currentMenu->numitems)
			row = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row-1) % height;
		if (row < 0)
			row = height-1;
		if (((column*height)+row) >= currentMenu->numitems)
			row--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);
		if (currentMenu->menuitems[itemOn].mvar1 == 0)
		{
			//S_StartSound(NULL, sfx_s26d);
			if (!shitsfree)
			{
				shitsfree = TICRATE;
				S_StartSound(NULL, sfx_itfree);
			}
		}
		else
		{
			if (currentMenu->menuitems[itemOn].mvar2)
			{
				S_StartSound(NULL, currentMenu->menuitems[itemOn].mvar2);
			}
			else
			{
				S_StartSound(NULL, sfx_s1ba);
			}
			CV_AddValue(&cv_items[currentMenu->menuitems[itemOn].mvar1-1], 1);
			// KartItem_OnChange will toggle thunderdome and play sound
		}
	}

	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);
		exitmenu = true;
	}

	if (exitmenu)
	{
		itemToggleGrid = false; //SCS ADD
		
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}
