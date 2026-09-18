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

void Obj_WreckingBallDeath(mobj_t *th, dboolean ismini)
{
	th->momz = 0;
	
	if (!ismini)
	{
		for (int32_t i = 0; i < 16; ++i)
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
	}
	
	if (th->hnext != NULL && !ismini)
		Obj_WreckingBallBreakChain(th->hnext);	//Starts from the end of the chain
	
}

void Obj_WreckingBallThink(mobj_t *th, dboolean ismini)
{
	dboolean grounded = P_IsObjectOnGround(th);
	
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

		if (th->health < 10)
		{
			/*if (ismini)
			{
				int32_t i;
				for (i = 9; i >= th->health; i--)
				{
					if (i < 7)
						break;
					
					finalspeed = FixedMul(finalspeed, FRACUNIT-FRACUNIT/4);
				}
			}
			else*/
			
			if (!ismini)
				finalspeed = FixedMul(finalspeed, FRACUNIT-FRACUNIT/4);
			
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
		wreckingball_spin(th) += ANG1;//ANGLE_112h;
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

	th->extravalue1 = FixedMul(mobjinfo[ismini ? MT_GHZBALLMINILINK : MT_GHZBALLLINK].height*P_MobjFlip(th), th->scale);
		
	if (!ismini && th->hnext != NULL)
		Obj_WreckingBallChainThink(th->hnext, (th->hnext)->extravalue1);
	
}

void Obj_WreckingBallChainThink(mobj_t *th, dboolean first)
{
	//Ashnal code, hi Ashnal! :D 0/ You are good at math!
	if (!P_MobjWasRemoved(th) && th->target != NULL)
	{
		//CONS_Printf("We are link %d\n", th->extravalue2);
		//if (first)
			//CONS_Printf("We are the first link!!!\n");
		
		angle_t angletomovetowards = R_PointToAngle2(th->x, th->y, th->target->x, th->target->y);		//th->target is referencing the link's previous link in the chain, so checking backwards from the end to the start
		fixed_t firstdist = first ? FixedMul(40*FRACUNIT, th->target->scale) : 0;				//HOWEVER, for the first link in the chain, target is the BALL, not a chain link!
		fixed_t firstz = first ? th->target->height/2 : 0;
		fixed_t dist = R_PointToDist2(th->x, th->y, th->target->x, th->target->y) - FixedMul(22*FRACUNIT, th->scale) - FixedMul(firstdist, th->scale);
		th->momx = P_ReturnThrustX(th, angletomovetowards, dist);
		th->momy = P_ReturnThrustY(th, angletomovetowards, dist);
		//print("z "..mo.z.." momz "..(mo.target.z+mo.target.height/2).." - "..mo.z.." - "..mo.tracer.zadjust.." = "..(mo.target.z+mo.target.height/2 - mo.z))
		//th->momz = FixedMul(th->target->z+firstz - th->z - (12*FRACUNIT*P_MobjFlip(th->tracer))/th->extravalue2, FRACUNIT/2);
		th->momz = FixedMul(th->target->z+firstz - th->z - th->tracer->extravalue1, FRACUNIT/2);
		
		//CONS_Printf("Our momz is %d\n", th->momz);
		//CONS_Printf("Our offset is %d\n", th->tracer->extravalue1);
		//CONS_Printf("----------------\n");
		
		//th->tracer->extravalue1 = (th->tracer->extravalue1*8)/9;		//Nah, not gonna use this. Looks best when it doesn't get adjusted
		th->fuse = 350;
		
		if (th->target->type != MT_GHZBALL)
			Obj_WreckingBallChainThink(th->target, th->target->extravalue1);
		
	}
	else
	{
		if (th->flags & MF_NOCLIP)
			th->flags |= MF_NOCLIP;
		
		Obj_WreckingBallChunkThink(th);
	}
	
}

void Obj_WreckingBallChainMiniThink(mobj_t *th, dboolean first, fixed_t curx, fixed_t cury)
{
	if (th->type == MT_PLAYER)
		return;
	
	if (!P_MobjWasRemoved(th) && th->target != NULL && th->tracer != NULL)
	{
		//CONS_Printf("curx = %d\n", curx);
		//CONS_Printf("cury = %d\n", cury);

		curx += (th->tracer->x - th->tracer->target->x) / 4;//stepx;
		cury += (th->tracer->y - th->tracer->target->y) / 4;//stepy;
		
		//angle_t angletomovetowards = R_PointToAngle2(th->x, th->y, th->target->x, th->target->y);		//th->target is referencing the link's previous link in the chain, so checking backwards from the end to the start
		//fixed_t firstdist = first ? FixedMul(15*FRACUNIT, th->target->scale) : 0;														//HOWEVER, for the first link in the chain, target is the BALL, not a chain link!
		fixed_t firstz = th->tracer->height/6;//first ? th->target->height/5 : 0;
		
		if (th->extravalue2 == 3)
			firstz = th->tracer->target->height/4;
		
		//fixed_t dist = R_PointToDist2(th->x, th->y, th->target->x, th->target->y) - FixedMul(11*FRACUNIT, th->scale) - FixedMul(firstdist, th->scale);
		//th->momx = P_ReturnThrustX(th, angletomovetowards, dist);
		//th->momy = P_ReturnThrustY(th, angletomovetowards, dist);
		th->x = curx;///FRACUNIT;
		th->y = cury;///FRACUNIT;
		th->angle = th->tracer->angle;
		
		if (first)
			th->z = th->tracer->z+firstz;		//don't use th->tracer->target here because it needs to align with the ball
		else if (th->extravalue2 == 3)
			th->z = th->tracer->target->z+firstz;
		else
		{
			if (th->target->z > th->z)
				th->z = th->tracer->z+(firstz/2);
			else if (th->tracer->target->z < th->z)
				th->z = th->tracer->target->z+(firstz/2);
			else
				th->z = th->target->z;
			
			//th->z = ((th->target->z)-(th->tracer->target->z))+firstz;
			//th->momz = 0;
		}
		
		//else		
		//	th->momz = FixedMul(th->target->z+firstz - th->z - 1, FRACUNIT/2);						//th->target is prev link, which can be the ball, th->tracer IS the ball, th->tracer->target is the player
		//th->z = th->tracer->z+firstz;
		/*else
		{
			th->z = (th->tracer->z + (th->tracer->height/2));
			//th->momz = 0;
		}*/
		
		//CONS_Printf("Our momz is %d\n", th->momz);
		//CONS_Printf("Our offset is %d\n", th->tracer->extravalue1);     
		//CONS_Printf("----------------\n");
		
		//th->tracer->extravalue1 = (th->tracer->extravalue1*8)/9;		//Nah, not gonna use this. Looks best when it doesn't get adjusted
		th->fuse = 165;
		
		if (th->target->type != MT_GHZBALLMINI)
			Obj_WreckingBallChainMiniThink(th->target, th->target->extravalue1, curx, cury);
		
	}
	else
		Obj_WreckingBallChunkThink(th);
	
}

void Obj_WreckingBallChunkThink(mobj_t *th)
{
	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}
		
	dboolean onground = P_IsObjectOnGround(th);
	//uint8_t bounces = 0;
	
	//th->spritexscale = FRACUNIT;
	//th->spriteyscale = FRACUNIT;	//brute forcing
	
	if (onground)
	{
		th->momz += (10*th->scale) * P_MobjFlip(th);
		//th->momz = FixedMul(th->lastmomz*-1, FRACUNIT*3/4);
		//bounces++;
	}
	
	//th->lastmomz = th->momz;
	
	//if (bounces >= 3)
		//P_KillMobj(th, NULL, NULL, DMG_NORMAL);
	
}

