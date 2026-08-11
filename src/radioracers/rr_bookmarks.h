// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2026 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/netgame/rr_bookmarks.h
/// \brief Character bookmarks

#ifndef RR_BOOKMARKS_H
#define RR_BOOKMARKS_H

#include "../doomtype.h"
#include "../doomdef.h"

#ifdef __cplusplus

#include <vector>

// Prone to change
#define MAX_BOOKMARKS 50

typedef struct {
    // d_player.h
    int32_t skin;
    uint16_t skincolor;
    int32_t follower;
    uint16_t followercolor;

} characterbookmark_t;

typedef struct {
    bool valid;
    bool skin_usable;
    bool skincolor_present;
    bool skincolor_usable;
    bool follower_present;
    bool follower_usable;
    bool followercolor_present;
    bool followercolor_usable;
    characterbookmark_t bookmark;

    char skin_name[SKINNAMESIZE+1];
    char skincolor_name[MAXCOLORNAME+1];
    char follower_name[SKINNAMESIZE+1];
    char followercolor_name[MAXCOLORNAME+1];
} characterbookmarkparent_t;

extern std::vector<characterbookmarkparent_t> char_bookmarks;
// Functions
void RR_InitBookmarkCommands(void);
void RR_VerifyBookmarks(void);

extern "C" {
#endif

#define RADIO_BOOKMARKS_FILE "rr_bookmarks.txt"

#ifdef __cplusplus
} // extern "C"
#endif
#endif