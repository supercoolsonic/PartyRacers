// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hyudoro.c
/// \brief Hyudoro item code.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../k_roulette.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../k_hitlag.h"
#include "../p_slopes.h"
#include "../k_hud.h" // K_AddMessage   SCS ADD
//#include "../k_kart.h"				  //SCS ADD

// Radio
#include "../radioracers/rr_hud.h"				//SCS - RADIO

enum {
	HYU_PATROL,
	HYU_RETURN,
	HYU_HOVER,
	HYU_ORBIT,
	HYU_VANISH								//SCS ADD
};

// TODO: make these general functions

static fixed_t
K_GetSpeed (mobj_t *mobj)
{
	return FixedHypot(mobj->momx, mobj->momy);
}

#define hyudoro_mode(o) ((o)->extravalue1)
#define hyudoro_itemtype(o) ((o)->movefactor)
#define hyudoro_itemcount(o) ((o)->movecount)
#define hyudoro_hover_stack(o) ((o)->threshold)
#define hyudoro_next(o) ((o)->tracer)
#define hyudoro_stackpos(o) ((o)->reactiontime)

// cannot be combined
#define hyudoro_center(o) ((o)->target)
#define hyudoro_target(o) ((o)->target)

#define hyudoro_stolefrom(o) ((o)->hnext)
#define hyudoro_capsule(o) ((o)->hprev)
#define hyudoro_timer(o) ((o)->movedir)

#define hyudoro_center_max_radius(o) ((o)->threshold)
#define hyudoro_center_master(o) ((o)->target)

#define HYU_VISUAL_HEIGHT (24)

static angle_t
trace_angle (mobj_t *hyu)
{
	mobj_t *center = hyu->target;

	if (hyu->x != center->x || hyu->y != center->y)
	{
		return R_PointToAngle2(
				center->x, center->y, hyu->x, hyu->y);
	}
	else
		return hyu->angle;
}

static angle_t
get_look_angle (mobj_t *thing)
{
	player_t *player = thing->player;

	return player ? player->angleturn : thing->angle;
}

static boolean
is_hyudoro (mobj_t *thing)
{
	return !P_MobjWasRemoved(thing) &&
		(thing->type == MT_HYUDORO || thing->type == MT_BHYUDORO);		//SCS EDIT
}

static mobj_t *
get_hyudoro_master (mobj_t *hyu)
{
	if (hyu->type == MT_PPOCKETHYUDORO || hyu->type == MT_MINIHYUDORO)				//SCS ADD
		return hyu->type == MT_MINIHYUDORO ? hyu->target->target : hyu->target;
	
	mobj_t *center = hyudoro_center(hyu);

	return center ? hyudoro_center_master(center) : NULL;
}

static player_t *
get_hyudoro_target_player (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	return target ? target->player : NULL;
}

static void
sine_bob
(		mobj_t * hyu,
		angle_t a,
		fixed_t sineofs)
{
	hyu->sprzoff = FixedMul(HYU_VISUAL_HEIGHT * hyu->scale,
			sineofs + FINESINE(a >> ANGLETOFINESHIFT)) * P_MobjFlip(hyu);
			
	if (P_IsObjectFlipped(hyu))
		hyu->sprzoff -= hyu->height;
}

static void
bob_in_place
(		mobj_t * hyu,
		INT32 bob_speed)
{
	sine_bob(hyu,
			(leveltime & (bob_speed - 1)) *
			(ANGLE_MAX / bob_speed), -(3*FRACUNIT/4));
}

static void
reset_shadow (mobj_t *hyu)
{
	hyu->shadowcolor = 15;
	hyu->whiteshadow = true;
}

static void
project_hyudoro (mobj_t *hyu)
{
	mobj_t *center = hyudoro_center(hyu);

	angle_t angleStep = FixedMul(5 * ANG1,
			FixedDiv(hyudoro_center_max_radius(center),
				center->radius));

	angle_t angle = trace_angle(hyu) + angleStep;

	fixed_t d = center->radius;

	fixed_t x = P_ReturnThrustX(center, angle, d);
	fixed_t y = P_ReturnThrustY(center, angle, d);

	hyu->momx = (center->x + x) - hyu->x;
	hyu->momy = (center->y + y) - hyu->y;
	hyu->angle = angle + ANGLE_90;

	sine_bob(hyu, angle, FRACUNIT);

	hyu->z = P_GetZAt(center->standingslope, hyu->x, hyu->y,
			P_GetMobjGround(center));
}

static void
rise_thru_stack (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	fixed_t spacer = ((target->height / 2) +
			(HYU_VISUAL_HEIGHT * hyu->scale * 2));

	fixed_t sink = hyudoro_stackpos(hyu) * spacer;

	fixed_t zofs = abs(hyu->momz);
	fixed_t d = (zofs - sink);
	fixed_t speed = d / 8;

	if (abs(d) < abs(speed))
		zofs = sink;
	else
		zofs -= speed;

	hyu->momz = zofs * P_MobjFlip(target);
}

