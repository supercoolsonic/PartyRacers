// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/util/rr_util.c
/// \brief Util functions


#include "../../doomtype.h"

#include <ctype.h>

#include "../../fastcmp.h"
#include "../../deh_tables.h"
#include "../../doomstat.h" // encoremode, localencore
#include "../rr_util.h" // encoremode, localencore
#include "../rr_cvar.h"
#include "../rr_setup.h"
#include "../rr_controller.h"
#include "../../p_local.h"
#include "../../hu_stuff.h"
#include "../../g_game.h"
#include "../../s_sound.h"
#include "../../k_kart.h"
#include "../../k_hud.h"
#include "../../r_textures.h"
#include "../../r_fps.h"

dboolean shouldApplyEncore(void)
{
    // Check the encoremode flag first, THEN the clientside flag.
    return (encoremode || localencore) || (shouldUseHaki());
}

dboolean shouldUseHaki(void)
{
    // Prefer to use a function rather then just checking for 'hakimode' everywhere.
    if (netgame) {
        if (cv_applyhaki.enablefornetgames) {
            return hakimode;
        }
        return false;
    }
    return hakimode;
}

void RR_HandleBlueSphereRumble(player_t *player)
{
    if (!cv_rr_rumble_spheres.value)
        return;

    if (P_IsMachineLocalPlayer(player)) {
        localPlayerPickupSpheresDelay = 0; // reset the delay everytime a player picks up a sphere

        if (localPlayerPickupSpheresDelay < 4)
            localPlayerPickupSpheres++;
    }
}

static const char* BATTLE_WIN_MESSAGES [] = {
    "\x82%s\\FUCKING WINS!", // Points
    "\x82%s\\GOT ALL OF THE FUCKING EMERALDS!", // Emeralds  
};

/**
 * Just do a cecho
 */
void RR_AnnounceBattleWinner(player_t *player, battle_win_type_t type)
{
    if (!cv_battle_toggle_winner_announcement.value)
        return;

    HU_SetCEchoFlags(0);
    HU_SetCEchoDuration(4);

    // This used to crash in netgames but just to be safe..
    if (type < 0 || type >= sizeof(BATTLE_WIN_MESSAGES) / sizeof(BATTLE_WIN_MESSAGES[0])) {
        return;
    }

    // va causes crashes sometimes but ONLY semi-rarely, weird.
    HU_DoCEcho(va(BATTLE_WIN_MESSAGES[type], player_names[player-players]));    
}

dboolean RR_IsBattle(void) {
    return (gametyperules & GTR_BUMPERS);
}

void RR_PlayCountdownJingle(int16_t timer, player_t *player) {
    if (!cv_powersound.value)
        return;

    if (stplyr != player)
        return;

    if (timer > 0 && timer <= 3 * TICRATE)
    {
        if (timer % TICRATE == 0)
        {
            S_StartSound(NULL, sfx_s242);
            
            // (skypegiggle)
            if (found_radioracers && cv_powersoundjoke.value && radio_last_powerup_jingle_sound != sfx_None && timer == TICRATE) {
                S_StartSoundAtVolume(NULL, radio_last_powerup_jingle_sound, 255/4);
            }
        }
    }
}

static dboolean isGTopArrow(mobj_t* mo)			//SCS ADD
{
    return (mo->type == MT_GARDENTOPARROW);
}	
static dboolean isRing(mobj_t* mo)
{
    return (mo->type == MT_RING || mo->type == MT_FLINGRING);
}
static dboolean isRingBox(mobj_t* mo)
{
    statenum_t specialstate = mo->state - states;
    return (mo->type == MT_RANDOMITEM) && (specialstate >= S_RINGBOX1 && specialstate <= S_RINGBOX12);
}

static dboolean canGhost(void)
{
    if (netgame && !cv_accessibility_rings_hide.enablefornetgames) {
        return false;
    }
    return cv_accessibility_rings_hide.value && r_splitscreen == 0;
}

