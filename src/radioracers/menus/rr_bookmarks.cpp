// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2026 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/menu/rr_bookmarks.cpp
/// \brief Character bookmarks

#include "../rr_menu.h"
#include "../rr_util.h"
#include "../rr_setup.h"
#include "../rr_bookmarks.h"
#include "../../k_menu.h"
#include "../../r_skins.h"
#include "../../st_stuff.h"
#include "../../k_color.h"
#include "../../k_hud.h"
#include "../../r_draw.h"
#include "../../p_local.h"
#include "../../r_state.h"
#include "../../z_zone.h"
#include "../../s_sound.h"
#include "../../v_video.h"
#include "../../command.h"

#define ROWS 7
#define COLUMNS 3

struct bookmarkmenu_s bookmarkmenu;
std::vector<characterbookmarkparent_t> char_bookmarks;
UINT8 bookmark_page;
INT16 bookmark_idx;

enum preview_debug_message_type_e {
	PREVIEW_ERROR = 0,
	PREVIEW_WARNING = 1,
};

struct preview_debug_message_s
{
    preview_debug_message_type_e type;
    std::string message;
};

static characterbookmarkparent_t M_BuildBookmarkForPlayer(player_t* p)
{
    const bool follower_present = !((p->followerskin < 0 || p->followerskin >= numfollowers));

    characterbookmark_t new_bookmark_child = {
        p->skin,
        p->skincolor,
        p->followerskin,
        p->followercolor,
    };

    characterbookmarkparent_t new_bookmark = {
        true,
        true,
        true,
        true,
        follower_present,
        follower_present,
        follower_present,
        follower_present,
        new_bookmark_child
    };

    strlcpy(new_bookmark.skin_name, skins[p->skin]->name, SKINNAMESIZE+1);
    strlcpy(new_bookmark.skincolor_name, skincolors[p->skincolor].name, MAXCOLORNAME+1);

    if (follower_present) {
        strlcpy(new_bookmark.follower_name, followers[p->followerskin].name, SKINNAMESIZE+1);

        UINT16 actual_follower_color = K_GetEffectiveFollowerColor(
			p->followercolor,
			&followers[p->followerskin],
			p->skincolor,
			skins[p->skin]
		);
        strlcpy(new_bookmark.followercolor_name, skincolors[actual_follower_color].name, MAXCOLORNAME+1);
    }
    return new_bookmark;
}

/**
COMMANDS
*/
static void Command_SaveCharacterBookmark(void)
{
    // At the very least, be in the game
    if (!Playing())
        return;

    // ..What the? No!
    if (demo.playback)
        return;

    if (char_bookmarks.size() + 1 > MAX_BOOKMARKS) {
        CONS_Alert(CONS_NOTICE, M_GetText("Your bookmarks are full. Stop being greedy!\n"));
        return;
    }
    
    // Never supporting splitscreen
    player_t* p = &players[g_localplayers[0]];
    if (p == NULL)
        return;

    characterbookmarkparent_t new_bookmark = M_BuildBookmarkForPlayer(p);

    char_bookmarks.emplace_back(new_bookmark);
}

void RR_InitBookmarkCommands(void) {
    COM_AddCommand("bookmark", Command_SaveCharacterBookmark);
}

/**
MISC
**/

static bool M_AreBookmarksFull(void)
{
    return char_bookmarks.size() == MAX_BOOKMARKS;
}

static size_t M_GetMaxPagesForBookmarkMenu() {
    size_t bookmarks_size = (char_bookmarks.size() + 1 < MAX_BOOKMARKS) ? char_bookmarks.size() + 1 : char_bookmarks.size();
    return std::ceil(
        static_cast<float>(bookmarks_size) / static_cast<float>((ROWS * COLUMNS))
    );
}

static void M_NextPage(void)
{
    if (bookmarkmenu.current_page+1 > M_GetMaxPagesForBookmarkMenu()-1) {
        bookmarkmenu.current_page = 0;
    } else {
        bookmarkmenu.current_page++;
    }
}

static void M_DrawPreviewSilhouette(INT16 x, INT16 y)
{
    M_DrawCharacterSprite(
        x+40,
        y+78,
        0,
        SPR2_STIN,
        7,
        0,
        0,
        R_GetTranslationColormap(TC_BLINK, SKINCOLOR_GREY, GTC_CACHE)
    );
}

static UINT8* M_GetWarningColormap(void)
{
    return R_GetTranslationColormap(
         TC_DEFAULT, 
        SKINCOLOR_YELLOW, 
        GTC_CACHE
    );
}

static UINT8* M_GetErrorColormap(void)
{
    return R_GetTranslationColormap(
        TC_DEFAULT, 
        SKINCOLOR_RED, 
        GTC_CACHE
    );
}

static UINT8* M_GetBookmarkSkinColormap(characterbookmarkparent_t* bookmark_parent) {
    skincolornum_t charcolornum = SKINCOLOR_NONE;

    if (bookmark_parent->skincolor_present && bookmark_parent->skincolor_usable) {
        charcolornum = static_cast<skincolornum_t>(bookmark_parent->bookmark.skincolor);
    }  else {
        charcolornum = static_cast<skincolornum_t>(skins[bookmark_parent->bookmark.skin]->prefcolor);
    }

    return R_GetTranslationColormap(
        bookmark_parent->bookmark.skin, 
        charcolornum, 
        GTC_CACHE
    );
}

static UINT8* M_GetBookmarkFollowerColormap(characterbookmarkparent_t* bookmark_parent) {
    skincolornum_t followercolornum = followers[bookmark_parent->bookmark.follower].defaultcolor;

    if (bookmark_parent->followercolor_present && bookmark_parent->followercolor_usable) {
        followercolornum = static_cast<skincolornum_t>(bookmark_parent->bookmark.followercolor);
    }

    return R_GetTranslationColormap(
        TC_DEFAULT, 
        followercolornum, 
        GTC_MENUCACHE
    );
}