static void
project_hyudoro_hover (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	// Turns a bit toward its target
	angle_t ang = get_look_angle(target) + ANGLE_67h;
	fixed_t rad = (target->radius * 2) + hyu->radius;

	P_MoveOrigin(hyu,
			target->x - P_ReturnThrustX(hyu, ang, rad),
			target->y - P_ReturnThrustY(hyu, ang, rad),
			target->z);

	// Cancel momentum from HYU_RETURN.
	// (And anything else! I don't trust this game!!)
	hyu->momx = 0;
	hyu->momy = 0;

	rise_thru_stack(hyu);

	hyu->angle = ang;

	// copies sprite tilting
	hyu->pitch = target->pitch;
	hyu->roll = target->roll;

	bob_in_place(hyu, 64);
}

static boolean
project_hyudoro_orbit (mobj_t *hyu)
{
	mobj_t *orbit = hyudoro_target(hyu);

	if (P_MobjWasRemoved(orbit))
	{
		return false;
	}

	P_MoveOrigin(hyu, orbit->x, orbit->y, orbit->z);
	hyu->destscale = orbit->scale;

	mobj_t *facing = orbit->target;

	if (!P_MobjWasRemoved(facing))
	{
		hyu->angle = R_PointToAngle2(
				hyu->x, hyu->y, facing->x, facing->y);
	}

	return true;
}

static mobj_t *
find_duel_target (mobj_t *ignore)
{
	mobj_t *ret = NULL;
	UINT8 bestPosition = UINT8_MAX;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator || player->exiting)
		{
			continue;
		}

		if (!player->mo || P_MobjWasRemoved(player->mo))
		{
			continue;
		}

		if (ignore != NULL && player->mo == ignore)
		{
			continue;
		}

		if (player->position < bestPosition)
		{
			ret = player->mo;
			bestPosition = player->position;

			if (bestPosition <= 1)
			{
				// Can't get any lower
				break;
			}
		}
	}

	return ret;
}

static void
do_confused (mobj_t *hyu)
{
	// Hyudoro is confused.
	// Spin around, try to find a new target.

	// Try to find new target
	P_SetTarget(&hyudoro_target(hyu),
		find_duel_target(hyudoro_stolefrom(hyu)));

	// Spin in circles
	hyu->angle += ANGLE_45;

	// Bob very fast
	bob_in_place(hyu, 32);

	hyu->sprzoff += HYU_VISUAL_HEIGHT * hyu->scale;
}

static void
move_to_player (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	angle_t angle;
	fixed_t speed;

	if (!target || P_MobjWasRemoved(target))
	{
		do_confused(hyu);
		return;
	}

	angle = R_PointToAngle2(
			hyu->x, hyu->y, target->x, target->y);

	speed = (hyu->radius / 2) +
		max(hyu->radius, K_GetSpeed(target));

	// For first place only: cap hyudoro speed at 50%
	// target player's kart speed
	if (target->player && target->player->leaderpenalty)
	{
		const fixed_t normalspeed =
			K_GetKartSpeed(target->player, false, false) / 2;

		if (hyu->type == MT_HYUDORO)
			speed = min(speed*2, normalspeed*2);		//SCS EDIT - double speed
		else
			speed = min(speed*3, normalspeed*3);		//SCS ADD - for Butler Hyudoro
		
	}													//SCS NOTE - the pickpocket will not get a speedboost

	P_InstaThrust(hyu, angle, speed);

	hyu->z = target->z; // stay level with target
	hyu->angle = angle;

	hyu->color = target->color;
}

static void
deliver_item (mobj_t *hyu)
{
	/* set physical position to visual position in stack */
	hyu->z += hyu->momz;
	hyu->momz = 0;

	mobj_t *emerald = P_SpawnMobjFromMobj(
			hyu, 0, 0, 0, MT_EMERALD);

	/* only want emerald for its orbiting behavior, so make
	   it invisible */
	P_SetMobjState(emerald, S_INVISIBLE);

	Obj_BeginEmeraldOrbit(
			emerald, hyudoro_target(hyu), 0, 64, 128);

	/* See Obj_GiveEmerald. I won't risk relying on the
	   Hyudoro object in case it is removed first. So go
	   through the capsule instead. */
	Obj_SetEmeraldAwardee(emerald, hyudoro_capsule(hyu));

	/* hyudoro will teleport to emerald (orbit the player) */
	hyudoro_mode(hyu) = HYU_ORBIT;
	P_SetTarget(&hyudoro_target(hyu), emerald);

	hyu->renderflags &= ~(RF_DONTDRAW | RF_BLENDMASK);
	reset_shadow(hyu);
}

static void
append_hyudoro
(		mobj_t ** head,
		mobj_t * hyu)
{
	INT32 lastpos = 0;

	while (is_hyudoro(*head))
	{
		lastpos = hyudoro_stackpos(*head);
		head = &hyudoro_next(*head);
	}

	hyudoro_stackpos(hyu) = lastpos + 1;
	P_SetTarget(head, hyu);

	/* only first in list gets a shadow */
	if (lastpos == 0)
	{
		reset_shadow(hyu);
	}
	else
	{
		hyu->shadowcolor = 31;/* black - hide it */
	}
}

