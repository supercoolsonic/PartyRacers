// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  jawz.c
/// \brief Afterburner Jawz item code.

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

#define MAX_ABJAWZ_TURN (ANGLE_90 / 15) // We can turn a maximum of 6 degrees per frame at regular max speed

#define abjawz_speed(o) ((o)->movefactor)
#define abjawz_selfdelay(o) ((o)->threshold)
#define abjawz_dropped(o) ((o)->flags2 & MF2_AMBUSH)
#define abjawz_droptime(o) ((o)->movecount)

#define abjawz_retcolor(o) ((o)->extravalue2)
#define abjawz_stillturn(o) ((o)->cusval)

#define abjawz_owner(o) ((o)->target)
#define abjawz_chase(o) ((o)->tracer)

static void ABJawzChase(mobj_t *th, dboolean grounded)
{
	fixed_t thrustamount = 0;
	fixed_t frictionsafety = (th->friction == 0) ? 1 : th->friction;
	fixed_t topspeed = abjawz_speed(th);

	if (abjawz_chase(th) != NULL && P_MobjWasRemoved(abjawz_chase(th)) == false)
	{
		if (abjawz_chase(th)->health > 0)
		{
			const angle_t targetangle = R_PointToAngle2(
				th->x, th->y,
				abjawz_chase(th)->x, abjawz_chase(th)->y
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
					abjawz_chase(th)->x - th->x,
					abjawz_chase(th)->y - th->y
				);

				if (distaway < distbarrier)
				{
					if (abjawz_chase(th)->player != NULL)
					{
						fixed_t speeddifference = abs(
							topspeed - min(
								abjawz_chase(th)->player->speed,
								(K_GetKartSpeed(abjawz_chase(th)->player, false, false))/2
							)
						);

						topspeed = topspeed - FixedMul(speeddifference, FRACUNIT - FixedDiv(distaway, distbarrier));
					}
				}
			}

			if (angledelta != 0)
			{
				angle_t turnSpeed = MAX_ABJAWZ_TURN;
				dboolean turnclockwise = true;

				// MAX_JAWZ_TURN gets stronger the slower the top speed of jawz
				if (topspeed < abjawz_speed(th))
				{
					if (topspeed == 0)
					{
						turnSpeed = ANGLE_180;
					}
					else
					{
						fixed_t anglemultiplier = FixedDiv(abjawz_speed(th), topspeed);
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

			ret = P_SpawnMobjFromMobj(abjawz_chase(th), 0, 0, 0, MT_ABURNERRETICLE);
			ret->old_x = abjawz_chase(th)->old_x;
			ret->old_y = abjawz_chase(th)->old_y;
			ret->old_z = abjawz_chase(th)->old_z;
			P_SetTarget(&ret->target, abjawz_chase(th));
			ret->frame |= ((leveltime % 10) / 2) + 5;
			ret->color = abjawz_retcolor(th);
			ret->renderflags = (ret->renderflags & ~RF_DONTDRAW) | (th->renderflags & RF_DONTDRAW);
			ret->hitlag = 0; // spawns every tic, so don't inherit player hitlag
		}
		else
		{
			P_SetTarget(&abjawz_chase(th), NULL);
		}
	}

	if (abjawz_chase(th) == NULL || P_MobjWasRemoved(abjawz_chase(th)) == true)
	{
		mobj_t *newChase = NULL;
		player_t *owner = NULL;

		th->angle = K_MomentumAngle(th);

		if ((abjawz_owner(th) != NULL && P_MobjWasRemoved(abjawz_owner(th)) == false)
			&& (abjawz_owner(th)->player != NULL))
		{
			owner = abjawz_owner(th)->player;
		}

		newChase = K_FindJawzTarget(th, owner, ANGLE_90);
		if (newChase != NULL)
		{
			P_SetTarget(&abjawz_chase(th), newChase);
		}
	}

	if (abjawz_stillturn(th) > 0)
	{
		// When beginning to chase your own owner,
		// we should turn but not thrust quite yet.
		return;
	}

	if (grounded == true)
	{
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
		
		if (abjawz_selfdelay(th) <= 0)
			thrustamount *= 3;
		else
			thrustamount /= 5;
		
		//if (thrustamount <= FRACUNIT/2)
			//thrustamount = 2*FRACUNIT;

		P_Thrust(th, th->angle, thrustamount);
	}
}

static dboolean ABJawzSteersBetter(void)
{
	return !!!(gametyperules & GTR_CIRCUIT);
}

void Obj_ABJawzThink(mobj_t *th)
{
	
	dboolean grounded = P_IsObjectOnGround(th);

	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}

	if (abjawz_dropped(th))
	{
		if (grounded && (th->flags & MF_NOCLIPTHING))
		{
			th->momx = 1;
			th->momy = 0;
			S_StartSound(th, th->info->deathsound);
			th->flags &= ~MF_NOCLIPTHING;
		}

		return;
	}

	if (abjawz_owner(th) != NULL && P_MobjWasRemoved(abjawz_owner(th)) == false
		&& abjawz_owner(th)->player != NULL && abjawz_selfdelay(th) < 0)
	{
		mobj_t *ghost = P_SpawnGhostMobj(th);
		ghost->color = abjawz_owner(th)->player->skincolor;
		ghost->colorized = true;
	}

	if (!abjawz_stillturn(th) && (th->momx != 0 && th->momy != 0))		//SCS EDIT - improving accuracy here
	{
		if (ABJawzSteersBetter() == true)
			th->friction = max(0, 3 * th->friction / 4);
		else if (abjawz_selfdelay(th) > 0)
			th->friction = max(0, 4 * th->friction / 5);
		else
			th->friction = max(0, 6 * th->friction / 7);
		
	}

	ABJawzChase(th, grounded);
	K_DriftDustHandling(th);

	/* todo: UDMFify
	if (P_MobjTouchingSectorSpecialFlag(th, ?))
	{
		K_DoPogoSpring(th, 0, 1);
	}
	*/
	
	if (th->state == &states[S_AFTERBURNERJAWZ_IDLE])
		P_SetMobjState(th, S_AFTERBURNERJAWZ_A);
		//th->state = &states[S_AFTERBURNERJAWZ_A];

	if (abjawz_selfdelay(th) >= 0)
	{
		abjawz_selfdelay(th)--;
		
		if (abjawz_selfdelay(th) == 5)
		{
			//th->state = &states[S_AFTERBURNERJAWZ_JUMP];
			P_SetMobjState(th, S_AFTERBURNERJAWZ_JUMP);
			//CONS_Printf("JUMP!!!!\n");
			
			mobj_t *booster = P_SpawnMobj(th->x, th->y, th->z, MT_ABJZBOOSTER);
			
			if (booster)
			{
				booster->target = th;
				booster->scale = th->scale;
			}
			
			S_StartSound(th, sfx_s1bc);	
		}
		else if (abjawz_selfdelay(th) == 0)
		{
			//th->state = &states[S_AFTERBURNERJAWZ_IDLE];
			P_SetMobjState(th, S_AFTERBURNERJAWZ_IDLE);
			//CONS_Printf("FLY!!!!\n");
		}			
	}

	if (abjawz_stillturn(th) > 0)
	{
		abjawz_stillturn(th)--;
	}

	if (leveltime % TICRATE == 0)
	{
		S_StartSound(th, th->info->activesound);
	}
	/*
	if (th->state == &states[S_AFTERBURNERJAWZ_IDLE])
		CONS_Printf("We are idle\n");
	else if (th->state == &states[S_AFTERBURNERJAWZ_WHEEL])
		CONS_Printf("We are walking\n");
	else if (th->state == &states[S_AFTERBURNERJAWZ_JUMP])
		CONS_Printf("We are jumping\n");
*/
}

void Obj_ABJawzBoosterThink(mobj_t *th)
{
	if (th->target)
	{
		angle_t newangle = th->target->angle + th->angle;
		
		uint32_t newx = th->target->x - th->target->momx - (FixedMul(th->target->scale, th->scale), FINECOSINE((newangle) >> ANGLETOFINESHIFT));

		uint32_t newy = th->target->y - th->target->momy - (FixedMul(th->target->scale, th->scale), FINESINE((newangle) >> ANGLETOFINESHIFT));
				
		th->momx = 0;
		th->momy = 0;
		th->momz = 0;
		th->angle = th->target->angle;
		
		//if (K_GetForwardMove(th->target->player) < 0)
		//{
			//th->angle = th->target->angle + ANGLE_180;
		//}	
		
		P_SetOrigin(th, newx, newy, th->target->z);
	}
	else
		P_RemoveMobj(th);
	
}

void Obj_ABJawzThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir)
{
	int32_t lastTarg = -1;
	player_t *owner = NULL;

	if (abjawz_owner(th) != NULL && P_MobjWasRemoved(abjawz_owner(th)) == false
		&& abjawz_owner(th)->player != NULL)
	{
		lastTarg = abjawz_owner(th)->player->lastabjawztarget;
		abjawz_retcolor(th) = abjawz_owner(th)->player->skincolor;
		owner = abjawz_owner(th)->player;
	}
	else
	{
		abjawz_retcolor(th) = SKINCOLOR_KETCHUP;
	}

	if (dir < 0)
	{
		// Thrown backwards, init self-chase
		P_SetTarget(&abjawz_chase(th), abjawz_owner(th));

		// Stop it here.
		th->momx = 0;
		th->momy = 0;

		// Return at absolutely 120% of the owner's speed if it's any less than that.
		fixed_t min_backthrowspeed = 6*(K_GetKartSpeed(owner, false, false))/5;
		if (owner->speed >= min_backthrowspeed)
		{
			finalSpeed = 6*(owner->speed)/5;
		}
		else
		{
			finalSpeed = min_backthrowspeed; 
		}

		// Set a fuse.
		th->fuse = RR_PROJECTILE_FUSE;

		// Stay still while you turn towards the player
		abjawz_stillturn(th) = ANGLE_180 / MAX_ABJAWZ_TURN;
	}
	else
	{
		if ((lastTarg >= 0 && lastTarg < MAXPLAYERS)
			&& playeringame[lastTarg] == true)
		{
			player_t *tryPlayer = &players[lastTarg];

			if (tryPlayer->spectator == false)
			{
				P_SetTarget(&abjawz_chase(th), tryPlayer->mo);	
			}
		}
		
		th->destscale = th->scale*(3/2);

		// Sealed Star: target the UFO immediately. I don't
		// wanna fuck with the lastjawztarget stuff, so just
		// do this if a target wasn't set.
		if (abjawz_chase(th) == NULL || P_MobjWasRemoved(abjawz_chase(th)) == true)
		{
			P_SetTarget(&abjawz_chase(th), K_FindJawzTarget(th, owner, ANGLE_90));
		}
	}
	
	th->destscale = th->destscale*2;
	
	abjawz_selfdelay(th) = 22;

	S_StartSound(th, th->info->activesound);
	abjawz_speed(th) = finalSpeed;
}