static size_t M_GetPageOffset(void)
{
    return bookmarkmenu.current_page * (ROWS*COLUMNS);
}

static characterbookmarkparent_t* FetchBookmarkParent(INT16 index)
{
    if (char_bookmarks.empty())
        return NULL;

    if (index >= 0 && index < char_bookmarks.size())
        return &char_bookmarks.at(index);
    
    return NULL;
}

static characterbookmarkparent_t* FetchSelectedBookmarkParent(void)
{
    return FetchBookmarkParent(M_GetPageOffset() + bookmark_idx);
}

static bool IsBookmarkUnusable(characterbookmarkparent_t* parent)
{
    if (!parent)
        return true;

    bool unusable = !parent->valid || !parent->skin_usable;
    if (!unusable && parent->follower_present) {
        unusable = !parent->follower_usable;
    }
    return unusable;
}

static bool IsBookmarkSkinIncomplete(characterbookmarkparent_t* parent)
{
    return !parent->skincolor_present || (parent->skincolor_present && !parent->skincolor_usable);
}

static bool IsBookmarkFollowerIncomplete(characterbookmarkparent_t* parent)
{
    return !parent->followercolor_present || (parent->followercolor_present && !parent->followercolor_usable);
}

static bool IsBookmarkIncomplete(characterbookmarkparent_t *parent)
{
    bool incomplete = IsBookmarkSkinIncomplete(parent);
    if (parent->follower_present && parent->follower_usable) {
        incomplete = IsBookmarkFollowerIncomplete(parent);
    }
    return incomplete;
}

static boolean IsFollowerValid(INT32 followernum)
{
    return !(followernum < 0 || followernum >= numfollowers);
}

static bool VerifyBookmarkSkin(INT32 *skin, const char* name)
{
    *skin = R_SkinAvailableEx(name, false);
    return *skin > -1 && R_SkinUsable(g_localplayers[0], *skin, false);
}

static bool VerifyBookmarkSkinColor(UINT16 *color, const char* name)
{
    *color = get_skincolornum(name);
    return K_ColorUsable(static_cast<skincolornum_t>(*color), false, true);
}

static bool VerifyBookmarkFollowerSkin(INT32 *follower_skin, const char* name)
{
    *follower_skin = K_FollowerAvailable(name);
    return *follower_skin > -1 && K_FollowerUsable(*follower_skin);
}

static bool VerifyBookmarkFollowerSkinColor(UINT16 *color, const char* name)
{
    *color = get_skincolornum(name);
    return K_ColorUsable(static_cast<skincolornum_t>(*color), true, true);
}

static void VerifySingleBookmark(characterbookmarkparent_t *b)
{
    characterbookmark_t *child = &b->bookmark;

    b->skin_usable = VerifyBookmarkSkin(&child->skin, b->skin_name);

    if (b->skincolor_present)
        b->skincolor_usable = VerifyBookmarkSkinColor(&child->skincolor, b->skincolor_name);

    if (b->follower_present) {
        b->follower_usable = VerifyBookmarkFollowerSkin(&child->follower, b->follower_name);
        if (b->followercolor_present)
            b->followercolor_usable = VerifyBookmarkFollowerSkinColor(&child->followercolor, b->followercolor_name);
    }
}

void RR_VerifyBookmarks(void)
{
    for (auto& bookmark_parent: char_bookmarks) {
        if (!bookmark_parent.valid)
            continue;

        VerifySingleBookmark(&bookmark_parent);
    }
}