dboolean RR_ShouldShowEmeraldsInMinimap(void)
{
    if (netgame && !cv_battle_toggle_emerald_on_minimap.enablefornetgames) {
        return false;
    }
    return cv_battle_toggle_emerald_on_minimap.value;
}

dboolean RR_ShouldGhostGardenArrowTop(mobj_t *mo)		//SCS ADD
{
    return isGTopArrow(mo) &&
    !(P_MobjWasRemoved(mo)) && 
    (cv_toggle_gardentoparrows.value == 2 || cv_toggle_gardentoparrows.value == 4);
}		

dboolean RR_ShouldGhostGardenArrowFloor(mobj_t *mo)		//SCS ADD
{
    return isGTopArrow(mo) &&
    !(P_MobjWasRemoved(mo)) && 
    (cv_toggle_gardentoparrows.value == 1 || cv_toggle_gardentoparrows.value == 4);
}		

dboolean RR_ShouldGhostRing(mobj_t *mo)
{
    return canGhost() &&
    isRing(mo) && 
    !(!P_MobjWasRemoved(mo->target) && mo->target->type == MT_PLAYER) && 
    (IS_BEING_CHASED_BY_SPB(stplyr) || (RINGTOTAL(stplyr) >= stplyr->ringlimit && !((stplyr)->timestonefrozen))); //SCS EDIT - Don't ghost rings if we have the Time Stone active, since we get to keep picking them up to add to superring
}											
dboolean RR_ShouldGhostRingboxes(mobj_t *mo)
{
    return canGhost() &&
    isRingBox(mo) && 
    (IS_BEING_CHASED_BY_SPB(stplyr));
}

dboolean RR_ShouldGhostItemCapsuleParts(mobj_t *mo)
{
    return canGhost() &&
    (
        (mo->type == MT_ITEMCAPSULE_PART && (mo->sprite == SPR_ITEM && mo->frame & KITEM_SUPERRING)) ||
        (mo->type == MT_ITEMCAPSULE)
    ) &&
    IS_BEING_CHASED_BY_SPB(stplyr);
}

dboolean RR_ShouldGhostItemCapsuleNumbers(mobj_t *mo)
{
    // isSuperRingItemNumber is only set to true in p_mobj.c in P_RefreshItemCapsuleParts
    return canGhost() && 
    mo->isSuperRingItemNumber &&
    IS_BEING_CHASED_BY_SPB(stplyr);
}

/**
 * https://github.com/blondedradio/RadioRacers/issues/14
 * 
 * Getting a voltage charge increases the speed of a player's drift sparks.
 * To *better* convey this to the player, 
 * re-colour the aura VFX with the colour as their drift sparks.
 */
dboolean RR_ShouldRecolorVoltage(mobj_t *mo) 
{
    dboolean isVoltage = (mo->type == MT_CHARGEAURA);
    dboolean hasValidTarget = !P_MobjWasRemoved(mo->target) 
				&& mo->target->player
				&& mo->target->player == stplyr;

    return cv_obvious_voltage.value && r_splitscreen == 0 && isVoltage && hasValidTarget;
}