static void
pop_hyudoro (mobj_t **head)
{
	mobj_t *hyu = *head;

	if (!is_hyudoro(hyu))
	{
		return;
	}

	INT32 lastpos = hyudoro_stackpos(hyu);

	{
		mobj_t *next = hyudoro_next(hyu);

		P_SetTarget(head, next);
		P_SetTarget(&hyudoro_next(hyu), NULL);

		hyu = next;
	}

	if (!is_hyudoro(hyu))
	{
		return;
	}

	reset_shadow(hyu);/* show it */

	do
	{
		INT32 thispos = hyudoro_stackpos(hyu);

		hyudoro_stackpos(hyu) = lastpos;
		lastpos = thispos;

		hyu = hyudoro_next(hyu);
	}
	while (is_hyudoro(hyu));
}

static mobj_t *
spawn_capsule (mobj_t *hyu, INT32 item)					//SCS EDIT - adding item parameter so flags can not be set if it's a pickpocket ringbox
{
	mobj_t *caps = P_SpawnMobjFromMobj(
			hyu, 0, 0, 0, MT_ITEMCAPSULE);

	/* hyudoro only needs its own shadow */
	caps->shadowscale = 0;

	caps->flags |=
		MF_NOGRAVITY |
		MF_NOCLIP |
		MF_NOCLIPTHING |
		MF_NOCLIPHEIGHT;

	/* signal that this item capsule always puts items in the HUD */
	if (item >= 0)														//SCS ADD - if we're less than 0, it's likely a pickpocket ringbox
		caps->flags2 |= MF2_STRONGBOX;
	else																//SCS ADD - We're gonna use extravalue2 to keep track of the item amount when it returns to the player
		caps->extravalue2 = hyu->extravalue2;
		
	//CONS_Printf("Item amount set: %d \n", caps->extravalue2);

	P_SetTarget(&hyudoro_capsule(hyu), caps);

	/* capsule teleports to hyudoro */
	P_SetTarget(&caps->target, hyu);

	/* so it looks like hyudoro is holding it */
	caps->sprzoff = 20 * hyu->scale;

	return caps;
}

static void
update_capsule_position (mobj_t *hyu)
{
	mobj_t *caps = hyudoro_capsule(hyu);

	if (P_MobjWasRemoved(caps))
		return;

	caps->extravalue1 = hyu->scale / 3;

	/* hold it in the hyudoro's hands */
	const fixed_t r = hyu->radius;
	caps->sprxoff = FixedMul(r, FCOS(hyu->angle));
	caps->spryoff = FixedMul(r, FSIN(hyu->angle));
}

static void
set_item
(		mobj_t * hyu,
		INT32 item,
		INT32 amount)
{
	mobj_t *caps = P_MobjWasRemoved(hyudoro_capsule(hyu))
		? spawn_capsule(hyu, item) : hyudoro_capsule(hyu);

	hyudoro_itemtype(hyu) = item;
	hyudoro_itemcount(hyu) = amount;

	caps->threshold = item == -1 ? HYUCAPSULE_RING : hyudoro_itemtype(hyu);	//SCS EDIT - -1 is the value passed by pickpockets for stealing rings
	caps->movecount = hyudoro_itemcount(hyu);
	
	//CONS_Printf("Amount: %d \n", amount);
}

static void
hyudoro_set_held_item_from_player
(		mobj_t * hyu,
		player_t * player)
{
	if (hyu->type != MT_PPOCKETHYUDORO && K_ItemEnabled(KITEM_KITCHENSINK))		//SCS EDIT
	{
		boolean convert = false;

		switch (player->itemtype)
		{
			// The following permits a case-by-case balancing for items
			// we don't want ending up in 2nd place's hands too often...
			case KITEM_SPB:
				convert = true;
				break;
			default:
				break;
		}

		if (convert == true)
		{
			if (player->itemtype > 0 && player->itemtype < NUMKARTITEMS)
			{
				// A conversion has occoured, this is no longer on the
				// playing field... someone else must manifest it!?
				itemCooldowns[player->itemtype - 1] = 0;
			}

			set_item(hyu, KITEM_KITCHENSINK, 1);

			return;
		}
	}
	
	if (hyu->type == MT_BHYUDORO)		//SCS ADD
	{
		if (player->itemtype == KITEM_BUBBLESHIELD || player->itemtype == KITEM_FLAMESHIELD || player->itemtype == KITEM_NORMALSHIELD || player->itemtype == KITEM_ARMASHIELD)	//no stacking shields
			set_item(hyu, player->itemtype, player->itemamount);
		else
			set_item(hyu, player->itemtype, min(player->itemamount*2, 99));
		
	}
	else if (player->itemtype != KITEM_NONE)					//SCS ADD - Pickpockets don't try to grab items; if the item is valid, spawn a new normal hyu to grab it
	{
		if (hyu->type != MT_PPOCKETHYUDORO)		//if you're not a pickpocket, grab the item like normal
		{
			set_item(hyu, player->itemtype, player->itemamount);
			return;
		}
		else if (player->rings > 0 || player->superring > 0)
		{
			mobj_t *itemhyu = P_SpawnMobjFromMobj(hyu, 0, 0, 0, MT_HYUDORO);
			
			if (itemhyu)
			{
				itemhyu->target = hyu->target;
				set_item(itemhyu, player->itemtype, player->itemamount);	//new hyu grabs the item
				hyudoro_mode(itemhyu) = HYU_RETURN;
				P_SetMobjState(itemhyu, S_HYUDORO_RETURNING);
				player->hyudorotimer = hyudorotime;
			}
			
			hyu->target->player->pickpockethyucombo = min(hyu->target->player->pickpockethyucombo++, 5);
			
			if (player->superring > 0)										//pickpocket grabs the moolah
				set_item(hyu, -1, ((player->rings + player->superring)*hyu->target->player->pickpockethyucombo));			
			else
				set_item(hyu, -1, (player->rings*hyu->target->player->pickpockethyucombo));
			
			player->rings = 0;
			player->superring = 0;
			K_AddMessageForPlayer(hyu->target->player, va("Pickpocket Combo x%d!", hyu->target->player->pickpockethyucombo), true, false);
		}
	}
	else if (player->rings > 0 || player->superring > 0)		//SCS ADD - For pickpocket, stealing rings only
	{
		hyu->target->player->pickpockethyucombo = min(hyu->target->player->pickpockethyucombo++, 5);
		set_item(hyu, -1, ((player->rings + player->superring)*hyu->target->player->pickpockethyucombo));
		player->rings = 0;
		player->superring = 0;
		K_AddMessageForPlayer(hyu->target->player, va("Pickpocket Combo x%d!", hyu->target->player->pickpockethyucombo), true, false);
	}
}

