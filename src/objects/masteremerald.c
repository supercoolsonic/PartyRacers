// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
// PORTED TO RR BY SCS
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  masteremerald.c

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
#include "../d_player.h"

fixed_t cSpd(fixed_t ang, fixed_t spd, mobj_t *th, dboolean Cosine)
{
	spd = FixedMul(spd, th->scale);
	
	if (Cosine)
		return FixedMul(cos(ang)*(FRACUNIT*2), spd);
	else
		return FixedMul(sin(ang)*(FRACUNIT*2), spd);
}

void EM_OrbitThinker(mobj_t *th)
{
	if (th->target)
	{	
		th->frame = ((leveltime/3)%6)|FF_FULLBRIGHT;
		mobj_t *t = th->target;
		uint32_t offset = t->height/2;
		uint32_t thisAng = leveltime*(ANG2);
		fixed_t x = cSpd(thisAng+360, (th->fuse*2)*mapobjectscale, t, true);
		fixed_t y = cSpd(thisAng+360, (th->fuse*2)*mapobjectscale, t, false);
		P_MoveOrigin(th, t->x+x, t->y+y, t->z + offset);
	}
	else
	{
		P_RemoveMobj(th);
	}
	
	if (th->target && th->target->player)
	{
		if (th->fuse == 1 && th->target->player->orbitmasteremerald == th)
		{
			//th->target->player->invincbilitytimer = th->target->player->rings;
			K_DoInvincibility(th->target->player, th->target->player->rings);
			
			K_PlayPowerGloatSound(th->target->player->mo);
			
			mobj_t *beam = P_SpawnMobj(th->target->player->mo->x, th->target->player->mo->y, th->target->player->mo->z, MT_XEMBEAM);
			
			if (beam)
			{
				beam->scale = 3*mapobjectscale;
				beam->target = th->target->player->mo;
				beam->color = th->target->player->skincolor;
				beam->fuse = 40;
				S_StartSound(th->target->player->mo, sfx_s3k9f);
				th->target->player->orbitmasteremerald = NULL;
			}
		}
	}
}

void EM_BeamThinker(mobj_t *th)
{
	if (!th) return;
	
	if (th->target)
		P_MoveOrigin(th, th->target->x, th->target->y, th->target->z);
	
}