/**
MENUS
**/
menuitem_t PLAY_CharacterBookmarks[] = 
{
    {IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

menu_t BOOKMARKS_MainDef = {
    sizeof (PLAY_CharacterBookmarks) / sizeof (menuitem_t),
    &MainDef,
    0,
    PLAY_CharacterBookmarks,
    0, 0,
    SKINCOLOR_GOLD, 0,
    0,
    NULL,
    0, 0,
    RRM_DrawCharacterBookmarks,
    NULL,
    RRM_BookmarkTick,
    RRM_BookmarkInit,
    RRM_BookmarkQuit,
    RRM_BookmarkHandler
};

/**
MENU HANDLERS
**/
void RRM_BookmarkSelect(INT32 choice) {
    (void)choice;
    
    BOOKMARKS_MainDef.prevMenu = currentMenu;
    M_SetupNextMenu(&BOOKMARKS_MainDef, true);
}

static bool M_HandleBookmarkOverwrite()
{
    player_t* p = &players[g_localplayers[0]];
    if (p == NULL) {
        // When would this even happen?
        CONS_Printf("Failed to overwrite bookmark!");
        return false;
    }
    characterbookmarkparent_t new_bookmark = M_BuildBookmarkForPlayer(p);
    char_bookmarks[M_GetPageOffset() + bookmark_idx] = new_bookmark;

    return true;
}

static void M_HandleBookmarkSave(void)
{
    if (bookmarkmenu.stage != BMENU_STAGE_BROWSING)
        return;

    // The console command already checks for this, but something user-friendly is better
    if (M_AreBookmarksFull()) {
        S_StartSound(NULL, sfx_s3k7b);
        return;
    }

    if (FetchSelectedBookmarkParent() == NULL) {
        COM_BufInsertText("bookmark");
        RR_VerifyBookmarks();
        bookmarkmenu.stage = BMENU_STAGE_BOOKMARKED;

        size_t global_idx = char_bookmarks.size() - 1;
        bookmarkmenu.current_page = global_idx / (ROWS * COLUMNS);
        bookmark_idx = global_idx % (ROWS * COLUMNS);
    } else {
        if (M_HandleBookmarkOverwrite()) {
            RR_VerifyBookmarks();
            bookmarkmenu.stage = BMENU_STAGE_OVERWRITTEN;
        }
    }

    player_t* p = &players[g_localplayers[0]];
    if (p != NULL && p->skin != -1) {
        S_StartSound(NULL, skins[p->skin]->soundsid[S_sfx[sfx_kattk1].skinsound]);
    }
    S_StartSound(NULL, sfx_tmxunx);
}

static void M_HandleBookmarkDelete(void)
{
    if (bookmarkmenu.stage != BMENU_STAGE_BROWSING)
        return;

    characterbookmarkparent_t *bookmark_parent = FetchSelectedBookmarkParent();
    if (bookmark_parent == NULL)
        return;

    /**
        bookmark_idx is page local (0 to ROWS*COLUMNS-1),
        so need to offset it by the page offset to make
        sure the right element is being deleted
     */ 
    size_t global_bookmark_idx = M_GetPageOffset() + bookmark_idx;
    char_bookmarks.erase(char_bookmarks.begin() + global_bookmark_idx);

    if (global_bookmark_idx >= char_bookmarks.size())
        global_bookmark_idx = (char_bookmarks.size() > 0) ? char_bookmarks.size() - 1 : 0;

    bookmarkmenu.current_page = global_bookmark_idx / (ROWS * COLUMNS);
    bookmark_idx = global_bookmark_idx % (ROWS * COLUMNS);

    S_StartSound(NULL, sfx_s3k7b); 
    RR_VerifyBookmarks();
}

static void M_BookmarkApply(void)
{
    characterbookmarkparent_t *bookmark_parent = FetchSelectedBookmarkParent();

    characterbookmark_t *bookmark = &bookmark_parent->bookmark;
    
    // Apply the CV vars
        
    // Skin color
    skincolornum_t charcolornum = SKINCOLOR_GREEN;
    
    if (bookmark_parent->skincolor_present && bookmark_parent->skincolor_usable) {
        charcolornum = static_cast<skincolornum_t>(bookmark->skincolor);
    } else {
        charcolornum = static_cast<skincolornum_t>(skins[bookmark->skin]->prefcolor);
    }
    CV_StealthSetValue(&cv_playercolor[0], static_cast<INT16>(charcolornum));
    
    // Follower
    skincolornum_t followercolornum = SKINCOLOR_GREEN;

    if (bookmark_parent->follower_present && bookmark_parent->follower_usable) {
        CV_StealthSet(&cv_follower[0], 
            followers[bookmark->follower].name);
        
        // Check the follower color in here
        if (bookmark_parent->followercolor_present && bookmark_parent->followercolor_usable) {
            followercolornum = static_cast<skincolornum_t>(bookmark_parent->bookmark.followercolor);
        } else {
            followercolornum = followers[bookmark_parent->bookmark.follower].defaultcolor;
        }
    } else {
        CV_StealthSet(&cv_follower[0], "None");
    }

    // Skin
    CV_StealthSet(&cv_skin[0], skins[bookmark->skin]->name);

    // Follower Color
    CV_SetValue(&cv_followercolor[0], static_cast<INT16>(followercolornum));

    bookmarkmenu.stage = BMENU_STAGE_APPLIED_BOOKMARK;

    S_StartSound(NULL, skins[bookmark->skin]->soundsid[S_sfx[sfx_kattk2].skinsound]);
    S_StartSound(NULL, sfx_s3k63);
	
    // Handled in the tick handler, see M_BookmarkApply
    bookmarkmenu.selected = false;
}

static void M_HandleBookmarkApply(void)
{
    if (bookmarkmenu.stage != BMENU_STAGE_BROWSING)
        return;

    if (IsBookmarkUnusable(FetchSelectedBookmarkParent())) {
        S_StartSound(NULL, sfx_s3k7b); 
        return;
    }

    bookmarkmenu.selected = true;
}

boolean RRM_BookmarkHandler(INT32 choice) {
    (void)choice;

    const UINT8 pid = 0;

    size_t current_row = bookmark_idx / COLUMNS;
    size_t current_col = bookmark_idx % COLUMNS;
    bool changed_bookmark = false;
    bool changed_direction = false;

    if (menucmd[pid].dpad_lr > 0) // Right
    {
        bookmark_idx = (current_col + 1) % COLUMNS + current_row * COLUMNS;
        changed_bookmark = true;
        changed_direction = true;

        S_StartSound(NULL, sfx_s3k5b);
        M_SetMenuDelay(pid);
    }
    else if(menucmd[pid].dpad_lr < 0) // Left
    {
        bookmark_idx = (current_col + (COLUMNS-1)) % COLUMNS + current_row * COLUMNS;
        changed_bookmark = true;
        changed_direction = true;
        
        S_StartSound(NULL, sfx_s3k5b);
        M_SetMenuDelay(pid);
    }
    else if (menucmd[pid].dpad_ud > 0) // Down
	{
        bookmark_idx = current_col + ((current_row + 1) % ROWS) * COLUMNS;
        changed_bookmark = true;
        changed_direction = true;

        M_SetMenuDelay(pid);
	}
    else if (menucmd[pid].dpad_ud < 0) // Up
	{
        bookmark_idx = current_col + ((current_row + (ROWS-1)) % ROWS) * COLUMNS;
        changed_bookmark = true;
        changed_direction = true;

        M_SetMenuDelay(pid);
	}
    else if (M_MenuButtonPressed(pid, MBT_L))
    {
        M_NextPage();
        M_SetMenuDelay(pid);
        changed_bookmark = true;
        changed_direction = true;
    }
    else if (M_MenuButtonPressed(pid, MBT_Y))
    {
        M_HandleBookmarkSave();
        M_SetMenuDelay(pid);
        changed_bookmark = true;
    }
    else if (M_MenuExtraPressed(pid)) {
        M_HandleBookmarkDelete();
        M_SetMenuDelay(pid);
        changed_bookmark = true;
    }
    else if (M_MenuButtonPressed(pid, MBT_R)) {
        bookmarkmenu.show_info ^= true;
        M_SetMenuDelay(pid);
    } else if (M_MenuConfirmPressed(pid)) {
        M_HandleBookmarkApply();
        M_SetMenuDelay(pid);
    }

    if (changed_bookmark)
        memset(&bookmarkmenu.preview_follower, 0, sizeof(bookmark_menu_follower_anim_t));

    // Prematurely end the stage if the player moves the cursor
    if (changed_direction) {
        if (bookmarkmenu.stage != BMENU_STAGE_BROWSING) {
            bookmarkmenu.stage_timer = 0;
            bookmarkmenu.stage = BMENU_STAGE_BROWSING;
        }
    }

    return false;
}

boolean RRM_BookmarkQuit(void)
{
	return true;
}

void RRM_BookmarkInit(void) {
    memset(&bookmarkmenu, 0, sizeof(bookmarkmenu));

    bookmark_page = 0;
    bookmark_idx = 0;

    /**
     * Ideally, this would be done on first boot.
     * But considering unlockables and modded characters...
     * Just do it before the menu is loaded.
     */
    RR_VerifyBookmarks();
}

// Uplifted from play-char-select.c
static void AnimateFollowerForMenu(INT32 followerskin, bookmark_menu_follower_anim_t *menu) {
    if (!(followerskin < 0 || followerskin >= numfollowers)) {

        if (menu->follower_state == NULL) {
            menu->follower_state = &states[followers[followerskin].followstate];

            if (menu->follower_state->frame & FF_ANIMATE)
                menu->follower_tics = menu->follower_state->var2;	// support for FF_ANIMATE
            else
                menu->follower_tics = menu->follower_state->tics;

            menu->follower_frame = menu->follower_state->frame & FF_FRAMEMASK;
        }

        if (--menu->follower_tics <= 0) {
            // FF_ANIMATE; cycle through FRAMES and get back afterwards. This will be prominent amongst followers hence why it's being supported here.
            if (menu->follower_state->frame & FF_ANIMATE)
            {
                menu->follower_frame++;
                menu->follower_tics = menu->follower_state->var2;
                if (menu->follower_frame > (menu->follower_state->frame & FF_FRAMEMASK) + menu->follower_state->var1)	// that's how it works, right?
                    menu->follower_frame = menu->follower_state->frame & FF_FRAMEMASK;
            }
            else
            {
                if (menu->follower_state->nextstate != S_NULL)
                    menu->follower_state = &states[menu->follower_state->nextstate];
                menu->follower_tics = menu->follower_state->tics;
                menu->follower_frame = menu->follower_state->frame & FF_FRAMEMASK;
            }
        }
    }
}

void RRM_BookmarkTick(void) {
    bookmarkmenu.follower_timer++;

    player_t* p = &players[g_localplayers[0]];
    
    AnimateFollowerForMenu(p->followerskin, &bookmarkmenu.current_follower);

    characterbookmarkparent_t *selected_bookmark = FetchSelectedBookmarkParent();
    if (selected_bookmark != NULL) {
        if (selected_bookmark->follower_present && selected_bookmark->follower_usable) 
        {
            AnimateFollowerForMenu(
        selected_bookmark->bookmark.follower, &bookmarkmenu.preview_follower);
        }
    }
	
    // Player has applied a bookmark
    if (bookmarkmenu.selected) {
        M_BookmarkApply();
    }

    if (bookmarkmenu.stage != BMENU_STAGE_BROWSING) {
        if (bookmarkmenu.stage_timer == 0) {
            bookmarkmenu.stage_timer = TICRATE-5;
        } else {
            bookmarkmenu.stage_timer--;
            if (bookmarkmenu.stage_timer == 0)
                bookmarkmenu.stage = BMENU_STAGE_BROWSING;
        }
    }
}

/**
HUD DRAWING
**/

// Uplifted from k_menudraw.c
static boolean M_DrawFollowerSprite(
    INT16 x, 
    INT16 y, 
    INT32 num, 
    UINT8 *colormap, 
    INT32 addflags,
    bookmark_menu_follower_anim_t *follower_anim
)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	INT32 followernum;
	state_t *usestate;
	UINT32 useframe;
	follower_t *fl;
	UINT8 rotation = 7;

    followernum = num;

	// Don't draw if we're outta bounds.
	if (followernum < 0 || followernum >= numfollowers)
		return false;

	fl = &followers[followernum];

	// if (p != NULL)
	{
		usestate = follower_anim->follower_state;
		useframe = follower_anim->follower_frame;
	}
	// else
	// {
	// 	usestate = &states[followers[followernum].followstate];
	// 	useframe = usestate->frame & FF_FRAMEMASK;
	// }

    if (usestate == NULL) {
        return true;
    }

	sprdef = &sprites[usestate->sprite];

	// draw the follower

	if (useframe >= sprdef->numframes)
		useframe = 0;	// frame doesn't exist, we went beyond it... what?

	sprframe = &sprdef->spriteframes[useframe];
	patch = static_cast<patch_t *>(W_CachePatchNum(sprframe->lumppat[rotation], PU_CACHE));

    if (sprframe->flip & (1<<rotation)) // Only for first sprite
	{
		addflags ^= V_FLIP; // This sprite is left/right flipped!
	}

	fixed_t sine = 0;

	{
		sine = FixedMul(fl->bobamp, FINESINE(((FixedMul(4 * M_TAU_FIXED, fl->bobspeed) * bookmarkmenu.follower_timer)>>ANGLETOFINESHIFT) & FINEMASK));
	}

	V_DrawFixedPatch((x*FRACUNIT), ((y-12)*FRACUNIT) + sine, fl->scale, addflags, patch, colormap);

	return true;
}

static void M_DrawBackgroundForCharAndFollower(INT16 x, INT16 y)
{
    if (!bookmarkmenu.show_info)
        return;
    
    V_DrawFill(
        x+2,
        y-1,
        75,
        21,
        27|V_20TRANS
    );

    if (found_radioracers) {
        V_DrawScaledPatch(x+2, y-4, 0, static_cast<patch_t*>(W_CachePatchName("BOOKCUR2", PU_CACHE)));
    }

}

static void M_DrawCurrentCharAndFollower(
    INT16 x,
    INT16 y,
    player_t* p
) {
    // Draw the character first
    if (p->skin < 0 || p->skin > numskins)
        return;

    UINT8 *charcolormap = R_GetTranslationColormap(
        p->skin, 
        static_cast<skincolornum_t>(p->skincolor), 
        GTC_CACHE
    );

    const bool hasfollower = IsFollowerValid(p->followerskin);

    INT16 namex = x+39;
    INT16 iconx = (hasfollower) ? namex - 16: namex - (16/2);
    INT16 icony = y+93;
    INT16 namey = icony+16+5;

    // Background for name and followers
    M_DrawBackgroundForCharAndFollower(x, namey);

    if (!hasfollower)
        namey += 5;

    // Icon
    V_DrawScaledPatch(iconx + 1, icony - 1, 0, static_cast<patch_t*>(W_CachePatchName("ICONBACK", PU_CACHE)));
    V_DrawMappedPatch(iconx, icony, 0, faceprefix[p->skin][FACE_RANK], charcolormap);

    // Character name
    if (bookmarkmenu.show_info)
        V_DrawCenteredThinString(namex, namey, 0, skins[p->skin]->realname);
    
    // Then the follower
    if (hasfollower) {
        INT16 followernamey = namey+10;
        iconx += 16 + 1;

        UINT16 fcolor = K_GetEffectiveFollowerColor(
            p->followercolor,
            &followers[p->followerskin],
            p->skincolor,
            skins[p->skin]
        );

        V_DrawScaledPatch(iconx, icony - 1, 0, static_cast<patch_t*>(W_CachePatchName("ICONBACK", PU_CACHE)));
        V_DrawMappedPatch(
            iconx, 
            icony, 
            0, 
            static_cast<patch_t*>(W_CachePatchName(followers[p->followerskin].icon, PU_CACHE)), 
            R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(fcolor), GTC_MENUCACHE)
        );
        
        if (bookmarkmenu.show_info)
            V_DrawCenteredThinString(namex, followernamey, 0, followers[p->followerskin].name);
    }
}