static void pickpocket_hyudoro_chain_destroy (mobj_t * hyu)							//SCS ADD
{
	if (hyu->tracer != NULL)
		pickpocket_hyudoro_chain_destroy(hyu->tracer);

	hyu->target = NULL;
	P_RemoveMobj(hyu);
	//CONS_Printf("Chain Destroy\n");
}

static void pickpocket_hyudoro_set_stunned (mobj_t * hyu)							//SCS ADD
{
	if (hyu->tracer != NULL)
		pickpocket_hyudoro_set_stunned(hyu->tracer);

	P_SetMobjState(hyu, S_PPOCKET_STUNNED);
	hyu->destscale = hyu->scale*2;
}

static boolean
hyudoro_patrol_hit_player
(		mobj_t * hyu,
		mobj_t * toucher)
{
	player_t *player = toucher->player;

	mobj_t *center = hyudoro_center(hyu);

	mobj_t *master = NULL;

	if (!player)
		return false;

	// Cannot hit its master
	master = get_hyudoro_master(hyu);
	if (toucher == master)
		return false;

	// Don't punish a punished player
	if (player->hyudorotimer)
		return false;

	player->pflags |= PF_CASTSHADOW;

	// NO ITEM?
	if (!player->itemamount)
		if (hyu->type != MT_PPOCKETHYUDORO && hyu->type != MT_MINIHYUDORO)		//SCS ADD (These types steal rings too)
			return false;
		
	if ((hyu->type == MT_PPOCKETHYUDORO || hyu->type == MT_MINIHYUDORO) && player->rings <= 0 && player->superring <= 0)	//SCS ADD
		return false;
	
	if (hyu->type == MT_MINIHYUDORO)		//SCS ADD - If we hit one of the mini-hyudoros in the chain of the pickpocket, switch over to the leader, and handle interacting with the leader from here on
		hyu = hyu->target;
		
	if (hyu->state == &states[S_PPOCKET_STUNNED] || hyu->state == &states[S_PPOCKET_VANISH])	//Stunned Pickpockets can't steal
		return false;
		
	if (hyu->type == MT_PPOCKETHYUDORO && hyu->extravalue2 == 0)	//point blank hit, do nothing
		return false;
		
	K_AddHitLag(toucher, TICRATE/2, false);
	
	if (hyu->type == MT_PPOCKETHYUDORO && hyu->tracer != NULL)	//SCS ADD - Also destroy all the mini ones at this point
		pickpocket_hyudoro_chain_destroy(hyu->tracer);	

	hyudoro_mode(hyu) = HYU_RETURN;

	hyudoro_set_held_item_from_player(hyu, player);

	if (!P_MobjWasRemoved(hyudoro_capsule(hyu)) && hyudoro_capsule(hyu)->extravalue2 <= 0)		//SCS EDIT - Don't meddle with HYUCAPSULE capsules, we need this value to properly transfer itemamount back to the player owner
	{
		hyudoro_capsule(hyu)->extravalue2 = player->skincolor;
	}
	
	//K_StripItems(player);
	K_StripItemsExceptBackup(player);		//SCS EDIT - So, what? Does the backup item just vanish into the void???

	/* do not make 1st place invisible */
	if (player->hyudorotimer <= 0 && (hyu->type == MT_PPOCKETHYUDORO || player->leaderpenalty == 0))		//SCS EDIT - Pickpockets always grant some invisibility time, since it's not a trap item. If their timer is already set here, a pickpocket took an item.
	{																																																//In that case, it's a full timer set. Don't reset it here.
		player->hyudorotimer = (hyudoro_capsule(hyu)->threshold > 0 && hyudoro_capsule(hyu)->threshold != HYUCAPSULE_RING) ? hyudorotime : (hyudorotime/3);	//SCS EDIT - Pickpocket ring steal grants less invisibility time
	}

	S_StartSound(toucher, sfx_s3k92);

	player->stealingtimer = hyudorotime;
	
	if (hyu->type == MT_BHYUDORO)				//SCS ADD
	{
		player->bstealingtimer = hyudorotime;	//this is literally only used for the HUD to show a different stealing graphic in the item slot
		hyu->destscale = hyu->scale/2;			//when returning to the owner, shrink back down as to not obstruct the owner's view
	}

	P_SetTarget(&hyudoro_stolefrom(hyu), toucher);

	if (master == NULL || P_MobjWasRemoved(master))
	{
		// if master is NULL, it is probably a DUEL
		master = find_duel_target(toucher);
	}

	P_SetTarget(&hyudoro_target(hyu), master);									//SCS NOTE - this is what sets the target for the hyudoro so it moves back to the player

	//if (master && !P_MobjWasRemoved(master))
		//K_SpawnAmps(master->player, K_PvPAmpReward(20, master->player, player), toucher);
	
	UINT8 hyuAmps = 0;															//SCS - RADIO START
	if (master && !P_MobjWasRemoved(master)) {
		hyuAmps = K_PvPAmpReward(hyu->type == MT_PPOCKETHYUDORO ? 5 : 20, master->player, player);
		K_SpawnAmps(master->player, hyuAmps, toucher);
	}																			//SCS - RADIO END

	if (hyu->type != MT_PPOCKETHYUDORO && hyu->type != MT_MINIHYUDORO)	//SCS ADD
	{
		if (center)
			P_RemoveMobj(center);
	}
	else
		hyu->flags |= MF_NOCLIP;

	hyu->renderflags &= ~(RF_DONTDRAW);

	// Reset shadow to default (after alt_shadow)
	reset_shadow(hyu);

	// This will flicker the shadow
	hyudoro_timer(hyu) = 18;

	if (hyu->type == MT_BHYUDORO)		//SCS ADD
	{
		P_SetMobjState(hyu, S_BHYUDORO_RETURNING);
		// Radio
		RR_PushPlayerInteractionToFeed(master, toucher, ATTACK_HYUDOROBUTLER, hyuAmps);
	}
	else if (hyu->type == MT_PPOCKETHYUDORO)	//SCS ADD
	{
		P_SetMobjState(hyu, S_HYUDORO_RETURNING);
		// Radio
		RR_PushPlayerInteractionToFeed(master, toucher, ATTACK_PICKPOCKETHYU, hyuAmps);
	}	
	else
	{
		P_SetMobjState(hyu, S_HYUDORO_RETURNING);
		// Radio
		RR_PushPlayerInteractionToFeed(master, toucher, ATTACK_HYUDORO, hyuAmps);		//SCS - RADIO
	}

	return true;
}

