// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  megachopper.c

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"
#include "../k_collide.h"
#include "../m_fixed.h"
#include "../m_easing.h"
#include "../p_spec.h"
#include "../k_collide.h"

void Obj_MegaChopperDeath(mobj_t *th)
{
	P_SetMobjState(th, th->info->deathstate);
	
	P_SetObjectMomZ(th, 8*FRACUNIT, false);
	
	if (th->target)
		th->target->player->megachopper = NULL;
	
	P_InstaThrust(th, P_RandomRange(PR_DECORATION, 1, 360)*ANG1, 16*FRACUNIT);
	th->fuse = 15;
}

void Obj_MegaChopperThink(mobj_t *th)
{	
	if (th->state == &states[S_MEGACHOPPERDEAD])	//death blink
	{
		if (!(th->renderflags & RF_DONTDRAW))
			th->renderflags &= ~RF_DONTDRAW;	
		else
			th->renderflags |= RF_DONTDRAW;
		
		th->fuse--;
		return;		//only blink, let states animate
	}

	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}
	
	if (R_PointToDist2(th->target->x, th->target->y, th->x, th->y) > FRACUNIT*50)	//If the fish gets too far away, just teleport it back.
	{
		th->x = th->target->x;
		th->y = th->target->y;
		th->z = th->target->z;
	}
	
	angle_t newangle = th->target->angle + th->angle;
	
	uint32_t newx = th->target->x + th->target->momx + (FixedMul(th->target->scale, th->scale), FINECOSINE((newangle) >> ANGLETOFINESHIFT));

	uint32_t newy = th->target->y + th->target->momy + (FixedMul(th->target->scale, th->scale), FINESINE((newangle) >> ANGLETOFINESHIFT));
			
	th->momx = 0;
	th->momy = 0;
	th->momz = 0;
	th->angle = th->target->angle;
	
	if (K_GetForwardMove(th->target->player) < 0)
	{
		th->angle = th->target->angle + ANGLE_180;
	}	
	
	//P_SetOrigin(th, newx, newy, th->target->z);
	P_MoveOrigin(th, newx, newy, th->target->z);

	if (leveltime % 6 == 0)
	{
		S_StartSound(th, th->info->activesound);
	}
}