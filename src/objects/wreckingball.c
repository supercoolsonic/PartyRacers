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
/// \file  wreckingball.c

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

#define WRECKINGB_MAXTURN (ANGLE_135)
#define WRECKINGB_TURNLERP (23)

#define wreckingball_speed(o) ((o)->movefactor)
#define wreckingball_selfdelay(o) ((o)->threshold)
#define wreckingball_droptime(o) ((o)->movecount)

#define wreckingball_turn(o) ((o)->extravalue1)

#define wreckingball_owner(o) ((o)->target)

enum {
	WBALL_DROPPED	= 0x01, // stationary hazard
	WBALL_TOSSED	= 0x02, // Gacha Bom tossed forward
	WBALL_TRAIL		= 0x04, // spawn afterimages
	WBALL_SPIN		= 0x08, // animate facing angle
	WBALL_WATERSKI	= 0x10, // this orbinaut can waterski
};

#define wreckingball_flags(o) ((o)->movedir)
#define wreckingball_spin(o) ((o)->extravalue2)
/*
struct Ball;

struct BChain : Mobj
{
	void hnext() = delete;
	BChain* next() const { return Mobj::hnext<BChain>(); }
	void next(BChain* n) { Mobj::hnext(n); }

	void target() = delete;
	//mobj_t* wball() const { return Mobj::target<Ball>(); }
	//void wball(mobj_t* n) { Mobj::target(n); }

	bool valid() const;

	//bool try_damage(Player* pmo);

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		return true;
	}
};
*/

void Obj_WreckingBallDeath(mobj_t *th)
{
	//P_SetMobjState(th, th->info->deathstate);
	
	th->momz = 0;
	
	for (INT32 i = 0; i < 16; ++i)
	{
		mobj_t* chunk = P_SpawnMobj(
			P_RandomRange(PR_ITEM_DEBRIS, (th->x - th->radius), (th->x + th->radius)), 
			P_RandomRange(PR_ITEM_DEBRIS, (th->y - th->radius), (th->y + th->radius)), 
			P_RandomRange(PR_ITEM_DEBRIS, (th->z + (1)), (th->z + (th->height*3/4))), 
		MT_GHZBALLCHUNK);
		
		K_FlipFromObject(chunk, th);
		
		chunk->momx = P_RandomRange(PR_ITEM_DEBRIS, -8, 8) * mapobjectscale;
		chunk->momy = P_RandomRange(PR_ITEM_DEBRIS, -8, 8) * mapobjectscale;
		chunk->momz = P_RandomRange(PR_ITEM_DEBRIS, 8, 20) * mapobjectscale * P_MobjFlip(chunk);
		chunk->scale = th->scale/2;
		chunk->fuse = 350;
	}
	
	/*mobj_t* splitball = P_SpawnMobj(th->x, th->y, th->z, MT_THOK);
	
	if (splitball)
	{
		P_SetMobjState(splitball, S_GHZBALLDEATHANIM);
		splitball->scale = th->scale;
	}*/
}

void Obj_WreckingBallThink(mobj_t *th)
{
	boolean grounded = P_IsObjectOnGround(th);
	
	if (th->state == &states[S_GHZBALLDEATHANIM])	//death blink
	{
		if (!(th->renderflags & RF_DONTDRAW))
			th->renderflags &= ~RF_DONTDRAW;	
		else
			th->renderflags |= RF_DONTDRAW;
		
		return;		//only blink, let states animate
	}

	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}
	
	if (wreckingball_flags(th) & WBALL_DROPPED)
	{
		if (grounded && (th->flags & MF_NOCLIPTHING))
		{
			th->momx = 10;
			th->momy = 0;
			//th->frame = 3;
			S_StartSound(th, th->info->activesound);
			th->flags &= ~MF_NOCLIPTHING;
		}
		else if (wreckingball_droptime(th))
		{
			wreckingball_droptime(th)--;
		}
		else if (th->frame < 3)
		{
			wreckingball_droptime(th) = 2;
			//th->frame++;
		}

		return;
	}
	

	if (wreckingball_flags(th) & WBALL_TRAIL)
	{
		mobj_t *ghost = NULL;

		ghost = P_SpawnGhostMobj(th);
		ghost->colorized = true; // already has color!
	}

	th->angle = K_MomentumAngle(th);
	if (wreckingball_turn(th) != 0)
	{
		th->angle += wreckingball_turn(th);

		if (abs(wreckingball_turn(th)) < WRECKINGB_MAXTURN)
		{
			if (wreckingball_turn(th) < 0)
			{
				wreckingball_turn(th) -= WRECKINGB_MAXTURN / WRECKINGB_TURNLERP;
			}
			else
			{
				wreckingball_turn(th) += WRECKINGB_MAXTURN / WRECKINGB_TURNLERP;
			}
		}
	}

	if (grounded == true)
	{
		fixed_t finalspeed = wreckingball_speed(th);
		const fixed_t currentspeed = R_PointToDist2(0, 0, th->momx, th->momy);
		fixed_t thrustamount = 0;
		fixed_t frictionsafety = (th->friction == 0) ? 1 : th->friction;

		if (th->health <= 10)
		{
			INT32 i;
			for (i = 10; i >= th->health; i--)
			{
				finalspeed = FixedMul(finalspeed, FRACUNIT-FRACUNIT/4);
			}
		}

		finalspeed = abs(finalspeed);

		if (currentspeed >= finalspeed)
		{
			// Thrust as if you were at top speed, slow down naturally
			thrustamount = FixedDiv(finalspeed, frictionsafety) - finalspeed;
		}
		else
		{
			const fixed_t beatfriction = FixedDiv(currentspeed, frictionsafety) - currentspeed;
			// Thrust to immediately get to top speed
			thrustamount = beatfriction + FixedDiv(finalspeed - currentspeed, frictionsafety);
		}

		P_Thrust(th, th->angle, thrustamount);
	}

	if (wreckingball_flags(th) & WBALL_SPIN)
	{
		th->angle = wreckingball_spin(th);
		wreckingball_spin(th) += ANGLE_112h;
	}

	/* todo: UDMFify
	if (P_MobjTouchingSectorSpecialFlag(th, ?))
	{
		K_DoPogoSpring(th, 0, 1);
	}
	*/

	if (wreckingball_selfdelay(th) > 0)
	{
		wreckingball_selfdelay(th)--;
	}

	if (leveltime % 6 == 0)
	{
		S_StartSound(th, th->info->activesound);
	}
	
	Obj_WreckingBallChainThink(th);
}