static boolean
award_immediately (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	if (player)
	{
		if (hyudoro_itemtype(hyu) == -1 && hyu->target->health > 0)				//SCS ADD - -1 means we have rings and are a pickpocket. We can always take rings!
		{
			deliver_item(hyu);
			return true;
		}		
		
		if (hyudoro_itemtype(hyu) == -2 && hyu->target->health > 0)				//SCS ADD - -2 means we got hit after being fired, and are returning to go into the player's item slot
		{
			P_SetMobjState(hyu, S_PPOCKET_VANISH);
			
			if (hyu->target->player)
			{
				//CONS_Printf("We're giving back: %d\n", hyu->extravalue2);
				
				if (hyu->target->player->itemtype == KITEM_PICKPOCKETHYU || hyu->target->player->itemtype == KITEM_NONE)		//Going back into player's item slots
				{
					hyu->target->player->itemtype = KITEM_PICKPOCKETHYU;
					K_AdjustPlayerItemAmount(hyu->target->player, hyu->extravalue2);
				}
				else if (hyu->target->player->backupitemtype == KITEM_PICKPOCKETHYU || hyu->target->player->backupitemtype == KITEM_NONE)
				{
					hyu->target->player->backupitemtype = KITEM_PICKPOCKETHYU;
					K_AdjustPlayerBackupItemAmount(hyu->target->player, hyu->extravalue2);
				}
			}
			
			hyudoro_mode(hyu) = HYU_VANISH;
			hyu->threshold = 5;
			//P_RemoveMobj(hyu);
			return true;
		}
		
		if (!P_CanPickupItem(player, PICKUP_ITEMBOX))
			return false;
		
		if (player->leaderpenalty)
		{
			return false;
		}

		// Prevent receiving any more items or even stacked
		// Hyudoros! Put on a timer so roulette cannot become
		// locked permanently.
		player->itemRoulette.reserved = 2*TICRATE;
	}
	//CONS_Printf("Bing bing wahoo");
	deliver_item(hyu);

	return true;
}

