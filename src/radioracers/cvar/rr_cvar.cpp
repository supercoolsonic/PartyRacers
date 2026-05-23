// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/cvar/rr_cvar.cpp

#include "../rr_cvar.h"
#include "../../doomstat.h"

#include "../rr_hud.h"
#include "../rr_setup.h"

void KartLocalEncore_OnChange(void)
{
    localencore = (boolean)cv_applylocalencore.value;
    CONS_Printf(M_GetText("Encore Mode palettes will be \x82%s\x80 next round.\n"), cv_applylocalencore.string);
}

static boolean FakeNetCvar_OnChange(consvar_t* cvar)
{
    if (netgame && !cvar->enablefornetgames) {
        CONS_Alert(
            CONS_NOTICE,
            M_GetText("This feature cannot be toggled, you are playing in a server.\n")
        );
        CV_StealthSetValue(cvar, 0);
        return false;
    }
    return true;
}

void KartHaki_OnChange(void)
{
	// Check if it's been disabled by the server
    // or if it's a core server
    if (!FakeNetCvar_OnChange(&cv_applyhaki)) {
        hakimode = false;
        return;
    }

    // Check if in a level before enabling it
    if (gamestate != GS_LEVEL) {
        CONS_Alert(
            CONS_NOTICE,
            M_GetText("You must be in a level to toggle this feature.\n")
        );
        CV_StealthSetValue(&cv_applyhaki, 0);
        return;
    }
	
    hakimode = (boolean)cv_applyhaki.value;

    if (hakimode == 1) {
        // Missing the lump needed for this effect to work!
        if (!radioracers_usehakiencore) {
            CONS_Alert(
                CONS_NOTICE,
                M_GetText("This feature cannot be enabled, missing the graphic lump.\n")
            );
            CV_StealthSetValue(&cv_applyhaki, 0);
            return;
        }
    }
    // TODO: Play dramatic sound.
    CONS_Printf(M_GetText("Your observation haki will be turned \x82%s\x80 next round.\n"), cv_applyhaki.string);
}

void EmeraldsMinimap_OnChange(void)
{
    if (!FakeNetCvar_OnChange(&cv_battle_toggle_emerald_on_minimap))
        return;
}

void AccessibilityRings_OnChange(void)
{
    if (!FakeNetCvar_OnChange(&cv_accessibility_rings_hide))
        return;
}

void KartExtraPowerSound_OnChange(void)
{
    boolean extra_power_sound = (boolean)cv_powersoundjoke.value;
    if (extra_power_sound)
    {
        if ((!found_radioracers || radio_last_powerup_jingle_sound == sfx_None) ) {
            CONS_Alert(
                CONS_NOTICE,
                M_GetText("This feature cannot be enabled, missing the sound lump.\n")
            );
            CV_StealthSetValue(&cv_powersoundjoke, 0);
            return;
        }
    }
}

static void resetCvarWithNotice(consvar_t *cvar) {
    CONS_Alert(
        CONS_NOTICE,
        M_GetText("This feature cannot be enabled, missing radioracers.pk3.\n")
    );
    CV_StealthSetValue(cvar, 0);
}

static boolean RR_MissingWad_OnChange(consvar_t *cvar)
{
    boolean enabled = (boolean)cvar->value;
    if (!found_radioracers && enabled) {
        CONS_Alert(
            CONS_NOTICE,
            M_GetText("This feature cannot be enabled, missing radioracers.pk3.\n")
        );
        CV_StealthSetValue(cvar, 0);
        return false;
    }
    return true;
}

void RR_ObviousTripwire_OnChange(void) {
    if (!RR_MissingWad_OnChange(&cv_obvious_tripwire)) {
        return;
    }
}

void RR_Hudfeed_OnChange(void) {
    boolean enabled = (boolean)(cv_hudfeed_enabled.value);
    if (enabled) {
        if (!found_radioracers || !radioracers_usehudfeed) {
            resetCvarWithNotice(&cv_hudfeed_enabled);
            return;
        }
    } else {
        // Being turned off, clear the feed
        RR_ClearHudFeed();
    }
}

void KartFinishLineTicker_OnChange(void)
{
    // Immediately empty the queue
    if (!cv_show_riders_finish_ticker.value)
        RR_resetRidersFinishTicker();
}

void RR_ChatEmotes_OnChange(void)
{
    // Immediately turn off all the emote menus/preview values
    if (!cv_chat_emotes.value)
    {
        RR_ResetAllEmoteChatInfo();
    }
}