static void M_DrawPreviewCharAndFollower(INT16 x, INT16 y, characterbookmarkparent_t* current_bookmark_parent) {
    
    INT16 namex = x+39;
    INT16 iconx = namex - 16;
    INT16 icony = y+93;
    INT16 namey = icony+16+5;

    if(current_bookmark_parent == NULL)
        return;

    if(!current_bookmark_parent->valid) {
        if (found_radioracers) {
            patch_t* corrupt_sign = static_cast<patch_t*>(W_CachePatchName("CRRPTBKM", PU_CACHE));
            V_DrawMappedPatch(x+1, y+1, 0, corrupt_sign, NULL);
        }

        V_DrawCenteredThinString(namex, icony, V_REDMAP,"Corrupt bookmark.");
        V_DrawCenteredThinString(namex, icony + 11, V_REDMAP, "Check your file.");
        return;
    }

    // Background for name and followers
    M_DrawBackgroundForCharAndFollower(x, namey);

    // Draw the character first
    characterbookmark_t *bookmark = &current_bookmark_parent->bookmark;

    const bool has_follower = current_bookmark_parent->follower_present;
    iconx = (has_follower) ? namex- 16 : namex - (16/2);

    if (!has_follower) 
        namey += 5;

    UINT8* charcolormap = NULL;
    INT32 skinstringflags = V_YELLOWMAP;
    
    const char* skinstring = "Character not found";
    patch_t* defaultskinicon = static_cast<patch_t*>(W_CachePatchName("K_NOBLNS", PU_CACHE));

    patch_t* valid_bulb = static_cast<patch_t*>(W_CachePatchName("ALTSDOT", PU_CACHE));
    
    // Urghhhhhh
    if (found_radioracers) {
        defaultskinicon = static_cast<patch_t*>(W_CachePatchName("WHORUICO", PU_CACHE));
    }

    patch_t* skinicon = defaultskinicon;
    
    // If the player has bookmarked a character (it's required) and it's ACTUALLY usable
    if (current_bookmark_parent->skin_usable) {
        charcolormap = M_GetBookmarkSkinColormap(current_bookmark_parent);

        skinstringflags = V_GREENMAP;
        skinicon = faceprefix[bookmark->skin][FACE_RANK];
        skinstring = skins[bookmark->skin]->realname;
    } else {
        skinstring = current_bookmark_parent->skin_name;
    }

    // Character Icon
    V_DrawScaledPatch(iconx + 1, icony - 1, 0, static_cast<patch_t*>(W_CachePatchName("ICONBACK", PU_CACHE)));
    V_DrawMappedPatch(iconx, icony, 0, skinicon, charcolormap);

    // Character Name
    if (bookmarkmenu.show_info)
        V_DrawCenteredThinString(namex, namey, skinstringflags, skinstring);

    // Now the follower (if present)
    UINT8* followercolormap = NULL;
    INT32 followerskinstringflags = V_YELLOWMAP;
    const char* followerskinstring = "Follower not found";

    patch_t* followerskinicon = defaultskinicon;
    
    INT16 followernamey = namey+10;
    INT16 followericonx = iconx + 16 + 1;
    if (has_follower) {
        if (current_bookmark_parent->follower_usable) {
            followercolormap = M_GetBookmarkFollowerColormap(current_bookmark_parent);

            followerskinstringflags = V_GREENMAP;

            followerskinicon = static_cast<patch_t*>(W_CachePatchName(followers[bookmark->follower].icon, PU_CACHE));
            followerskinstring = followers[bookmark->follower].name;
        } else {
            followerskinstring = current_bookmark_parent->follower_name;
        }

        // Follower Icon
        V_DrawScaledPatch(followericonx, icony - 1, 0, static_cast<patch_t*>(W_CachePatchName("ICONBACK", PU_CACHE)));
        V_DrawMappedPatch(followericonx, icony, 0, followerskinicon, followercolormap);
    
        // Follower Name
        if (bookmarkmenu.show_info)
            V_DrawCenteredThinString(namex, followernamey, followerskinstringflags, followerskinstring);
    }

    // Draw a little indicator if the character is "incomplete"
    if (current_bookmark_parent->skin_usable && IsBookmarkSkinIncomplete(current_bookmark_parent)) {
        V_DrawMappedPatch(
            iconx - 2, icony-1, 0, valid_bulb, M_GetWarningColormap());
    }
    
    if (has_follower && current_bookmark_parent->follower_usable && IsBookmarkFollowerIncomplete(current_bookmark_parent)) {
        V_DrawMappedPatch(
            followericonx - 2, icony-1, 0, valid_bulb, M_GetWarningColormap());
    }

}