void Obj_WreckingBallChainThink(mobj_t *th)
{
	
}

void Obj_WreckingBallChunkThink(mobj_t *th)
{
	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}
		
	boolean onground = P_IsObjectOnGround(th);
	UINT8 bounces = 0;
	
	th->spritexscale = FRACUNIT;
	th->spriteyscale = FRACUNIT;	//brute forcing
	
	if (onground)
	{
		th->momz += (10*th->scale) * P_MobjFlip(th);
		//th->momz = FixedMul(th->lastmomz*-1, FRACUNIT*3/4);
		bounces++;
	}
	
	//th->lastmomz = th->momz;
	
	if (bounces >= 3)
		P_KillMobj(th, NULL, NULL, DMG_NORMAL);
	
}

void Obj_WreckingBallSpawnChain(mobj_t *th)
{
	/*BChain* link = nullptr;
	INT32 numLinks = 7;

	
	for (INT32 i = 0; i < numLinks; ++i)
	{
		*BChain node = P_SpawnMobj(th.x, th.y, th.z+th.height/2, MT_GHZBALLLINK)
		node->next(link);
		node->wball(th);
		link = node;
	}*/
}

void Obj_WreckingBallThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir)
{
	wreckingball_flags(th) = 0;

	if (wreckingball_owner(th) != NULL && P_MobjWasRemoved(wreckingball_owner(th)) == false
		&& wreckingball_owner(th)->player != NULL)
	{
		//th->color = wreckingball_owner(th)->player->skincolor;

		const mobj_t *owner = wreckingball_owner(th);
		const ffloor_t *rover = P_IsObjectFlipped(owner) ? owner->ceilingrover : owner->floorrover;

		if (dir >= 0 && rover && (rover->fofflags & FOF_SWIMMABLE))
		{
			// The owner can run on water, so we should too!
			wreckingball_flags(th) |= WBALL_WATERSKI;
		}
	}
	//else
	//{
		//th->color = SKINCOLOR_GREY;
	//}

	th->fuse = RR_PROJECTILE_FUSE;
	wreckingball_speed(th) = finalSpeed;

	wreckingball_flags(th) |= WBALL_TRAIL;

	if (dir < 0)
	{
		// Thrown backwards, init orbiting in place
		wreckingball_turn(th) = WRECKINGB_MAXTURN / WRECKINGB_TURNLERP;

		th->angle -= ANGLE_45;
		th->momx = FixedMul(finalSpeed, FINECOSINE(th->angle >> ANGLETOFINESHIFT));
		th->momy = FixedMul(finalSpeed, FINESINE(th->angle >> ANGLETOFINESHIFT));
	}
}

void Obj_WreckingBallThrown_Pre(mobj_t *th, fixed_t finalSpeed, fixed_t dir)
{
	Obj_WreckingBallThrown(th, finalSpeed, dir);
	Obj_WreckingBallSpawnChain(th);
	//th->state = &states[S_GHZBALL];
	th->scale = th->scale * 2;

	wreckingball_flags(th) &= ~(WBALL_TRAIL);

	if (dir < 0)
	{
		wreckingball_flags(th) |= WBALL_SPIN;
	}
	else if (dir > 0)
	{
		wreckingball_flags(th) |= WBALL_TOSSED;
	}
}

boolean Obj_WreckingBallWasTossed(mobj_t *th)
{
	return (wreckingball_flags(th) & WBALL_TOSSED) == WBALL_TOSSED;
}

void Obj_WreckingBallDrop(mobj_t *th)
{
	wreckingball_flags(th) |= WBALL_DROPPED;
}

boolean Obj_WreckingBallCanRunOnWater(mobj_t *th)
{
	return (wreckingball_flags(th) & WBALL_WATERSKI) == WBALL_WATERSKI;
}
