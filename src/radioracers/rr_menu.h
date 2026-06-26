// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_menu.h
/// \brief RadioRacers Menu Options

#ifndef __RR_MENU__
#define __RR_MENU__

#include "../k_menu.h"
#include "../m_cond.h"
#include "../command.h"
#include "../console.h"

#include "rr_cvar.h"

#ifdef __cplusplus
extern "C" {
#endif

extern menuitem_t OPTIONS_RadioRacersMenu[];
extern menu_t OPTIONS_RadioRacersMenuDef;

extern menuitem_t OPTIONS_RadioRacersHud[];
extern menu_t OPTIONS_RadioRacersHudDef;

void RadioFunMenu_Init(void);
void RadioAccessibilityMenu_Init(void);
void RadioHudfeedMenu_Init(void);

// Bookmarks
void RRM_DrawCharacterBookmarks(void);
void RRM_BookmarkTick(void);
void RRM_BookmarkInit(void);
boolean RRM_BookmarkHandler(INT32 choice);
boolean RRM_BookmarkQuit(void);
void RRM_BookmarkSelect(INT32 choice);

typedef struct
{
    tic_t follower_tics;
    UINT8 follower_frame;
    state_t *follower_state;
} bookmark_menu_follower_anim_t;

typedef enum
{
    BMENU_STAGE_BROWSING = 0,
    BMENU_STAGE_BOOKMARKED,
    BMENU_STAGE_OVERWRITTEN,
    BMENU_STAGE_APPLIED_BOOKMARK
} bookmark_menu_stage_t;

extern struct bookmarkmenu_s
{
    bookmark_menu_follower_anim_t current_follower;
    bookmark_menu_follower_anim_t preview_follower;
    tic_t follower_timer;
    tic_t stage_timer;
    size_t current_page;
    boolean show_info;
	boolean selected;
    UINT8 stage;
} bookmarkmenu;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