static void M_DrawPreviewWarningsAndErrors(INT16 y, characterbookmarkparent_t* current_bookmark_parent)
{
    // Handled in other places
    if (current_bookmark_parent == NULL || !current_bookmark_parent->valid)
        return;

    std::vector<preview_debug_message_s> messages;

    // Is the character valid?
    if (!current_bookmark_parent->skin_usable) {
        messages.push_back({PREVIEW_ERROR, "* Character not found."});
    } else {
        if(!current_bookmark_parent->skincolor_present) {
            messages.push_back({PREVIEW_WARNING, "* Character colour missing."});
        } else {
            if (!current_bookmark_parent->skincolor_usable) {
                messages.push_back({
                    PREVIEW_WARNING,
                    "* Character colour not found, using default."
                });
            }
        }
    }

    // Is the follower valid?
    if (current_bookmark_parent->follower_present) {
        if(!current_bookmark_parent->follower_usable) {
            messages.push_back({PREVIEW_ERROR, "* Follower not found."});
        } else {
            if(!current_bookmark_parent->followercolor_present) {
                messages.push_back({PREVIEW_WARNING, "* Follower colour missing."});
            } else {
                if (!current_bookmark_parent->followercolor_usable) {
                    messages.push_back({
                        PREVIEW_WARNING,
                        "* Follower colour not found, using default."
                    });
                }
            }
        }
    }

    auto draw_message = [&](const char* message, fixed_t y, INT32 flags) {
        fixed_t error_x = (BASEVIDWIDTH - 8) << FRACBITS;
        error_x -= V_StringScaledWidth(FRACUNIT/2, FRACUNIT, FRACUNIT, 0, TINY_FONT, message);

        V_DrawStringScaled(
            error_x, 
            y, 
            FRACUNIT/2, 
            FRACUNIT, 
            FRACUNIT, 
            flags, 
            NULL,
            TINY_FONT, 
            message
        );
    };

    INT16 message_y = y - 10;
    for (auto& debug_message: messages) {
        INT32 flags = (debug_message.type == PREVIEW_ERROR) ? V_REDMAP : V_YELLOWMAP;
        draw_message(
            debug_message.message.c_str(), (message_y) << FRACBITS, flags);
        message_y -= 6;
    }

}