void Obj_WreckingBallBreakChain(mobj_t *th)
{
	if (th->target != NULL)		//Is our connected link valid?
	{
		if (th->target->type != MT_GHZBALL && th->target->type != MT_GHZBALLMINI)	//Is our connection the ball, or a link? If it's a link, run the break code on it too so it can bounce around
			Obj_WreckingBallBreakChain(th->target);
		
		th->target = NULL;		//This should set the chain links to bounce around instead of trying to follow the ball around
	}
}

void Obj_WreckingBallSpawnChain(mobj_t *th)
{
	mobj_t *previous = th;
	dboolean first = true;
	uint8_t i;
	
	for (i = 0; i < 7; i++)		//Based on Ashnal's code! :D Ashnal, you are teh smarts!
	{
		mobj_t *link = P_SpawnMobj(th->x, th->y, th->z+(th->height/2), MT_GHZBALLLINK);
		
		if (link)
		{
			link->target = previous;	//The previous link in the chain - it's the ball itself if this is the first link
			link->tracer = th;			//The reference to the ball, all links have this
			link->scale = th->scale;
			link->destscale = th->destscale*2;
			link->extravalue1 = first;	//Determines if it's the first one in the chain
			link->extravalue2 = (i+1);	//keeps track of which link it is in the chain
			previous = link;			//Previous is now set to the current link, so the next link can keep a reference to it
			th->hnext = link;		//This is a reference the ball keeps, which is the LAST link in the chain
			first = false;
		}
		else
			return;
		
	}
	
	/*BChain* link = nullptr;
	int32_t numLinks = 7;

	
	for (int32_t i = 0; i < numLinks; ++i)
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

void Obj_WreckingBallThrown_Pre(mobj_t *th, fixed_t finalSpeed, fixed_t dir, dboolean ismini)
{
	Obj_WreckingBallThrown(th, finalSpeed, dir);
	
	if (!ismini)
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

dboolean Obj_WreckingBallWasTossed(mobj_t *th)
{
	return (wreckingball_flags(th) & WBALL_TOSSED) == WBALL_TOSSED;
}

void Obj_WreckingBallDrop(mobj_t *th)
{
	wreckingball_flags(th) |= WBALL_DROPPED;
}

dboolean Obj_WreckingBallCanRunOnWater(mobj_t *th)
{
	return (wreckingball_flags(th) & WBALL_WATERSKI) == WBALL_WATERSKI;
}