int32_t RR_FetchAlternateTripwire(int32_t original_textnum)
{
    if (!found_radioracers || !radioracers_usealternatetripwire || !cv_obvious_tripwire.value || r_splitscreen > 0) 
        return original_textnum;

    char texname[9];
    strncpy(texname, textures[original_textnum]->name, 8);
    texname[8] = '\0';

    // Static
    dboolean is_short_tripwire_texture = strstr(texname, "TRIPWLOW") != NULL;
    dboolean is_short_2x_tripwire_texture = strstr(texname, "2RIPWLOW") != NULL;
    dboolean is_short_4x_tripwire_texture = strstr(texname, "4RIPWLOW") != NULL;

    dboolean is_tall_tripwire_texture = strstr(texname, "TRIPWIRE") != NULL;
    dboolean is_tall_2x_tripwire_texture = strstr(texname, "2RIPWIRE") != NULL;
    dboolean is_tall_4x_tripwire_texture = strstr(texname, "4RIPWIRE") != NULL;

    dboolean is_vertical_tripwire = strstr(texname, "VRIPWIRE") != NULL;

    // Animated
    dboolean is_short_tripwire_animated_texture = strstr(texname, "TWLOW") != NULL;
    dboolean is_short_2x_tripwire_animated_texture = strstr(texname, "2WLOW") != NULL;
    dboolean is_short_4x_tripwire_animated_texture = strstr(texname, "4WLOW") != NULL;

    dboolean is_tall_tripwire_animated_texture = strstr(texname, "TWIRE") != NULL;
    dboolean is_tall_2x_tripwire_animated_texture = strstr(texname, "2WIRE") != NULL;
    dboolean is_tall_4x_tripwire_animated_texture = strstr(texname, "4WIRE") != NULL;

    dboolean is_vertical_tripwire_animated_texture = strstr(texname, "VWIRE") != NULL;

    dboolean is_short_tripwire = (is_short_tripwire_texture || is_short_2x_tripwire_texture || is_short_4x_tripwire_texture);
    dboolean is_short_tripwire_animated = (is_short_tripwire_animated_texture || is_short_2x_tripwire_animated_texture || is_short_4x_tripwire_animated_texture);
    
    dboolean is_tall_tripwire = (is_tall_tripwire_texture || is_tall_2x_tripwire_texture || is_tall_4x_tripwire_texture);
    dboolean is_tall_tripwire_animated = (is_tall_tripwire_animated_texture || is_tall_2x_tripwire_animated_texture || is_tall_4x_tripwire_animated_texture);

    dboolean is_tripwire = (is_short_tripwire || is_tall_tripwire || is_vertical_tripwire);
    dboolean is_animated_tripwire = (is_short_tripwire_animated || is_tall_tripwire_animated || is_vertical_tripwire_animated_texture);

    // Is this actually a tripwire texture?
    if (!is_tripwire && !is_animated_tripwire) {
        return original_textnum;
    }

    // Player's tripwire eligiblity
	if (!K_TripwirePass(stplyr)) {
        // Show BAD tripwire (red)
        if (is_tripwire) {
            // Static
            if (is_short_tripwire) {
                if (is_short_tripwire_texture) {
                    return RADIO_BADWIRE_SHORT_TEX_ID;
                } else if(is_short_2x_tripwire_texture) {
                    return RADIO_BADWIRE_2X_SHORT_TEX_ID;
                } else if(is_short_4x_tripwire_texture) {
                    return RADIO_BADWIRE_4X_SHORT_TEX_ID;
                }
            } else if (is_tall_tripwire) {
                if (is_tall_tripwire_texture) {
                    return RADIO_BADWIRE_TALL_TEX_ID;
                } else if(is_tall_2x_tripwire_texture) {
                    return RADIO_BADWIRE_2X_TALL_TEX_ID;
                } else if(is_tall_4x_tripwire_texture) {
                    return RADIO_BADWIRE_4X_TALL_TEX_ID;
                }
            } else if (is_vertical_tripwire) {
                return RADIO_BADWIRE_VERTICAL_TEX_ID;
            }
        } else { 
            // Animated
            if (is_short_tripwire_animated) {
                if (is_short_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_SHORT_TEX_ID);
                } else if(is_short_2x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_2X_SHORT_TEX_ID);
                } else if(is_short_4x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_4X_SHORT_TEX_ID);
                }
            } else if (is_tall_tripwire_animated) {
                if (is_tall_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_TALL_TEX_ID);
                } else if(is_tall_2x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_2X_TALL_TEX_ID);
                } else if(is_tall_4x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_BADWIRE_4X_TALL_TEX_ID);
                }
            } else if (is_vertical_tripwire_animated_texture) {
                return R_GetTextureNum(RADIO_BADWIRE_VERTICAL_TEX_ID);
            }
        }
	} else {
        // Show GOOD tripwire (green)
        if (is_tripwire) {
            // Static
            if (is_short_tripwire) {
                if (is_short_tripwire_texture) {
                    return RADIO_GOODWIRE_SHORT_TEX_ID;
                } else if(is_short_2x_tripwire_texture) {
                    return RADIO_GOODWIRE_2X_SHORT_TEX_ID;
                } else if(is_short_4x_tripwire_texture) {
                    return RADIO_GOODWIRE_4X_SHORT_TEX_ID;
                }
            } else if (is_tall_tripwire) {
                if (is_tall_tripwire_texture) {
                    return RADIO_GOODWIRE_TALL_TEX_ID;
                } else if(is_tall_2x_tripwire_texture) {
                    return RADIO_GOODWIRE_2X_TALL_TEX_ID;
                } else if(is_tall_4x_tripwire_texture) {
                    return RADIO_GOODWIRE_4X_TALL_TEX_ID;
                }
            } else if (is_vertical_tripwire) {
                return RADIO_GOODWIRE_VERTICAL_TEX_ID;
            }
        } else { 
            // Animated
            if (is_short_tripwire_animated) {
                if (is_short_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_SHORT_TEX_ID);
                } else if(is_short_2x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_2X_SHORT_TEX_ID);
                } else if(is_short_4x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_4X_SHORT_TEX_ID);
                }
            } else if (is_tall_tripwire_animated) {
                if (is_tall_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_TALL_TEX_ID);
                } else if(is_tall_2x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_2X_TALL_TEX_ID);
                } else if(is_tall_4x_tripwire_animated_texture) {
                    return R_GetTextureNum(RADIO_GOODWIRE_4X_TALL_TEX_ID);
                }
            } else if (is_vertical_tripwire_animated_texture) {
                return R_GetTextureNum(RADIO_GOODWIRE_VERTICAL_TEX_ID);
            }
        }
    }

    // If all else fails
    return original_textnum;
}