static void M_DrawPreviewCharacterCard(INT16 x, INT16 y, characterbookmarkparent_t* current_bookmark_parent) {
    UINT8 *charcolormap;
    INT32 addflags = 0;

    const bool bookmarked_applied = bookmarkmenu.stage == BMENU_STAGE_APPLIED_BOOKMARK;
    if (bookmarked_applied)
        addflags = V_TRANSLUCENT;

    V_DrawScaledPatch(
        x, 
        y, 
        0, 
        static_cast<patch_t*>(W_CachePatchName("DUELGRPH", PU_CACHE))
    );

	V_DrawScaledPatch(
        x+8, 
        y+9,
        V_TRANSLUCENT,
        static_cast<patch_t*>(W_CachePatchName("PREVBACK", PU_CACHE))
    );

    
    // No bookmark? Just draw a silhouette of Eggman and call it a day
    if (current_bookmark_parent == NULL) {
        M_DrawPreviewSilhouette(x, y);
    } else if(current_bookmark_parent->valid) {
        // Draw the preview character
        characterbookmark_t *bookmark = &current_bookmark_parent->bookmark;
        if (current_bookmark_parent->skin_usable) {

            skincolornum_t charcolornum = SKINCOLOR_NONE;

            if (current_bookmark_parent->skincolor_present && current_bookmark_parent->skincolor_usable) {
                charcolornum = static_cast<skincolornum_t>(bookmark->skincolor);
            }  else {
                charcolornum = static_cast<skincolornum_t>(skins[bookmark->skin]->prefcolor);
            }

            charcolormap = R_GetTranslationColormap(
                bookmark->skin, 
                charcolornum, 
                GTC_CACHE
            );

            M_DrawCharacterSprite(
                x+40,
                y+78,
                bookmark->skin,
                SPR2_STIN,
                7,
                0,
                addflags,
                charcolormap
            );
        } else {
            M_DrawPreviewSilhouette(x, y);
        }

        // Draw the follower
        const bool has_follower = current_bookmark_parent->follower_present && current_bookmark_parent->follower_usable;
        if (has_follower) {
            UINT8 *follower_colormap = M_GetBookmarkFollowerColormap(current_bookmark_parent);
        
            M_DrawFollowerSprite(
                x+20,
                y+78,
                bookmark->follower,
                follower_colormap,
                addflags,
                &bookmarkmenu.preview_follower
            );
        }
    }

    // TITLE
    V_DrawCenteredFileString(x+37, y-4, 0, "PREVIEW");

    if (bookmarked_applied)
        V_DrawCenteredString(x+39, y+40, V_GREENMAP, "Confirmed!");

    // ERRORS AND WARNINGS
    M_DrawPreviewWarningsAndErrors(y, current_bookmark_parent);
}