static boolean
hyudoro_return_hit_player
(		mobj_t * hyu,
		mobj_t * toucher)
{
	if (toucher != hyudoro_target(hyu))
		return false;

	// If the player already has an item, just hover beside
	// them until they use/lose it.
	if (!award_immediately(hyu))
	{
		S_StartSound(hyudoro_target(hyu), sfx_kc3d);
		hyudoro_mode(hyu) = HYU_HOVER;
		append_hyudoro(&toucher->player->hoverhyudoro, hyu);
	}

	return true;
}

static boolean
hyudoro_hover_await_stack (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	if (!player)
		return false;

	// First in stack goes first
	if (hyu != player->hoverhyudoro)
		return false;

	if (!award_immediately(hyu))
		return false;

	pop_hyudoro(&player->hoverhyudoro);

	return true;
}

static void
trail_ghosts
(		mobj_t * hyu,
		boolean colorize)
{
	// Spawns every other frame
	if (leveltime & 1)
	{
		return;
	}

	mobj_t *ghost = P_SpawnGhostMobj(hyu);

	// Flickers every frame
	ghost->extravalue1 = 1;
	ghost->extravalue2 = 2;

	// copy per-splitscreen-player visibility
	ghost->renderflags =
		(hyu->renderflags & RF_DONTDRAW);

	ghost->colorized = colorize;

	ghost->tics = 8;

	P_SetTarget(&ghost->tracer, hyu);
}

static void
trail_glow (mobj_t *hyu)
{
	mobj_t *ghost = P_SpawnGhostMobj(hyu);

	// Flickers every frame
	ghost->extravalue1 = 1;
	ghost->extravalue2 = 0;

	ghost->renderflags = RF_ADD | RF_TRANS80;

	ghost->tics = 2; // this actually does last one tic

	ghost->momz = hyu->momz; // copy stack position
}

static void
blend_hover_hyudoro (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	hyu->renderflags &= ~(RF_BLENDMASK);

	if (!player)
	{
		return;
	}

	/* 1st place: Hyudoro stack is unusable, so make a visual
	   indication */
	if (player->leaderpenalty)
	{
		hyu->renderflags |= RF_MODULATE;
		trail_glow(hyu);
	}
}

static void
alt_shadow (mobj_t *hyu)
{
	/* spaced out pulse, fake randomness */
	switch (leveltime % (7 + ((leveltime / 8) % 3)))
	{
		default:
			hyu->shadowcolor = 15;
			hyu->whiteshadow = false;
			break;
		case 1:
			hyu->shadowcolor = 5;
			hyu->whiteshadow = true;
			break;
		case 2:
			hyu->shadowcolor = 181;
			hyu->whiteshadow = true;
			break;
		case 3:
			hyu->shadowcolor = 255;
			hyu->whiteshadow = true;
			break;
	}
}

void
Obj_InitHyudoroCenter (mobj_t * center, mobj_t * master)
{
	mobj_t *hyu = P_SpawnMobjFromMobj(
			center, 0, 0, 0, MT_HYUDORO);

	// This allows a Lua override
	if (!hyudoro_center_max_radius(center))
	{
		hyudoro_center_max_radius(center) =
			128 * center->scale;
	}

	center->radius = hyu->radius;

	hyu->angle = center->angle;
	P_SetTarget(&hyudoro_center(hyu), center);
	P_SetTarget(&hyudoro_center_master(center), master);

	hyudoro_mode(hyu) = HYU_PATROL;

	// Set splitscreen player visibility
	hyu->renderflags |= RF_DONTDRAW;
	if (master && !P_MobjWasRemoved(master) && master->player)
	{
		hyu->renderflags &= ~(K_GetPlayerDontDrawFlag(master->player));
	}

	Obj_SpawnFakeShadow(hyu); // this sucks btw
}

void
Obj_InitBHyudoroCenter (mobj_t * center, mobj_t * master)		//SCS ADD
{
	mobj_t *hyu = P_SpawnMobjFromMobj(
			center, 0, 0, 0, MT_BHYUDORO);

	// This allows a Lua override
	if (!hyudoro_center_max_radius(center))
	{
		hyudoro_center_max_radius(center) =
			128 * center->scale;
	}

	center->radius = hyu->radius;
	
	hyu->destscale = hyu->scale*2;	//He has a bigger hitbox

	hyu->angle = center->angle;
	P_SetTarget(&hyudoro_center(hyu), center);
	P_SetTarget(&hyudoro_center_master(center), master);

	hyudoro_mode(hyu) = HYU_PATROL;

	// Set splitscreen player visibility
	hyu->renderflags |= RF_DONTDRAW;
	if (master && !P_MobjWasRemoved(master) && master->player)
	{
		hyu->renderflags &= ~(K_GetPlayerDontDrawFlag(master->player));
	}

	Obj_SpawnFakeShadow(hyu); // this sucks btw
}

