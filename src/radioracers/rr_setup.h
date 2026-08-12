// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_setup.h

#ifndef RR_SETUP_H
#define RR_SETUP_H

#include "../doomtype.h"
#include "../doomdef.h"
#include "../../s_sound.h"

#ifdef __cplusplus
extern "C" {
#endif

// Music
extern uint8_t radio_maxrandompositionmus;
extern uint8_t radio_mapmusrng;

extern dboolean found_radioracers;
extern dboolean found_radioracers_plus;
extern dboolean radioracers_usemuteicons;
extern dboolean radioracers_usebookmarkicons;
extern dboolean radioracers_usehakiencore;
extern dboolean radioracers_useendkey;
extern dboolean radioracers_usehudfeed;
extern dboolean radioracers_usealternatetripwire;

/**
 * Textures in this game are stored by ID, not name, which is sensible.
 * To avoid having to look up the ID for custom textures, find it ONCE during texture setup, grab the ID and assign it.
 */

// Base texture IDs

extern int32_t RADIO_BADWIRE_SHORT_TEX_ID;
extern int32_t RADIO_BADWIRE_2X_SHORT_TEX_ID;
extern int32_t RADIO_BADWIRE_4X_SHORT_TEX_ID;
extern int32_t RADIO_BADWIRE_TALL_TEX_ID;
extern int32_t RADIO_BADWIRE_2X_TALL_TEX_ID;
extern int32_t RADIO_BADWIRE_4X_TALL_TEX_ID;
extern int32_t RADIO_BADWIRE_VERTICAL_TEX_ID;

extern int32_t RADIO_GOODWIRE_SHORT_TEX_ID;
extern int32_t RADIO_GOODWIRE_2X_SHORT_TEX_ID;
extern int32_t RADIO_GOODWIRE_4X_SHORT_TEX_ID;
extern int32_t RADIO_GOODWIRE_TALL_TEX_ID;
extern int32_t RADIO_GOODWIRE_2X_TALL_TEX_ID;
extern int32_t RADIO_GOODWIRE_4X_TALL_TEX_ID;
extern int32_t RADIO_GOODWIRE_VERTICAL_TEX_ID;

// Texture name
extern const char* RADIO_BADWIRE_SHORT_TEX_NAME;
extern const char* RADIO_BADWIRE_2X_SHORT_TEX_NAME;
extern const char* RADIO_BADWIRE_4X_SHORT_TEX_NAME;
extern const char* RADIO_BADWIRE_TALL_TEX_NAME;
extern const char* RADIO_BADWIRE_2X_TALL_TEX_NAME;
extern const char* RADIO_BADWIRE_4X_TALL_TEX_NAME;
extern const char* RADIO_BADWIRE_VERTICAL_TEX_NAME;

extern const char* RADIO_GOODWIRE_SHORT_TEX_NAME;
extern const char* RADIO_GOODWIRE_2X_SHORT_TEX_NAME;
extern const char* RADIO_GOODWIRE_4X_SHORT_TEX_NAME;
extern const char* RADIO_GOODWIRE_TALL_TEX_NAME;
extern const char* RADIO_GOODWIRE_2X_TALL_TEX_NAME;
extern const char* RADIO_GOODWIRE_4X_TALL_TEX_NAME;
extern const char* RADIO_GOODWIRE_VERTICAL_TEX_NAME;

extern patch_t *end_key[2];
extern sfxenum_t radio_ding_sound;
extern sfxenum_t radio_last_powerup_jingle_sound;
extern sfxenum_t radio_s_rank_voiceline;
extern sfxenum_t radio_perfectboost_line;
extern void RR_Init(void);
extern void RR_CheckForServerConfig(uint16_t wadnum);
extern void RR_AddAllEmotes(uint16_t wadnum);
extern void RR_CleanupEmoteFrames(void);
extern void RR_SaveEmoteUsage(void);
extern void RR_resetRadioFakeNetCvars(void);
extern void RR_SaveFavouriteEmotes(void);
extern void RR_UpdateEmoteUsageVector(void);
extern void RR_FavouriteEmote(char* name);
extern void RR_UnfavouriteEmote(char* name);
extern void RR_SaveBookmarks(void);
extern void RR_LoadBookmarks(void);
extern void RR_RefreshBookmarks(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