static void M_DrawCharacterCard(INT16 x, INT16 y, player_t *p) {
    if (p->skin < 0 || p->skin > numskins)
        return;

    INT32 addflags = 0;
    const bool has_bookmarked = bookmarkmenu.stage >= BMENU_STAGE_BOOKMARKED && bookmarkmenu.stage < BMENU_STAGE_APPLIED_BOOKMARK;
    if (has_bookmarked)
        addflags = V_TRANSLUCENT;

    UINT8 *charcolormap = R_GetTranslationColormap(
        p->skin, 
        static_cast<skincolornum_t>(p->skincolor), 
        GTC_CACHE
    );

    V_DrawScaledPatch(
        x, 
        y, 
        0, 
        static_cast<patch_t*>(W_CachePatchName("DUELGRPH", PU_CACHE))
    );

	V_DrawScaledPatch(
        x+8, 
        y+9,
        V_TRANSLUCENT,
        static_cast<patch_t*>(W_CachePatchName("PREVBACK", PU_CACHE))
    );

    M_DrawCharacterSprite(
        x+40,
        y+78,
        p->skin,
        SPR2_STIN,
        7,
        0,
        addflags,
        charcolormap
    );

    // Draw the follower (if present)
    const bool has_follower = IsFollowerValid(p->followerskin);

    if (has_follower) {
        UINT16 followercolor = K_GetEffectiveFollowerColor(
            p->followercolor,
            &followers[p->followerskin],
            p->skincolor,
            skins[p->skin]
        );

        UINT8 *follower_colormap = R_GetTranslationColormap(
            TC_DEFAULT, 
            static_cast<skincolornum_t>(followercolor), 
            GTC_MENUCACHE
        );
    
        M_DrawFollowerSprite(
            x+20,
            y+78,
            p->followerskin,
            follower_colormap,
            addflags,
            &bookmarkmenu.current_follower
        );
    }

    // Title
    V_DrawCenteredFileString(x+37, y-4, 0, "CURRENT");

    // Confirmation
    if (has_bookmarked)
        V_DrawCenteredString(x+39, y+40, V_GREENMAP, "Saved!");
}

static void M_DrawSingleBookmark(INT16 x, INT16 y, INT16 index, bool highlighted)
{
    characterbookmarkparent_t* bookmark_parent = FetchBookmarkParent(index);
    const bool bookmark_valid = (bookmark_parent != NULL && bookmark_parent->valid);
    const INT32 flags = (highlighted) ? 0 : V_40TRANS;

    // First, draw the little zit to show if the bookmark is valid or not
    patch_t* valid_bulb = static_cast<patch_t*>(W_CachePatchName("K_BLNA", PU_CACHE));

    x += 2;

    if (!bookmark_valid) {
        V_DrawMappedPatch(x, y+1, flags, valid_bulb, M_GetErrorColormap());
        V_DrawThinString(x+10, y, V_REDMAP|flags, "CORRUPT");
        return;
    }

    UINT8* bulbcolormap = NULL;
    const bool is_bookmark_unusable = IsBookmarkUnusable(bookmark_parent);

    if (is_bookmark_unusable) {
        bulbcolormap = M_GetErrorColormap();
    } else {
        bulbcolormap = IsBookmarkIncomplete(bookmark_parent) ? M_GetWarningColormap() : NULL;
    }
    V_DrawMappedPatch(x, y+1, flags, valid_bulb, bulbcolormap);

    // Then, draw the character icon (if present)
    characterbookmark_t *bookmark = &bookmark_parent->bookmark;

    x += 9;
    y-= 3;

    patch_t* defaultskinicon = static_cast<patch_t*>(W_CachePatchName("K_NOBLNS", PU_CACHE));
    if (found_radioracers) {
        defaultskinicon = static_cast<patch_t*>(W_CachePatchName("WHORUICO", PU_CACHE));
    }
    patch_t* skinicon = defaultskinicon;
    UINT8* charcolormap = NULL;

    if (bookmark_parent->skin_usable) {
        charcolormap = M_GetBookmarkSkinColormap(bookmark_parent);
        skinicon = faceprefix[bookmark->skin][FACE_RANK];
    }

    V_DrawMappedPatch(x, y, flags, skinicon, charcolormap);

    // Then, draw the follower icon (if present)
    x+= (16) + 1;

    UINT8* followercolormap = NULL;
    patch_t* followerskinicon = defaultskinicon;

    if (bookmark_parent->follower_present) {
        if (bookmark_parent->follower_usable) {
            followercolormap = M_GetBookmarkFollowerColormap(bookmark_parent);
            followerskinicon = static_cast<patch_t*>(
                W_CachePatchName(followers[bookmark->follower].icon, PU_CACHE));
        }
        V_DrawMappedPatch(x, y, flags, followerskinicon, followercolormap);
    }
}