void
Obj_HyudoroDeploy (mobj_t *master)
{
	mobj_t *center = P_SpawnMobjFromMobj(
			master, 0, 0, 0, MT_HYUDORO_CENTER);

	center->angle = master->angle;
	Obj_InitHyudoroCenter(center, master);

	// Update floorz to the correct position by indicating
	// that it should be recalculated by P_MobjThinker.
	center->floorz = master->z;
	center->ceilingz = master->z + master->height;
	center->z = P_GetMobjGround(center) - P_MobjFlip(center);

	S_StartSound(master, sfx_s3k92); // scary ghost noise
}

void
Obj_ButlerHyudoroDeploy (mobj_t *master)						//SCS ADD
{
	mobj_t *center = P_SpawnMobjFromMobj(
			master, 0, 0, 0, MT_BHYUDORO_CENTER);

	center->angle = master->angle;
	Obj_InitBHyudoroCenter(center, master);

	// Update floorz to the correct position by indicating
	// that it should be recalculated by P_MobjThinker.
	center->floorz = master->z;
	center->ceilingz = master->z + master->height;
	center->z = P_GetMobjGround(center) - P_MobjFlip(center);

	S_StartSound(master, sfx_s3k92); // scary ghost noise
}

void
Obj_PPocketHyudoroDeploy (mobj_t *master, mobj_t *hyu)						//SCS ADD (This just sets the mode, since it's already created by throwing it as a kart item)
{
	hyudoro_mode(hyu) = HYU_PATROL;

	S_StartSound(master, sfx_s3k92); // scary ghost noise
}

void 
Obj_MiniHyudoroThink(mobj_t *th)
{
	//angle_t newangle = th->target->angle + th->angle + ANGLE_90;
	
	//UINT32 newx = th->target->x + th->target->momx + FixedMul(FixedMul(th->target->scale, th->scale*10), FINECOSINE((newangle) >> ANGLETOFINESHIFT));

	//UINT32 newy = th->target->y + th->target->momy + FixedMul(FixedMul(th->target->scale, th->scale*10), FINESINE((newangle) >> ANGLETOFINESHIFT));
	if (th->extravalue2 > 0)	//vanishing
	{
		th->extravalue2--;
		
		if (th->extravalue2 <= 0)
		{
			th->tracer = NULL;
			P_RemoveMobj(th);
			return;
		}
	}
	else if (th->cusval > 0)
	{
		th->cusval--;
		
		if (th->cusval <= 0)
		{
			th->extravalue2 = 5;
			P_SetMobjState(th, S_PPOCKET_VANISH);
		}
	}
	else if (th->threshold > 0)
	{
		th->threshold--;
	}
	else if (th->target != NULL)
	{
		th->momx = th->target->momx;		//just follow the leader
		th->momy = th->target->momy;
		th->z = th->target->z;
	}
	//th->angle = th->target->angle;
	
	//if (K_GetForwardMove(th->target->player) < 0)
	//{
	//	th->angle = th->target->angle + ANGLE_180;
	//}
	
	//newx = FixedDiv(newx - th->x, 1);
//	newy = FixedDiv(newy - th->y, 1);
	
	//P_SetOrigin(th, (newx + th->threshold*FRACUNIT), (newy + th->threshold*FRACUNIT), (th->target->z + FRACUNIT*15));
	//P_MoveOrigin(th, (newx + th->threshold*FRACUNIT), (newy + th->threshold*FRACUNIT), th->target->z + th->target->height);
	//th->momx = FixedDiv(newx - th->x, 1);
	//th->momy = FixedDiv(newy - th->y, 1);
	
	//th->z = th->target->z + FRACUNIT*8;
}

void 
Obj_PPocketHyudoroThink(mobj_t *hyu)
{
	switch (hyudoro_mode(hyu))
	{
		case HYU_PATROL:
		
		if (hyu->cusval > 0)	//we're stunned
		{
			hyu->cusval--;
			
			if (hyu->tracer != NULL && hyu->tracer->state != &states[S_PPOCKET_STUNNED])
				pickpocket_hyudoro_set_stunned(hyu->tracer);
			
			//CONS_Printf("Timer: %d\n", hyu->cusval);
			
			if (hyu->cusval <= 0)
			{
				P_SetMobjState(hyu, S_HYUDORO);						//start returning to our owner
				
				if (hyu->tracer != NULL)							//get rid of the mini-hyudoro while we flee
					pickpocket_hyudoro_chain_destroy(hyu->tracer);
					
				
				hyudoro_mode(hyu) = HYU_RETURN;
				hyu->destscale = hyu->scale/3;
				hyudoro_itemtype(hyu) = -2;			//Mark as "don't deliver anything"			
			}
		}
		else if (hyu->threshold > -1)
		{
			hyu->threshold--;
			
			if (hyu->threshold % 10 == 0)
			{
				mobj_t *mini = P_SpawnMobjFromMobj((hyu->tracer != NULL && hyu->tracer->state != &states[S_PPOCKET_STUNNED] && hyu->tracer->state != &states[S_PPOCKET_VANISH]) ? hyu->tracer : hyu, 0, 0, 0, MT_MINIHYUDORO);
				
				if (mini != NULL)
				{
					mini->target = hyu;
					mini->threshold = 5;	//how long until it starts moving
					mini->angle = hyu->angle;
					
					if (hyu->tracer != NULL)	//if it's NOT the first one we spawned
					{
						mini->tracer = hyu->tracer;
					}
						
					hyu->tracer = mini;
				}
			}
		}
		break;
		case HYU_RETURN:
			move_to_player(hyu);
			trail_ghosts(hyu, true);

			if (hyudoro_timer(hyu) > 0)
				hyu->whiteshadow = !hyu->whiteshadow;
			break;

		case HYU_HOVER:
			if (hyudoro_target(hyu))
			{
				project_hyudoro_hover(hyu);

				if (hyudoro_hover_await_stack(hyu))
					break;
			}
			blend_hover_hyudoro(hyu);
		break;

		case HYU_ORBIT:
			if (!project_hyudoro_orbit(hyu))
			{
				hyu->tracer = NULL;	//Used to mark a delivering hyudoro rather than one that hit a wall (The minis should already be gone by this point)
				
				P_RemoveMobj(hyu);
				return;
			}
		break;
		case HYU_VANISH:
			if (hyu->threshold > 0 && hyudoro_itemtype(hyu) == -2 && !P_MobjWasRemoved(hyu))		//SCS ADD
			{
				//CONS_Printf("Disappear: %d\n", hyu->threshold);
				hyu->threshold--;
				
				if (hyu->threshold == 0)
					P_RemoveMobj(hyu);
				
			}		
		break;
	}

	update_capsule_position(hyu);

	if (hyudoro_timer(hyu) > 0)
		hyudoro_timer(hyu)--;
	
}

