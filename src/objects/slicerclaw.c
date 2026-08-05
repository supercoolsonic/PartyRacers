// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  slicerclaw.c
/// \brief Slicer Claw item code. (Based on jawz lol)

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
#include "../k_specialstage.h"

#define MAX_SLICER_TURN (ANGLE_90 / 15) // We can turn a maximum of 6 degrees per frame at regular max speed

#define slicer_speed(o) ((o)->movefactor)
#define slicer_selfdelay(o) ((o)->threshold)
#define slicer_dropped(o) ((o)->flags2 & MF2_AMBUSH)
#define slicer_droptime(o) ((o)->movecount)

#define slicer_retcolor(o) ((o)->extravalue2)
#define slicer_stillturn(o) ((o)->cusval)

#define slicer_owner(o) ((o)->target)

static void SlicerChase(mobj_t *th)
{
	fixed_t thrustamount = 0;
	fixed_t frictionsafety = (th->friction == 0) ? 1 : th->friction;
	fixed_t topspeed = slicer_speed(th);

	if (slicer_owner(th) != NULL && P_MobjWasRemoved(slicer_owner(th)) == false)
	{
		if (slicer_owner(th)->health > 0)
		{
			const angle_t targetangle = R_PointToAngle2(
				th->x, th->y,
				slicer_owner(th)->x, slicer_owner(th)->y
			);
			angle_t angledelta = th->angle - targetangle;
			mobj_t *ret = NULL;

			if (gametyperules & GTR_CIRCUIT)
			{
				const fixed_t distbarrier = FixedMul(
					512 * mapobjectscale,
					FRACUNIT + ((gamespeed-1) * (FRACUNIT/4))
				);

				const fixed_t distaway = P_AproxDistance(
					slicer_owner(th)->x - th->x,
					slicer_owner(th)->y - th->y
				);

				if (distaway < distbarrier)
				{
					if (slicer_owner(th)->player != NULL)
					{
						fixed_t speeddifference = abs(
							topspeed - min(
								slicer_owner(th)->player->speed,
								(K_GetKartSpeed(slicer_owner(th)->player, false, false))/2
							)
						);

						topspeed = topspeed - FixedMul(speeddifference, FRACUNIT - FixedDiv(distaway, distbarrier));
					}
				}
			}

			if (angledelta != 0)
			{
				angle_t turnSpeed = MAX_SLICER_TURN;
				boolean turnclockwise = true;

				// MAX_JAWZ_TURN gets stronger the slower the top speed of jawz
				if (topspeed < slicer_speed(th))
				{
					if (topspeed == 0)
					{
						turnSpeed = ANGLE_180;
					}
					else
					{
						fixed_t anglemultiplier = FixedDiv(slicer_speed(th), topspeed);
						turnSpeed += FixedAngle(FixedMul(AngleFixed(turnSpeed), anglemultiplier));
					}
				}

				if (angledelta > ANGLE_180)
				{
					angledelta = InvAngle(angledelta);
					turnclockwise = false;
				}

				if (angledelta > turnSpeed)
				{
					angledelta = turnSpeed;
				}

				if (turnclockwise == true)
				{
					th->angle -= angledelta;
				}
				else
				{
					th->angle += angledelta;
				}
			}
		}
		else
		{
			P_SetTarget(&slicer_owner(th), NULL);
		}
	}

	if (slicer_stillturn(th) > 0)
	{
		// When beginning to chase your own owner,
		// we should turn but not thrust quite yet.
		return;
	}

	const fixed_t currentspeed = R_PointToDist2(0, 0, th->momx, th->momy);

	if (currentspeed >= topspeed)
	{
		// Thrust as if you were at top speed, slow down naturally
		thrustamount = FixedDiv(topspeed, frictionsafety) - topspeed;
	}
	else
	{
		const fixed_t beatfriction = FixedDiv(currentspeed, frictionsafety) - currentspeed;
		// Thrust to immediately get to top speed
		thrustamount = beatfriction + FixedDiv(topspeed - currentspeed, frictionsafety);
	}
		
	if (slicer_selfdelay(th) <= 0)	//come out fast and return to the owner more slowly
		thrustamount *= 2;
	else
		thrustamount *= 6;

	P_Thrust(th, th->angle, thrustamount);
}