static void M_DrawPlaceholderBookmark(INT16 x, INT16 y)
{
    // Draw a grey zit
    patch_t* valid_bulb = static_cast<patch_t*>(W_CachePatchName("K_BLNA", PU_CACHE));

    x += 2;
    
    UINT8* colormap = R_GetTranslationColormap(
        TC_DEFAULT, 
        SKINCOLOR_SILVER, 
        GTC_CACHE
    );
    V_DrawMappedPatch(x, y+1, 0, valid_bulb, colormap);
}

static void M_DrawBookmarkCursor(INT16 x, INT16 y)
{
    // Urghhhh
    if (found_radioracers) {
        V_DrawMappedPatch(
            x - 6, 
            y - 10, 
            0, 
            static_cast<patch_t*>(W_CachePatchName("BOOKCURS", PU_CACHE)),
            NULL
        );
    } else {
        V_DrawFill(x, y - 6, 20, 5, 50);
    }
}

void RRM_DrawCharacterBookmarks(void) {
    player_t* p = &players[g_localplayers[0]];
    INT16 x = 1;
    INT16 y = (BASEVIDHEIGHT/2) - 55;
    
    // DRAWING THE CURRENT (selection)
    M_DrawCharacterCard(x, y, p);
    M_DrawCurrentCharAndFollower(x, y, p);

    // DRAWING THE PREVIEW (selection)
    INT16 x2 = 236;
    characterbookmarkparent_t *current_bookmark_parent = FetchSelectedBookmarkParent();

    M_DrawPreviewCharacterCard(x2, y, current_bookmark_parent);
    M_DrawPreviewCharAndFollower(x2, y, current_bookmark_parent);

    // DRAWING THE MENU (to select)
    const INT16 start_x = 87;

    INT16 row_y = 33;
    INT16 row_x = start_x;
    INT16 bookmark_menu_index = M_GetPageOffset();

    // No fancy borders for YOU
    if (found_radioracers) {
        V_DrawScaledPatch(
            start_x, 
            row_y - 4, 
            V_TRANSLUCENT, 
            static_cast<patch_t*>(W_CachePatchName("BOKGBACK", PU_CACHE))
        );
        V_DrawScaledPatch(
            start_x - 2, 
            row_y - 4, 
            0, 
            static_cast<patch_t*>(W_CachePatchName("BOOKGRPH", PU_CACHE))
        );
    }
    
    // Draw the page numbers
    const INT16 page_button_x = start_x;
    const INT16 page_button_y = row_y + 141;
    const size_t max_pages = M_GetMaxPagesForBookmarkMenu();
    const std::string l_button = (max_pages > 1) ? "<l_animated>" : "<l>";
    K_DrawGameControl(
        page_button_x - 3, 
        page_button_y, 
        0, 
        va("%s Page %d of %d", 
            l_button.c_str(),
            bookmarkmenu.current_page + 1, M_GetMaxPagesForBookmarkMenu()), 
        0, 
        TINY_FONT, 
        0
    );

    if (!IsBookmarkUnusable(current_bookmark_parent)) {
        K_DrawGameControl(
            page_button_x + 145, 
            page_button_y, 
            0, 
            std::string("<a_animated> \x83").append("Accept").c_str(), 
            2, 
            TINY_FONT, 
            0
        );
    }

    INT16 current_row = (bookmark_idx) / COLUMNS;
    INT16 current_column = (bookmark_idx) % COLUMNS;

    INT16 cursor_row_x = 0;
    INT16 cursor_row_y = 0;

    bool highlighted = false;
    for (INT16 i = 0; i < ROWS; i++) {
        for (INT16 j = 0; j < COLUMNS; j++) {
            highlighted = (i == current_row && j == current_column);
            if (bookmark_menu_index >= char_bookmarks.size()) {
                M_DrawPlaceholderBookmark(row_x, row_y);
            } else {
                M_DrawSingleBookmark(row_x, row_y, bookmark_menu_index, highlighted);
            }

            if (highlighted) {
                cursor_row_x = row_x;
                cursor_row_y = row_y;
            }
            
            row_x += 48;
            bookmark_menu_index++;
        }
        row_x = start_x;
        row_y += 20;
    }

    M_DrawBookmarkCursor(cursor_row_x, cursor_row_y);

    // Buttons
    const int button_y = 8;

    std::string bookmark_action = (current_bookmark_parent != NULL) ? 
        "<y> \x82Overwrite" :
        "<y> Bookmark";
    if (current_bookmark_parent == NULL && M_AreBookmarksFull()) {
        bookmark_action = std::string("\x85").append("Bookmarks full.");
    }
    std::string delete_action = (current_bookmark_parent == NULL) ? "" : "  <c> Delete";

    std::string bookmark_buttons = M_GetText(va("%s%s", bookmark_action.c_str(), delete_action.c_str()));
    
    K_DrawGameControl(
        start_x - 5, 
        button_y, 
        0, 
        bookmark_buttons.c_str(),
        0, 
        TINY_FONT, 
        0
    );
    
    K_DrawGameControl(
        start_x + 150, 
        button_y,
        0, 
        "<r_animated> Info", 
        2, 
        TINY_FONT, 
        0
    );
}