void RR_GetTrackingCoordinatesForPlayer(trackingResult_t *result, dboolean playerHasMobj) {
    vector3_t v;

    if (playerHasMobj)
    {
        v.x = R_InterpolateFixed(stplyr->mo->old_x, stplyr->mo->x);
        v.y = R_InterpolateFixed(stplyr->mo->old_y, stplyr->mo->y);
        v.z = R_InterpolateFixed(stplyr->mo->old_z, stplyr->mo->z);

        // Legacy GL perspective
        v.z += FixedMul(-15*FRACUNIT, stplyr->mo->scale);

        /*
        * Many thanks to Nev3r for figuring out the math for this functionality, opens up a lot of
        possiblities.
        */
        K_ObjectTracking(result, &v, false);
    } 
}

int scaleInt(int value, fixed_t scale)
{
    return value - (value - (int)(round(value * FixedToFloat(scale))));
}

// Uplifted from deh_soc.c
uint16_t get_skincolornum(const char *word)
{ // Returns the value of SKINCOLOR_ enumerations
	uint16_t i;
	if (*word >= '0' && *word <= '9')
		return atoi(word);
	if (fastncmp("SKINCOLOR_",word,10))
		word += 10; // take off the SKINCOLOR_

    char upper_word[MAXCOLORNAME+1];
    strlcpy(upper_word, word, sizeof(upper_word));
    strupr(upper_word);

	for (i = 0; i < NUMCOLORFREESLOTS; i++) {
		if (!FREE_SKINCOLORS[i])
			break;
		if (fastcmp(upper_word, FREE_SKINCOLORS[i]))
			return SKINCOLOR_FIRSTFREESLOT+i;
	}
	for (i = 0; i < SKINCOLOR_FIRSTFREESLOT; i++)
		if (fastcmp(upper_word, COLOR_ENUMS[i]))
			return i;
    
	return 0;
}