static boolean SlicerSteersBetter(void)
{
	return !!!(gametyperules & GTR_CIRCUIT);
}

void Obj_SlicerThink(mobj_t *th)
{
	boolean grounded = P_IsObjectOnGround(th);

	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}
	
	if (th->extravalue1 > 0)
		th->extravalue1--;
	
	//th->momz = 0;

	if (slicer_owner(th) != NULL && P_MobjWasRemoved(slicer_owner(th)) == false && slicer_owner(th)->player != NULL)
	{
		mobj_t *ghost = P_SpawnGhostMobj(th);
		ghost->color = slicer_owner(th)->player->skincolor;
		ghost->colorized = true;
	}

	if (!slicer_stillturn(th) && (th->momx != 0 && th->momy != 0))
	{
		if (SlicerSteersBetter() == true)
			th->friction = max(0, 3 * th->friction / 4);
		else if (slicer_selfdelay(th) > 0)
			th->friction = max(0, 4 * th->friction / 5);
		else
			th->friction = max(0, 8 * th->friction / 9);
		
	}

	SlicerChase(th);

	if (slicer_selfdelay(th) >= 0)
	{
		slicer_selfdelay(th)--;	
	}

	if (slicer_stillturn(th) > 0)
	{
		slicer_stillturn(th)--;
	}

	if (leveltime % TICRATE == 0)
	{
		S_StartSound(th, th->info->activesound);
	}
}

void Obj_SlicerThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir)
{
	// Thrown backwards, init self-chase
	//P_SetTarget(&th->target, slicer_owner(th));

	// Stop it here.
	//th->momx = 0;
	//th->momy = 0;
	
	// Return at absolutely 120% of the owner's speed if it's any less than that.
	fixed_t min_backthrowspeed = 6*(K_GetKartSpeed(slicer_owner(th)->player, false, false))/5;
	if (slicer_owner(th)->player->speed >= min_backthrowspeed)
	{
		finalSpeed = 6*(slicer_owner(th)->player->speed)/5;
	}
	else
	{
		finalSpeed = min_backthrowspeed; 
	}

	// Set a fuse.
	th->fuse = RR_PROJECTILE_FUSE;

	// Stay still while you turn towards the player
	slicer_stillturn(th) = ANGLE_180 / MAX_SLICER_TURN;
	
	if (dir >= 0)
		th->destscale = th->scale*(3/2);
		
	/*
	if (dir < 0)
	{
		// Return at absolutely 120% of the owner's speed if it's any less than that.
		fixed_t min_backthrowspeed = 6*(K_GetKartSpeed(owner, false, false))/5;
		if (slicer_owner(th)->speed >= min_backthrowspeed)
		{
			finalSpeed = 6*(slicer_owner(th)->speed)/5;
		}
		else
		{
			finalSpeed = min_backthrowspeed; 
		}

		// Set a fuse.
		th->fuse = RR_PROJECTILE_FUSE;

		// Stay still while you turn towards the player
		slicer_stillturn(th) = ANGLE_180 / MAX_SLICER_TURN;
	}
	else
	{
		if ((lastTarg >= 0 && lastTarg < MAXPLAYERS) && playeringame[lastTarg] == true)
		{
			player_t *tryPlayer = &players[lastTarg];

			if (tryPlayer->spectator == false)
			{
				P_SetTarget(&slicer_chase(th), tryPlayer->mo);	
			}
		}
		
		th->destscale = th->scale*(3/2);	//I'll keep this. Give it a bigger size when thrown forward, and let it be smaller/normal size when thrown backwards, as some balancing
	}*/
	
	th->destscale = th->destscale*2;
	
	slicer_selfdelay(th) = 20;

	S_StartSound(th, th->info->activesound);
	slicer_speed(th) = finalSpeed;
}