/*
void 
Obj_MiniHyudoroThink_OLD(mobj_t *th)
{
	angle_t newangle = th->target->angle + th->angle + ANGLE_90;
	
	UINT32 newx = th->target->x + th->target->momx + FixedMul(FixedMul(th->target->scale, th->scale*10), FINECOSINE((newangle) >> ANGLETOFINESHIFT));

	UINT32 newy = th->target->y + th->target->momy + FixedMul(FixedMul(th->target->scale, th->scale*10), FINESINE((newangle) >> ANGLETOFINESHIFT));
			
	th->momx = 0;
	th->momy = 0;
	th->momz = 0;
	th->angle = th->target->angle;
	
	if (K_GetForwardMove(th->target->player) < 0)
	{
		th->angle = th->target->angle + ANGLE_180;
	}
	
	//newx = FixedDiv(newx - th->x, 1);
//	newy = FixedDiv(newy - th->y, 1);
	
	//P_SetOrigin(th, (newx + th->threshold*FRACUNIT), (newy + th->threshold*FRACUNIT), (th->target->z + FRACUNIT*15));
	P_MoveOrigin(th, (newx + th->threshold*FRACUNIT), (newy + th->threshold*FRACUNIT), th->target->z + th->target->height);
	//th->momx = FixedDiv(newx - th->x, 1);
	//th->momy = FixedDiv(newy - th->y, 1);
	
	//th->z = th->target->z + FRACUNIT*8;
}
*/
void
Obj_HyudoroThink (mobj_t *hyu)
{
	switch (hyudoro_mode(hyu))
	{
		case HYU_PATROL:
			if (hyudoro_center(hyu))
				project_hyudoro(hyu);

			trail_ghosts(hyu, false);
			alt_shadow(hyu);
			break;

		case HYU_RETURN:
			move_to_player(hyu);
			trail_ghosts(hyu, true);

			if (hyudoro_timer(hyu) > 0)
				hyu->whiteshadow = !hyu->whiteshadow;
			break;

		case HYU_HOVER:
			if (hyudoro_target(hyu))
			{
				project_hyudoro_hover(hyu);

				if (hyudoro_hover_await_stack(hyu))
					break;
			}
			blend_hover_hyudoro(hyu);
			break;

		case HYU_ORBIT:
			if (!project_hyudoro_orbit(hyu))
			{
				P_RemoveMobj(hyu);
				return;
			}
			break;
	}

	update_capsule_position(hyu);

	if (hyudoro_timer(hyu) > 0)
		hyudoro_timer(hyu)--;
}

void
Obj_HyudoroCenterThink (mobj_t *center)
{
	fixed_t max_radius = hyudoro_center_max_radius(center);

	if (center->radius < max_radius)
		center->radius += max_radius / 64;
}

void
Obj_HyudoroCollide
(		mobj_t * hyu,
		mobj_t * toucher)
{
	//CONS_Printf("Collided!\n");	
	
	switch (hyudoro_mode(hyu))
	{
		case HYU_PATROL:
			hyudoro_patrol_hit_player(hyu, toucher);
			break;

		case HYU_RETURN:
			hyudoro_return_hit_player(hyu, toucher);
			break;
	}
}

boolean
Obj_HyudoroShadowZ
(		mobj_t * hyu,
		fixed_t * return_z,
		pslope_t ** return_slope)
{
	if (hyudoro_mode(hyu) != HYU_PATROL)
		return false;

	if (P_MobjWasRemoved(hyudoro_center(hyu)))
		return false;

	*return_z = hyu->z;
	*return_slope = hyudoro_center(hyu)->standingslope;

	return true;
}
