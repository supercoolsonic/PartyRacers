// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_botitem.cpp
/// \brief Bot item usage logic

#include <algorithm>

#include <tracy/tracy/Tracy.hpp>

#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "r_main.h"
#include "p_local.h"
#include "k_bot.h"
#include "lua_hook.h"
#include "byteptr.h"
#include "d_net.h" // nodetoplayer
#include "k_kart.h"
#include "z_zone.h"
#include "i_system.h"
#include "p_maputl.h"
#include "d_ticcmd.h"
#include "m_random.h"
#include "r_things.h" // numskins
#include "k_roulette.h"
#include "m_easing.h"

/*--------------------------------------------------
	static inline boolean K_ItemButtonWasDown(const player_t *player)

		Looks for players around the bot, and presses the item button
		if there is one in range.

	Input Arguments:-
		player - Bot to check.

	Return:-
		true if the item button was pressed last tic, otherwise false.
--------------------------------------------------*/
static inline boolean K_ItemButtonWasDown(const player_t *player)
{
	return !!(player->oldcmd.buttons & BT_ATTACK);
}

/*--------------------------------------------------
	static boolean K_BotUseItemNearPlayer(const player_t *player, ticcmd_t *cmd, fixed_t radius)

		Looks for players around the bot, and presses the item button
		if there is one in range.

	Input Arguments:-
		player - Bot to compare against.
		cmd - The bot's ticcmd.
		radius - The radius to look for players in.

	Return:-
		true if a player was found & we can press the item button, otherwise false.
--------------------------------------------------*/
static boolean K_BotUseItemNearPlayer(const player_t *player, ticcmd_t *cmd, fixed_t radius)
{
	ZoneScoped;

	UINT8 i;

	if (K_ItemButtonWasDown(player) == true)
	{
		return false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];

		if (target->mo == NULL || P_MobjWasRemoved(target->mo)
			|| player == target || target->spectator
			|| G_SameTeam(player, target)
			|| target->flashing)
		{
			continue;
		}

		dist = P_AproxDistance(P_AproxDistance(
			player->mo->x - target->mo->x,
			player->mo->y - target->mo->y),
			(player->mo->z - target->mo->z) / 4
		);

		if (dist <= radius)
		{
			cmd->buttons |= BT_ATTACK;
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static player_t *K_PlayerNearSpot(const player_t *player, fixed_t x, fixed_t y, fixed_t radius)

		Looks for players around a specified x/y coordinate.

	Input Arguments:-
		player - Bot to compare against.
		x - X coordinate to look around.
		y - Y coordinate to look around.
		radius - The radius to look for players in.

	Return:-
		The player we found, NULL if nothing was found.
--------------------------------------------------*/
static player_t *K_PlayerNearSpot(const player_t *player, fixed_t x, fixed_t y, fixed_t radius)
{
	ZoneScoped;

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];

		if (target->mo == NULL || P_MobjWasRemoved(target->mo)
			|| player == target || target->spectator
			|| G_SameTeam(player, target)
			|| target->flashing)
		{
			continue;
		}

		dist = P_AproxDistance(
			x - target->mo->x,
			y - target->mo->y
		);

		if (dist <= radius)
		{
			return target;
		}
	}

	return NULL;
}

/*--------------------------------------------------
	static player_t *K_PlayerPredictThrow(const player_t *player, UINT8 extra)

		Looks for players around the predicted coordinates of their thrown item.

	Input Arguments:-
		player - Bot to compare against.
		extra - Extra throwing distance, for aim forward on mines.

	Return:-
		The player we're trying to throw at, NULL if none was found.
--------------------------------------------------*/
static player_t *K_PlayerPredictThrow(const player_t *player, UINT8 extra)
{
	ZoneScoped;

	const fixed_t dist = (30 + (extra * 10)) * player->mo->scale;
	const UINT32 airtime = FixedDiv(dist + player->mo->momz, gravity);
	const fixed_t throwspeed = FixedMul(82 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const fixed_t estx = player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, (throwspeed + player->speed) * airtime);
	const fixed_t esty = player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, (throwspeed + player->speed) * airtime);

	return K_PlayerNearSpot(player, estx, esty, player->mo->radius * 2);
}

/*--------------------------------------------------
	static player_t *K_PlayerInCone(const player_t *player, UINT16 cone, boolean flip)

		Looks for players in the .

	Input Arguments:-
		player - Bot to compare against.
		radius - How far away the targets can be.
		cone - Size of cone, in degrees as an integer.
		flip - If true, look behind. Otherwise, check in front of the player.

	Return:-
		true if a player was found in the cone, otherwise false.
--------------------------------------------------*/
static player_t *K_PlayerInCone(const player_t *player, fixed_t radius, UINT16 cone, boolean flip)
{
	ZoneScoped;

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];

		if (target->mo == NULL || P_MobjWasRemoved(target->mo)
			|| player == target || target->spectator
			|| G_SameTeam(player, target)
			|| target->flashing
			|| !P_CheckSight(player->mo, target->mo))
		{
			continue;
		}

		dist = P_AproxDistance(P_AproxDistance(
			player->mo->x - target->mo->x,
			player->mo->y - target->mo->y),
			(player->mo->z - target->mo->z) / 4
		);

		if (dist <= radius)
		{
			angle_t a = player->mo->angle - R_PointToAngle2(player->mo->x, player->mo->y, target->mo->x, target->mo->y);
			INT16 ad = 0;

			if (a < ANGLE_180)
			{
				ad = AngleFixed(a)>>FRACBITS;
			}
			else
			{
				ad = 360-(AngleFixed(a)>>FRACBITS);
			}

			ad = abs(ad);

			if (flip)
			{
				if (ad >= 180-cone)
				{
					return target;
				}
			}
			else
			{
				if (ad <= cone)
				{
					return target;
				}
			}
		}
	}

	return NULL;
}

/*--------------------------------------------------
	static boolean K_RivalBotAggression(const player_t *bot, const player_t *target)

		Returns if a bot is a rival & wants to be aggressive to a player.

	Input Arguments:-
		bot - Bot to check.
		target - Who the bot wants to attack.

	Return:-
		false if not the rival. false if the target is another bot. Otherwise, true.
--------------------------------------------------*/
static boolean K_RivalBotAggression(const player_t *bot, const player_t *target)
{
	if (bot == NULL || target == NULL)
	{
		// Invalid.
		return false;
	}

	if (bot->bot == false)
	{
		// lol
		return false;
	}

	if (!(bot->botvars.rival || bot->botvars.foe) && !cv_levelskull.value)
	{
		// Not the rival, we aren't self-aware.
		return false;
	}

	if (target->bot == false)
	{
		// This bot knows that the real threat is the player.
		return true;
	}

	// Calling them your friends is misleading, but you'll at least spare them.
	return false;
}

/*--------------------------------------------------
	static void K_ItemConfirmForTarget(const player_t *bot, ticcmd_t *cmd, const player_t *target, UINT16 amount)

		Handles updating item confirm values for offense items.

	Input Arguments:-
		bot - Bot to check.
		cmd - Bot's ticcmd to edit.
		target - Who the bot wants to attack.
		amount - Amount to increase item confirm time by.

	Return:-
		None
--------------------------------------------------*/
static void K_ItemConfirmForTarget(const player_t *bot, ticcmd_t *cmd, const player_t *target, UINT16 amount)
{
	if (bot == NULL || target == NULL)
	{
		return;
	}

	if (K_RivalBotAggression(bot, target) == true)
	{
		// Double the rate when you're aggressive.
		cmd->bot.itemconfirm += amount << 1;
	}
	else
	{
		// Do as normal.
		cmd->bot.itemconfirm += amount;
	}
}

/*--------------------------------------------------
	static boolean K_BotGenericPressItem(const player_t *player, ticcmd_t *cmd, SINT8 dir)

		Presses the item button & aim buttons for the bot.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		dir - Aiming direction: 1 for forwards, -1 for backwards, 0 for neutral.

	Return:-
		true if we could press, false if not.
--------------------------------------------------*/
static boolean K_BotGenericPressItem(const player_t *player, ticcmd_t *cmd, SINT8 dir)
{
	ZoneScoped;

	if (K_ItemButtonWasDown(player) == true)
	{
		return false;
	}

	cmd->throwdir = KART_FULLTURN * dir;
	cmd->buttons |= BT_ATTACK;
	//player->botvars.itemconfirm = 0;
	return true;
}

/*--------------------------------------------------
	static void K_BotItemGenericTap(const player_t *player, ticcmd_t *cmd)

		Item usage for generic items that you need to tap.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericTap(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (K_ItemButtonWasDown(player) == false)
	{
		cmd->buttons |= BT_ATTACK;
		//player->botvars.itemconfirm = 0;
	}
}

/*--------------------------------------------------
	static boolean K_BotRevealsGenericTrap(const player_t *player, INT16 turnamt, boolean mine)

		Decides if a bot is ready to reveal their trap item or not.

	Input Arguments:-
		player - Bot that has the banana.
		turnamt - How hard they currently are turning.
		mine - Set to true to handle Mine-specific behaviors.

	Return:-
		true if we want the bot to reveal their banana, otherwise false.
--------------------------------------------------*/
static boolean K_BotRevealsGenericTrap(const player_t *player, INT16 turnamt, boolean mine)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		// DON'T reveal on turns, we can place bananas on turns whenever we have multiple to spare,
		// or if you missed your intentioned throw/place on a player.
		return false;
	}

	// Check the predicted throws.
	if (K_PlayerPredictThrow(player, 0) != NULL)
	{
		return true;
	}

	if (mine)
	{
		if (K_PlayerPredictThrow(player, 1) != NULL)
		{
			return true;
		}
	}

	// Check your behind.
	if (K_PlayerInCone(player, coneDist, 15, true) != NULL)
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static void K_BotItemGenericTrapShield(const player_t *player, ticcmd_t *cmd, INT16 turnamt, boolean mine)

		Item usage for Eggman shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.
		mine - Set to true to handle Mine-specific behaviors.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericTrapShield(const player_t *player, ticcmd_t *cmd, INT16 turnamt, boolean mine)
{
	ZoneScoped;

	if (player->itemflags & IF_ITEMOUT)
	{
		return;
	}

	cmd->bot.itemconfirm++;

	if (K_BotRevealsGenericTrap(player, turnamt, mine) || (player->botvars.itemconfirm > 5*TICRATE))
	{
		K_BotGenericPressItem(player, cmd, 0);
	}
}

/*--------------------------------------------------
	static void K_BotItemGenericOrbitShield(const player_t *player, ticcmd_t *cmd)

		Item usage for orbitting shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericOrbitShield(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->itemflags & IF_ITEMOUT)
	{
		return;
	}

	K_BotGenericPressItem(player, cmd, 0);
}

/*--------------------------------------------------
	static void K_BotItemSneaker(const player_t *player, ticcmd_t *cmd)

		Item usage for sneakers.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemSneaker(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// Don't use while mid-air.
		return;
	}

	if ((player->offroad && K_ApplyOffroad(player)) // Stuck in offroad, use it NOW
		|| K_GetWaypointIsShortcut(player->nextwaypoint) == true // Going toward a shortcut!
		|| player->speed < K_GetKartSpeed(player, false, true) / 2 // Being slowed down too much
		|| player->speedboost > (FRACUNIT/8) // Have another type of boost (tethering)
		|| player->botvars.itemconfirm > 4*TICRATE) // Held onto it for too long
	{
		if (player->sneakertimer == 0 && player->weaksneakertimer == 0 && K_ItemButtonWasDown(player) == false)
		{
			cmd->buttons |= BT_ATTACK;
			//player->botvars.itemconfirm = 2*TICRATE;
		}
	}
	else
	{
		cmd->bot.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemArmageddonShield player_t *player, ticcmd_t *cmd)

		Item usage for the arma shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemArmageddonShield(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// Don't use while mid-air.
		return;
	}

	if (player->botvars.itemconfirm > TICRATE)
	{
		if (player->armageddonshieldboostdelay <= 0 && leveltime % 45 == 0 && K_ItemButtonWasDown(player) == false)
		{
			cmd->buttons |= BT_ATTACK;
			//player->botvars.itemconfirm = 0;
		}
	}
	else
	{
		cmd->bot.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemChameleonBlaster(player_t *player, ticcmd_t *cmd)

		Item usage for the chameleon blaster.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemChameleonBlaster(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->botvars.itemconfirm > TICRATE)
	{
		if (player->gunusagetimer && player->gunfiredelay <= 0 && K_ItemButtonWasDown(player) == false)
		{
			if (player->botvars.difficulty > 7 && player->chamblasterrapidshots == 7) //if we're high enough level, be smart enough not to misfire (level 8 and above)
				return;
				
			else if (player->botvars.difficulty > 4 && player->chamblasterrapidshots >= 7 && player->chamblasterrapidshots < 10) //cpu should probably misfire a few times between levels 5 and 7
			{
				cmd->buttons |= BT_ATTACK;
				return;
			}
			else if (player->botvars.difficulty <= 4 && player->chamblasterrapidshots >= 7 && player->chamblasterrapidshots < 12) //cpu at really low levels should misfire a bunch before reloading
			{
				cmd->buttons |= BT_ATTACK;
				return;
			}
			else
				cmd->buttons |= BT_ATTACK;
			//player->botvars.itemconfirm = 0;
		}
	}
	else
	{
		cmd->bot.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemMegaChopper(player_t *player, ticcmd_t *cmd)

		Item usage for the mega chopper.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemMegaChopper(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// Don't use while mid-air.
		return;
	}

	if (player->botvars.itemconfirm > TICRATE)
	{
		if (leveltime % 35 == 0 && K_ItemButtonWasDown(player) == false)
		{
			cmd->buttons |= BT_ATTACK;
			//player->botvars.itemconfirm = 0;
		}
	}
	else
	{
		cmd->bot.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemRocketSneaker(player_t *player, ticcmd_t *cmd)

		Item usage for rocket sneakers.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRocketSneaker(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// Don't use while mid-air.
		return;
	}

	if (player->botvars.itemconfirm > TICRATE)
	{
		if (player->sneakertimer == 0 && player->weaksneakertimer == 0 && K_ItemButtonWasDown(player) == false)
		{
			cmd->buttons |= BT_ATTACK;
			//player->botvars.itemconfirm = 0;
		}
	}
	else
	{
		cmd->bot.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemBanana(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for trap item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemBanana(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = -1;
	boolean tryLookback = false;
	player_t *target = NULL;

	cmd->bot.itemconfirm++;

	target = K_PlayerInCone(player, coneDist, 15, true);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
		throwdir = -1;
		tryLookback = true;
	}

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		cmd->bot.itemconfirm += player->botvars.difficulty / 2;
		throwdir = -1;
	}
	else
	{
		target = K_PlayerPredictThrow(player, 0);

		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * 2);
			throwdir = 1;
		}
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE || player->bananadrag >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemMine(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for trap item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemMine(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = 0;
	boolean tryLookback = false;
	player_t *target = NULL;

	cmd->bot.itemconfirm++;

	target = K_PlayerInCone(player, coneDist, 15, true);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
		throwdir = -1;
	}

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		cmd->bot.itemconfirm += player->botvars.difficulty / 2;
		throwdir = -1;
		tryLookback = true;
	}
	else
	{
		target = K_PlayerPredictThrow(player, 0);
		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * 2);
			throwdir = 0;
		}

		target = K_PlayerPredictThrow(player, 1);
		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * 2);
			throwdir = 1;
		}
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE || player->bananadrag >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemLandmine(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for landmine tossing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemLandmine(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	player_t *target = NULL;

	cmd->bot.itemconfirm++;

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		cmd->bot.itemconfirm += player->botvars.difficulty / 2;
	}

	target = K_PlayerInCone(player, coneDist, 15, true);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, -1);
	}
}

/*--------------------------------------------------
	static void K_BotItemEggman(player_t *player, ticcmd_t *cmd)

		Item usage for Eggman item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggman(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const UINT8 stealth = K_EggboxStealth(player->mo->x, player->mo->y);
	SINT8 throwdir = -1;
	boolean tryLookback = false;
	player_t *target = NULL;

	cmd->bot.itemconfirm++;

	target = K_PlayerPredictThrow(player, 0);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty / 2);
		throwdir = 1;
	}

	target = K_PlayerInCone(player, coneDist, 15, true);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
		throwdir = -1;
		tryLookback = true;
	}

	if (stealth > 1 || player->itemRoulette.active == true)
	{
		cmd->bot.itemconfirm += player->botvars.difficulty * 4;
		throwdir = -1;
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE || player->bananadrag >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static boolean K_BotRevealsEggbox(const player_t *player)

		Decides if a bot is ready to place their Eggman item or not.

	Input Arguments:-
		player - Bot that has the eggbox.

	Return:-
		true if we want the bot to reveal their eggbox, otherwise false.
--------------------------------------------------*/
static boolean K_BotRevealsEggbox(const player_t *player)
{
	ZoneScoped;

	const fixed_t coneDist = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const UINT8 stealth = K_EggboxStealth(player->mo->x, player->mo->y);
	player_t *target = NULL;

	// This is a stealthy spot for an eggbox, lets reveal it!
	if (stealth > 1)
	{
		return true;
	}

	// Check the predicted throws.
	target = K_PlayerPredictThrow(player, 0);
	if (target != NULL)
	{
		return true;
	}

	// Check your behind.
	target = K_PlayerInCone(player, coneDist, 15, true);
	if (target != NULL)
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static void K_BotItemEggmanShield(const player_t *player, ticcmd_t *cmd)

		Item usage for Eggman shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggmanShield(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->itemflags & IF_EGGMANOUT)
	{
		return;
	}

	cmd->bot.itemconfirm++;

	if (K_BotRevealsEggbox(player) == true || (player->botvars.itemconfirm > 20*TICRATE))
	{
		K_BotGenericPressItem(player, cmd, 0);
	}
}

/*--------------------------------------------------
	static void K_BotItemEggmanExplosion(const player_t *player, ticcmd_t *cmd)

		Item usage for Eggman explosions.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggmanExplosion(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->position == 1)
	{
		// Hey, we aren't gonna find anyone up here...
		// why don't we slow down a bit? :)
		cmd->forwardmove /= 2;
	}

	K_BotUseItemNearPlayer(player, cmd, 128*player->mo->scale);
}

/*--------------------------------------------------
	static void K_BotItemOrbinaut(const player_t *player, ticcmd_t *cmd)

		Item usage for Orbinaut throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemOrbinaut(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(2560 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = -1;
	boolean tryLookback = false;
	UINT8 snipeMul = 2;
	player_t *target = NULL;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
		snipeMul = 3; // Confirm faster when you'll throw it with a bunch of extra speed!!
	}

	cmd->bot.itemconfirm++;

	target = K_PlayerInCone(player, radius, 15, false);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
		throwdir = 1;
	}
	else
	{
		target = K_PlayerInCone(player, radius, 15, true);

		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
			throwdir = -1;
			tryLookback = true;
		}
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemRingGun(const player_t *player, ticcmd_t *cmd)					//SCS ADD

		Item usage for charging the Ring Gun.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRingGun(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(2304 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	UINT8 snipeMul = 2;
	player_t *target = NULL;
	boolean hold = false;
	SINT8 desperate = 0;
	
	// Too slow, use the item as a last ditch-effort to keep going.
	if (player->speed < ((topspeed/4)*3)
		|| (player->offroad && K_ApplyOffroad(player)))
		desperate++;

	if (desperate == 0)
	{
		if (player->speed > topspeed)
		{
			radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
			snipeMul = 3; // Confirm faster when you'll throw it with a bunch of extra speed!!
		}

		target = K_PlayerInCone(player, (radius*2), 20, false);
		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
			
			fixed_t dist = P_AproxDistance(P_AproxDistance(
				player->mo->x - target->mo->x,
				player->mo->y - target->mo->y),
				(player->mo->z - target->mo->z) / 4
			);
			
			if (dist < ((radius/4)*3)) desperate += 3; // REALLY close.
			else if (dist < ((radius/4)*4)) desperate += 2; // Medium distance.
			else desperate++; // Very far away.
			
			// CONS_Printf("Dist to target: %d - throwdir: %d\n", dist/mapobjectscale, throwdir);
		}
	}

	cmd->bot.itemconfirm++;
	
	if (player->position < 2)		//Do NOT hold it in first place! That makes you blow up!
	{
		hold = false;
	}
	else if (target != NULL)		//we're currently trying to aim and fire at someone
	{
		if (player->playerringgunpower >= 110 || desperate > 1)	//If we've stored up this much power and still haven't fired, just let go.
			hold = false;
		else
			hold = true;		
	}
	else			//we do NOT have a target, and are holding the button to build up speed and firepower.
	{
		// We'll hold if we took too long, we're slow or not in first.
		hold = (player->botvars.itemconfirm > 10*TICRATE) || desperate < 1 || (player->position > 1);	
	}

	if (hold == true)
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemBallhog(const player_t *player, ticcmd_t *cmd)

		Item usage for Ballhog throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemBallhog(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(2304 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = 1;
	UINT8 snipeMul = 2;
	player_t *target = NULL;
	boolean hold = false;
	boolean desperate = false;
	
	// Too slow, use the item as a last ditch-effort to keep going.
	if (player->speed < ((topspeed/4)*3)
		|| (player->offroad && K_ApplyOffroad(player)))
		desperate = true;

	if (desperate == false)
	{
		if (player->speed > topspeed)
		{
			radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
			snipeMul = 3; // Confirm faster when you'll throw it with a bunch of extra speed!!
		}

		target = K_PlayerInCone(player, radius, 15, false);
		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
			
			fixed_t dist = P_AproxDistance(P_AproxDistance(
				player->mo->x - target->mo->x,
				player->mo->y - target->mo->y),
				(player->mo->z - target->mo->z) / 4
			);
			
			if (dist < ((radius/4)*2)) throwdir = -1; // REALLY close.
			else if (dist < ((radius/4)*3)) throwdir = 0; // Medium distance.
			else throwdir = 1; // Very far away.
			
			// CONS_Printf("Dist to target: %d - throwdir: %d\n", dist/mapobjectscale, throwdir);
		}
	}

	cmd->bot.itemconfirm++;
	
	if (target != NULL)
	{
		// We have a target, we'll hold a volley and release
		// when it's fully charged.
		INT32 ballhogmax = player->itemamount * BALLHOGINCREMENT;
		if (player->ballhogcharge >= ballhogmax)
			hold = false;
		else
			hold = true;
	}
	else
	{
		// We'll hold if we took too long, we're slow or first.
		hold = (player->botvars.itemconfirm > 10*TICRATE) || desperate || (player->position == 1);
	}

	if (hold == true)
	{
		cmd->throwdir = KART_FULLTURN * throwdir;
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemDropTarget(const player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for Drop Target throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemDropTarget(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = -1;
	boolean tryLookback = false;
	UINT8 snipeMul = 2;
	player_t *target = NULL;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
		snipeMul = 3; // Confirm faster when you'll throw it with a bunch of extra speed!!
	}

	cmd->bot.itemconfirm++;

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		cmd->bot.itemconfirm += player->botvars.difficulty / 2;
		throwdir = -1;
	}

	target = K_PlayerInCone(player, radius, 15, false);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
		throwdir = 1;
	}
	else
	{
		target = K_PlayerInCone(player, radius, 15, true);

		if (target != NULL)
		{
			K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
			throwdir = -1;
			tryLookback = true;
		}
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE || player->bananadrag >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemJawz(const player_t *player, ticcmd_t *cmd)

		Item usage for Jawz throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemJawz(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(2560 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = 1;
	boolean tryLookback = false;
	UINT8 snipeMul = 2;
	INT32 lastTarg = player->lastjawztarget;
	INT32 lastABTarg = player->lastabjawztarget;		//SCS ADD
	player_t *target = NULL;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
		snipeMul = 3; // Confirm faster when you'll throw it with a bunch of extra speed!!
	}

	cmd->bot.itemconfirm++;

	target = K_PlayerInCone(player, radius, 15, true);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty);
		throwdir = -1;
		tryLookback = true;
	}

	if (lastTarg != -1
		&& playeringame[lastTarg] == true
		&& players[lastTarg].spectator == false
		&& players[lastTarg].mo != NULL
		&& P_MobjWasRemoved(players[lastTarg].mo) == false)
	{
		target = &players[lastTarg];

		if (G_SameTeam(player, target) == false)
		{
			mobj_t *targMo = players[lastTarg].mo;
			mobj_t *mobj = NULL, *next = NULL;
			boolean targettedAlready = false;

			// Make sure no other Jawz are targetting this player.
			for (mobj = trackercap; mobj; mobj = next)
			{
				next = mobj->itnext;

				if ((mobj->type == MT_JAWZ || mobj->type == MT_AFTERBURNER_JAWZ) && mobj->target == targMo)		//SCS EDIT
				{
					targettedAlready = true;
					break;
				}
			}

			if (targettedAlready == false)
			{
				K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
				throwdir = 1;
			}
		}
	}
	else if (lastABTarg != -1
	&& playeringame[lastABTarg] == true
	&& players[lastABTarg].spectator == false
	&& players[lastABTarg].mo != NULL
	&& P_MobjWasRemoved(players[lastABTarg].mo) == false)
	{
		target = &players[lastABTarg];

		if (G_SameTeam(player, target) == false)
		{
			mobj_t *targMo = players[lastABTarg].mo;
			mobj_t *mobj = NULL, *next = NULL;
			boolean targettedAlready = false;

			// Make sure no other Jawz are targetting this player.
			for (mobj = trackercap; mobj; mobj = next)
			{
				next = mobj->itnext;

				if ((mobj->type == MT_JAWZ || mobj->type == MT_AFTERBURNER_JAWZ) && mobj->target == targMo)		//SCS EDIT
				{
					targettedAlready = true;
					break;
				}
			}

			if (targettedAlready == false)
			{
				K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
				throwdir = 1;
			}
		}
	}

	if (tryLookback == true && throwdir == -1)
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	if (player->botvars.itemconfirm > 10*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemLightning(const player_t *player, ticcmd_t *cmd)

		Item usage for Lightning Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemLightning(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	fixed_t radius = 192 * player->mo->scale;
	radius = Easing_Linear(FRACUNIT * player->botvars.difficulty / MAXBOTDIFFICULTY, 2*radius, 4*radius/3);

	if (K_BotUseItemNearPlayer(player, cmd, radius) == false)
	{
		if (player->botvars.itemconfirm > 10*TICRATE)
		{
			K_BotGenericPressItem(player, cmd, 0);
		}
		else
		{
			cmd->bot.itemconfirm++;
		}
	}
}

/*--------------------------------------------------
	static void K_BotItemBubble(const player_t *player, ticcmd_t *cmd)

		Item usage for Bubble Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemBubble(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	boolean hold = false;

	if (player->botvars.itemconfirm > 10*TICRATE)
		hold = true;

	if (player->bubbleblowup <= 0)
	{
		UINT8 i;

		cmd->bot.itemconfirm++;

		if (player->bubblecool <= 0)
		{
			fixed_t radius = 192 * player->mo->scale;
			radius = Easing_Linear(FRACUNIT * player->botvars.difficulty / MAXBOTDIFFICULTY, 2*radius, 4*radius/3);

			for (i = 0; i < MAXPLAYERS; i++)
			{
				player_t *target = NULL;
				fixed_t dist = INT32_MAX;

				if (!playeringame[i])
				{
					continue;
				}

				target = &players[i];

				if (target->mo == NULL || P_MobjWasRemoved(target->mo)
					|| player == target || target->spectator
					|| G_SameTeam(player, target)
					|| target->flashing)
				{
					continue;
				}

				dist = P_AproxDistance(P_AproxDistance(
					player->mo->x - target->mo->x,
					player->mo->y - target->mo->y),
					(player->mo->z - target->mo->z) / 4
				);

				if (dist <= radius)
				{
					hold = true;
					break;
				}
			}
		}
	}
	else if (player->bubbleblowup >= bubbletime)
	{
		if (player->botvars.itemconfirm > 10*TICRATE)
		{
			hold = true;
		}
	}
	else if (player->bubbleblowup < bubbletime)
	{
		hold = true;
	}

	if (hold && (player->itemflags & IF_HOLDREADY))
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemFlame(const player_t *player, ticcmd_t *cmd)

		Item usage for Flame Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemFlame(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->botvars.itemconfirm > 0)
	{
		cmd->bot.itemconfirm--;
	}
	else if (player->itemflags & IF_HOLDREADY)
	{
		INT32 flamemax = player->flamelength;

		if (player->flamemeter < flamemax || flamemax == 0)
		{
			cmd->buttons |= BT_ATTACK;
		}
		else
		{
			//player->botvars.itemconfirm = 3*flamemax/4;
		}
	}
}

/*--------------------------------------------------
	static void K_BotItemGardenTopDeploy(const player_t *player, ticcmd_t *cmd)

		Item usage for deploying the Garden Top.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGardenTopDeploy(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	cmd->bot.itemconfirm++;

	//if (player->curshield != KSHIELD_TOP)
	if (player->botvars.itemconfirm > 2*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, 0);
	}
}

/*--------------------------------------------------
	static void K_BotItemGardenTop(const player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for Garden Top movement.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGardenTop(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	fixed_t radius = FixedMul(2560 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	SINT8 throwdir = -1;
	UINT8 snipeMul = 1;
	player_t *target = NULL;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
		snipeMul = 2; // Confirm faster when you'll throw it with a bunch of extra speed!!
	}

	cmd->bot.itemconfirm++;

	target = K_PlayerInCone(player, radius, 15, false);
	if (target != NULL)
	{
		K_ItemConfirmForTarget(player, cmd, target, player->botvars.difficulty * snipeMul);
		throwdir = 1;
	}

	if (player->topdriftheld > 0)
	{
		// Grinding in place.
		// Wait until we're mostly done turning.
		// Cancel early if we hit max thrust speed.
		if ((abs(turnamt) >= KART_FULLTURN/8)
			&& (player->topdriftheld <= GARDENTOP_MAXGRINDTIME))
		{
			cmd->buttons |= BT_DRIFT;
		}
	}
	else
	{
		const angle_t maxDelta = ANGLE_11hh;
		angle_t delta = AngleDelta(player->mo->angle, K_MomentumAngle(player->mo));

		if (delta > maxDelta)
		{
			// Do we need to turn? Start grinding!
			cmd->buttons |= BT_DRIFT;
		}
	}

	if (player->botvars.itemconfirm > 25*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemNormShield(const player_t *player, ticcmd_t *cmd)			//SCS ADD

		Item usage for Normal Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemNormShield(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t topspeed = K_GetKartSpeed(player, false, true);
	boolean hold = true;
	SINT8 desperate = 0;
	UINT8 i;
	
	if (player->normalshieldboostcharge >= (5 * NORMSHIELDINCREMENT))
	{
		return;		//If we're at max charge, let go already!
	}
	
	// Too slow, use the item as a last ditch-effort to keep going.
	if (player->speed < ((topspeed/4)*2))
		desperate++;
	
	if (player->offroad && K_ApplyOffroad(player))	//if we're offroad, that's a good reason to start charging
		desperate++;

	if (desperate <= 1 && player->normalshieldboostcharge <= 0)	//if we're not desperate to use the shield, then...check surroundings if we aren't already charging it
	{
		fixed_t radius = 204 * player->mo->scale;
		radius = Easing_Linear(FRACUNIT * player->botvars.difficulty / MAXBOTDIFFICULTY, 2*radius, 4*radius/3);
			
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *target = NULL;
			fixed_t dist = INT32_MAX;

			if (!playeringame[i])
			{
				continue;
			}

			target = &players[i];

			if (target->mo == NULL || P_MobjWasRemoved(target->mo)
				|| player == target || target->spectator
				|| G_SameTeam(player, target)
				|| target->flashing || player->growshrinktimer > 0 || player->invincibilitytimer > 0)		//disregard smaller players with guns - they can't do anything
			{
				continue;
			}

			dist = P_AproxDistance(P_AproxDistance(
				player->mo->x - target->mo->x,
				player->mo->y - target->mo->y),
				(player->mo->z - target->mo->z) / 4
			);

			if (dist <= radius && (target->itemtype == KITEM_CHAMBLASTER || target->itemtype == KITEM_EGGBLASTER 
			|| target->itemtype == KITEM_RINGGUN))		//if we detect anyone nearby that has a gun item, keep the shield on - shields protect from bullets
			{
				hold = false;
				break;
			}
		}
	}
	else if (player->normalshieldboostcharge > 0)		//We're already charging, figure out when to let go!
	{
		if (player->botvars.difficulty < 5)
		{
			if (player->normalshieldboostcharge >= (2 * NORMSHIELDINCREMENT))
				hold = false;
				
		}
		else if (player->botvars.difficulty < 8)
		{
			if (player->normalshieldboostcharge >= (3 * NORMSHIELDINCREMENT))
				hold = false;
				
		}
		else
		{
			if (player->normalshieldboostcharge >= (4 * NORMSHIELDINCREMENT) && leveltime % 45 == 0)
				hold = false;
				
		}
	}

	if (hold == true)
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemPickpocket(const player_t *player, ticcmd_t *cmd)			//SCS ADD

		Item usage for Pickpocket Hyudoro

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemPickpocket(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (player->rings <= 0 || (player->offroad && K_ApplyOffroad(player)))		//if you're in ring debt, no questions: the immediate pay is more important. Offroad, too.
	{
		cmd->buttons |= BT_ATTACK;
		return;
	}
	
	if (player->invincibilitytimer > 0 || player->hyudorotimer || player->growshrinktimer > 0)		//basically, if we're invincible, don't even bother checking around - there's not too much that could ruin the combo.
		return;

	fixed_t radius = 245 * player->mo->scale;
	radius = Easing_Linear(FRACUNIT * player->botvars.difficulty / MAXBOTDIFFICULTY, 2*radius, 4*radius/3);
	SINT8 nearbyplayers = 0, loops = 0;
	UINT8 i;
		
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;
		
		loops++;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];

		if (target->mo == NULL || P_MobjWasRemoved(target->mo)
			|| player == target || target->spectator
			|| G_SameTeam(player, target)
			|| target->flashing)
		{
			continue;
		}

		dist = P_AproxDistance(P_AproxDistance(
			player->mo->x - target->mo->x,
			player->mo->y - target->mo->y),
			(player->mo->z - target->mo->z) / 4
		);
		
		if (dist <= radius)
			nearbyplayers++;
		
		if (target->invincibilitytimer > 0 || target->growshrinktimer > 0)		//if there's something really dangerous neraby, don't chance it - lock in the current combo
		{
			cmd->buttons |= BT_ATTACK;
			break;
		}
		else if (target->stoneShoe != NULL)			//Stone Shoe chain can cause a lot of havoc, but is also easy to hit yourself. Not worth the risk.
		{
			cmd->buttons |= BT_ATTACK;
			break;			
		}
		
		if (target->itemtype != KITEM_NONE)
		{
			if (target->itemtype == KITEM_CHAMBLASTER || target->itemtype == KITEM_EGGBLASTER) //if we detect anyone nearby that has a gun item, lock in the combo - there's a chance that there could be a lot of bullets flying around.
			{
				cmd->buttons |= BT_ATTACK;
				break;
			}			
			else if (target->itemtype == KITEM_WRECKINGBALL)			//far too dangerous lol
			{
				cmd->buttons |= BT_ATTACK;
				break;				
			}
		}
	}

	if (nearbyplayers < (loops/2))			//if there aren't enough players around us to try and combo off of, just lock in what you've got.
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemRings(const player_t *player, ticcmd_t *cmd)

		Item usage for rings.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRings(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	INT32 overdrivepreference = player->amps/3;
	const fixed_t topspeed = K_GetKartSpeed(player, false, true);		//SCS ADD
	if (player->position <= 1)
		overdrivepreference = 0;

	//INT32 saferingsval = 16 - K_GetKartRingPower(player, false) - overdrivepreference;
	INT32 saferingsval = 20 - K_GetKartRingPower(player, false) - overdrivepreference;			//SCS EDIT - Do they really need to be below 20 rings no matter what?
	
	
	if (player->masteremeraldinvincibility)					//SCS ADD
	{
		//Don't use rings when using the Master Emerald!
		return;
	}
	
	if (player->superring && (player->speed < ((topspeed/4)*2) && !(player->offroad && K_ApplyOffroad(player))))		//SCS ADD
	{
		//If we have a ring payout, for the love of God, stop grinding it each and every single time
		return;
	}
	
	if (player->inkblotchtimer > 5 && leveltime % 2 == 0)
	{
		//fake them being cautious about using rings when blinded
		return;
	}

	if (leveltime < starttime)
	{
		// Don't use rings during POSITION!!
		return;
	}

	if ((cmd->buttons & BT_ACCELERATE) == 0)
	{
		// Don't use rings if you're not trying to accelerate.
		return;
	}

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// Don't use while mid-air.
		return;
	}

	if (player->speed < (K_GetKartSpeed(player, false, true) * 9) / 10 // Being slowed down too much
		|| player->speedboost > (FRACUNIT/5)) // Have another type of boost (tethering)
	{
		saferingsval -= 5;
	}

	if (player->rings > saferingsval || (player->timestonefrozen && player->timestonefrozenringamount > 0))			//SCS EDIT - Make bots use rings while using a Time Stone. It DOES protect them from losing rings if hit,
	{																										//but otherwise they just kinda slow down while using it otherwise
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemInstashield(const player_t *player, ticcmd_t *cmd)

		Item usage for instashield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemInstashield(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const fixed_t radius = FixedMul(mobjinfo[MT_INSTAWHIP].radius, player->mo->scale);
	size_t i = SIZE_MAX;

	boolean nearbyThreat = false; // Someone's near enough to worry about, start charging.
	boolean attackOpportunity = false; // Someone's close enough to hit!
	boolean coastIsClear = true; // Nobody is nearby, let any pending charge go.

	UINT8 stupidRating = MAXBOTDIFFICULTY - player->botvars.difficulty;
	// Weak bots take a second to react on offense.
	UINT8 reactiontime = stupidRating;
 	// Weak bots misjudge their attack range. Purely accurate at Lv.MAX, 250% overestimate at Lv.1
	fixed_t radiusWithError = radius + 3*(radius * stupidRating / MAXBOTDIFFICULTY)/2;

	// Future work: Expand threat range versus fast pursuers.

	if (leveltime < starttime || player->spindash || player->defenseLockout)
	{
		// Instashield is on cooldown.
		return;
	}

	if (player->botvars.difficulty <= 7)
	{
		// Weak players don't whip.
		// Weak bots don't either.
		return;
	}

	// Find players within the instashield's range.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];
		if (P_MobjWasRemoved(target->mo) == true
			|| player == target
			|| target->spectator == true
			|| G_SameTeam(player, target) == true
			|| target->flashing != 0)
		{
			continue;
		}

		dist = P_AproxDistance(P_AproxDistance(
			player->mo->x - target->mo->x,
			player->mo->y - target->mo->y),
			(player->mo->z - target->mo->z) / 4
		);

		if (dist <= 8 * radius)
		{
			coastIsClear = false;
		}

		if (dist <= 5 * radius)
		{
			nearbyThreat = true;
		}

		if (dist <= (radiusWithError + target->mo->radius))
		{
			attackOpportunity = true;
			K_ItemConfirmForTarget(player, cmd, target, 1);
		}
	}

	if (player->instaWhipCharge) // Already charging, do we stay committed?
	{
		cmd->buttons |= BT_ATTACK; // Keep holding, unless...

		// ...there are no attackers that are even distantly threatening...
		if (coastIsClear)
			cmd->buttons &= ~BT_ATTACK;

		// ...or we're ready to rock.
		if (attackOpportunity && player->instaWhipCharge >= (INSTAWHIP_CHARGETIME + reactiontime) && player->botvars.itemconfirm >= reactiontime)
			cmd->buttons &= ~BT_ATTACK;
	}
	else // When should we get spooked and start a charge?
	{
		if (nearbyThreat)
			cmd->buttons |= BT_ATTACK;
	}

}

/*--------------------------------------------------
	static void K_BotItemIceCube(player_t *player, ticcmd_t *cmd)

		Item usage for ice cubes.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemIceCube(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	if (leveltime % 7)
	{
		return;
	}

	if (player->sneakertimer || player->weaksneakertimer)
	{
		return;
	}

	if (!P_IsObjectOnGround(player->mo))
	{
		return;
	}

	cmd->buttons |= BT_DRIFT;
}

/*--------------------------------------------------
	static tic_t K_BotItemRouletteMashConfirm(const player_t *player)

		How long this bot waits before selecting an item for
		the item roulette.

	Input Arguments:-
		player - Bot to do this for.

	Return:-
		Time to wait, in tics.
--------------------------------------------------*/
static tic_t K_BotItemRouletteMashConfirm(const player_t *player)
{
	// 24 tics late for Lv.1, frame-perfect for Lv.MAX
	return (MAXBOTDIFFICULTY - player->botvars.difficulty) * 2;
}

/*--------------------------------------------------
	static void K_BotItemRouletteMash(const player_t *player, ticcmd_t *cmd)

		Item usage for item roulette mashing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRouletteMash(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const tic_t confirmTime = K_BotItemRouletteMashConfirm(player);

	if (K_ItemButtonWasDown(player) == true)
	{
		return;
	}

	if (player->botvars.itemconfirm > confirmTime)
	{
		if (player->inkblotchtimer > 5 && leveltime % 2 == 0)		//SCS ADD - throw off the CPUs a little more if they're meant to have their item slot blinded
			return;
		
		if ((player->itemRoulette.ringbox == true && leveltime % 23 == 0) 
		|| player->itemRoulette.ringbox == false)			//SCS ADD - add a little variance on how bots select their result in the slots; max levels always seems to pick RING, which isn't the best.
		{
			// We've waited out our reaction time -- press the button now!
			cmd->buttons |= BT_ATTACK;
		}
	}
}

/*--------------------------------------------------
	void K_BotItemUsage(const player_t *player, ticcmd_t *cmd, INT16 turnamt)

		See header file for description.
--------------------------------------------------*/
void K_BotItemUsage(const player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	ZoneScoped;

	if (player->icecube.frozen)
	{
		K_BotItemIceCube(player, cmd);
		return;
	}

	if (player->itemflags & IF_USERINGS)
	{
		if (player->rings > 0)
		{
			// Use rings!
			K_BotItemRings(player, cmd);
		}
		else
		{
			// Use the instashield!
			K_BotItemInstashield(player, cmd);
		}
	}
	else
	{
		if (player->botvars.itemdelay)
		{
			return;
		}

		if (player->itemRoulette.active == true)
		{
			// Mashing behaviors
			K_BotItemRouletteMash(player, cmd);
			return;
		}

		if (player->stealingtimer == 0)
		{
			if (player->eggmanexplode)
			{
				K_BotItemEggmanExplosion(player, cmd);
			}
			else if (player->itemflags & IF_EGGMANOUT)
			{
				K_BotItemEggman(player, cmd);
			}
			else if (player->rocketsneakertimer > 0)
			{
				K_BotItemRocketSneaker(player, cmd);
			}
			else if (player->megachoppertimer > 0)			//SCS ADD
			{
				K_BotItemMegaChopper(player, cmd);
			}
			else if (player->gunusagetimer && player->itemtype == KITEM_CHAMBLASTER)	//SCS ADD
			{
				K_BotItemChameleonBlaster(player, cmd);
			}
			else if (player->armageddonshieldboosttimer > 1)	//SCS ADD
			{
				K_BotItemArmageddonShield(player, cmd);
			}
			else
			{
				switch (player->itemtype)
				{
					default:
						if (player->itemtype != KITEM_NONE)
						{
							K_BotItemGenericTap(player, cmd);
						}

						//player->botvars.itemconfirm = 0;
						break;
					case KITEM_INVINCIBILITY:
					case KITEM_SPB:
					case KITEM_GROW:
					case KITEM_SHRINK:
					case KITEM_SUPERRING:
					case KITEM_SUPERJACKPOT:					//SCS ADD
					case KITEM_WRECKINGBALL:					//SCS ADD
						K_BotItemGenericTap(player, cmd);
						break;
					case KITEM_MASTEREMERALD:					//SCS ADD
						if (!(player->superring))
						{
							K_BotItemGenericTap(player, cmd);
						}
						break;
					case KITEM_ROCKETSNEAKER:
						if (player->rocketsneakertimer <= 0)
						{
							K_BotItemGenericTap(player, cmd);
						}
						break;
					case KITEM_SNEAKER:
						K_BotItemSneaker(player, cmd);
						break;
					case KITEM_BANANA:
						if (!(player->itemflags & IF_ITEMOUT))
						{
							K_BotItemGenericTrapShield(player, cmd, turnamt, false);
						}
						else
						{
							K_BotItemBanana(player, cmd, turnamt);
						}
						break;
					case KITEM_EGGBLASTER:							//SCS ADD
					case KITEM_GACHABOM:							//SCS ADD
						K_BotItemBanana(player, cmd, turnamt);
						break;
					case KITEM_EGGMAN:
						K_BotItemEggmanShield(player, cmd);
						break;
					case KITEM_ORBINAUT:
						if (!(player->itemflags & IF_ITEMOUT))
						{
							K_BotItemGenericOrbitShield(player, cmd);
						}
						else if (player->position != 1) // Hold onto orbiting items when in 1st :)
						{
							K_BotItemOrbinaut(player, cmd);
						}
						break;
					case KITEM_JAWZ:
						if (!(player->itemflags & IF_ITEMOUT))
						{
							K_BotItemGenericOrbitShield(player, cmd);
						}
						else if (player->position != 1) // Hold onto orbiting items when in 1st :)
						{
							K_BotItemJawz(player, cmd);
						}
						break;
					case KITEM_ABURNERJAWZ:												//SCS ADD
						if (player->position != 1) // Hold onto this when in 1st :)
						{
							K_BotItemJawz(player, cmd);
						}					
						break;
					case KITEM_MINE:
						if (!(player->itemflags & IF_ITEMOUT))
						{
							K_BotItemGenericTrapShield(player, cmd, turnamt, true);
						}
						else
						{
							K_BotItemMine(player, cmd, turnamt);
						}
						break;
					case KITEM_LANDMINE:
					case KITEM_HYUDORO: // Function re-use, as they have about the same usage.
					case KITEM_PRESSUREMINE:														//SCS ADD
					case KITEM_BUTLERHYU:															//SCS ADD
					case KITEM_OCTUS:
						K_BotItemLandmine(player, cmd, turnamt);
						break;
					case KITEM_PICKPOCKETHYU:														//SCS ADD
						//K_BotItemPickpocket(player, cmd);
						K_BotItemLandmine(player, cmd, turnamt);			//Temp
						break;
					case KITEM_BALLHOG:
						K_BotItemBallhog(player, cmd);
						break;
					case KITEM_RINGGUN:								//SCS ADD
						K_BotItemRingGun(player, cmd);
						break;
					case KITEM_DROPTARGET:
						if (!(player->itemflags & IF_ITEMOUT))
						{
							K_BotItemGenericTrapShield(player, cmd, turnamt, false);
						}
						else
						{
							K_BotItemDropTarget(player, cmd, turnamt);
						}
						break;
					case KITEM_GARDENTOP:
						if (player->curshield != KSHIELD_TOP)
						{
							K_BotItemGardenTopDeploy(player, cmd);
						}
						else
						{
							K_BotItemGardenTop(player, cmd, turnamt);
						}
						break;
					case KITEM_LIGHTNINGSHIELD:
						K_BotItemLightning(player, cmd);
						break;
					case KITEM_BUBBLESHIELD:
						K_BotItemBubble(player, cmd);
						break;
					case KITEM_FLAMESHIELD:
						K_BotItemFlame(player, cmd);
						break;
					case KITEM_NORMALSHIELD:					//SCS ADD
						K_BotItemNormShield(player, cmd);
						break;
				}
			}
		}
	}
}

/*--------------------------------------------------
	static void K_UpdateBotGameplayVarsItemUsageMash(player_t *player)

		Thinker function used by K_UpdateBotGameplayVarsItemUsage for
		deterimining item rolls.
--------------------------------------------------*/
static void K_UpdateBotGameplayVarsItemUsageMash(player_t *player)
{
	const tic_t confirmTime = K_BotItemRouletteMashConfirm(player);

	if (player->botvars.roulettePriority == BOT_ITEM_PR__FALLBACK)
	{
		// No items were part of our list, so set immediately.
		player->botvars.itemconfirm = confirmTime + 1;
	}
	else if (player->botvars.itemconfirm > 0)
	{
		// Delaying our reaction time a bit...
		player->botvars.itemconfirm++;
	}
	else
	{
		botItemPriority_e currentPriority = K_GetBotItemPriority(
			static_cast<kartitems_t>( player->itemRoulette.itemList.items[ player->itemRoulette.index ] )
		);

		if (player->botvars.roulettePriority == currentPriority)
		{
			// This is the item we want! Start timing!
			player->botvars.itemconfirm++;
		}
		else
		{
			// Not the time we want... if we take too long,
			// reduce priority until we get to a valid one.
			player->botvars.rouletteTimeout++;

			if (player->botvars.rouletteTimeout > player->itemRoulette.itemList.len * player->itemRoulette.speed)
			{
				player->botvars.roulettePriority--;
				player->botvars.rouletteTimeout = 0;
			}
		}
	}
}

/*--------------------------------------------------
	void K_UpdateBotGameplayVarsItemUsage(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_UpdateBotGameplayVarsItemUsage(player_t *player)
{
	if (player->itemflags & IF_USERINGS && !player->instaWhipCharge)
	{
		return;
	}

	if (player->botvars.itemdelay)
	{
		player->botvars.itemdelay--;
		player->botvars.itemconfirm = 0;
		return;
	}

	if (player->cmd.bot.itemconfirm < 0 && abs(player->cmd.bot.itemconfirm) > player->botvars.itemconfirm)
	{
		player->botvars.itemconfirm = 0;
	}
	else
	{
		player->botvars.itemconfirm += player->cmd.bot.itemconfirm;
	}

	if (player->itemflags & IF_USERINGS)
	{
		;
	}
	else
	{
		if (player->itemRoulette.active == true)
		{
			// Mashing behaviors
			K_UpdateBotGameplayVarsItemUsageMash(player);
			return;
		}

		if (player->stealingtimer == 0)
		{
			if (player->eggmanexplode)
			{
				;
			}
			else if (player->itemflags & IF_EGGMANOUT)
			{
				;
			}
			else if (player->rocketsneakertimer > 0)
			{
				;
			}
			else if (player->megachoppertimer > 0)			//SCS ADD
			{
				;
			}
			else
			{
				switch (player->itemtype)
				{
					default:
					{
						break;
					}
					case KITEM_FLAMESHIELD:
					{
						if (player->botvars.itemconfirm == 0
							&& (player->itemflags & IF_HOLDREADY) == IF_HOLDREADY)
						{
							INT32 flamemax = player->flamelength;

							if (player->flamemeter < flamemax || flamemax == 0)
							{
								;
							}
							else
							{
								player->botvars.itemconfirm = (3 * flamemax / 4) + (TICRATE / 2);
							}
						}
						break;
					}
				}
			}
		}
	}
}

/*--------------------------------------------------
	void K_BotPickItemPriority(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_BotPickItemPriority(player_t *player)
{
	ZoneScoped;

	const fixed_t closeDistance = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	size_t i;

	// Roulette reaction time. This is how long to wait before considering items.
	// Takes 3 seconds for Lv.1, is instant for Lv.MAX
	player->botvars.itemdelay = ((MAXBOTDIFFICULTY - player->botvars.difficulty) * BOT_ITEM_DECISION_TIME) / (MAXBOTDIFFICULTY - 1);
	player->botvars.itemconfirm = 0;

	// Set neutral items by default.
	player->botvars.roulettePriority = BOT_ITEM_PR_NEUTRAL;
	player->botvars.rouletteTimeout = 0;

	// Check for items that are extremely high priority.
	for (i = 0; i < player->itemRoulette.itemList.len; i++)
	{
		botItemPriority_e priority = K_GetBotItemPriority( static_cast<kartitems_t>( player->itemRoulette.itemList.items[i] ) );

		if (priority < BOT_ITEM_PR__OVERRIDES)
		{
			// Not high enough to override.
			continue;
		}

		if (priority == BOT_ITEM_PR_RINGDEBT)
		{
			if (player->rings > 0)
			{
				// Only consider this priority when in ring debt.
				continue;
			}
		}

		player->botvars.roulettePriority = std::max( static_cast<botItemPriority_e>( player->botvars.roulettePriority ), priority );
	}

	if (player->botvars.roulettePriority >= BOT_ITEM_PR__OVERRIDES)
	{
		// Selected a priority in the loop above.
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *other = NULL;
		fixed_t distance = INT32_MAX;

		if (playeringame[i] == false)
		{
			continue;
		}

		other = &players[i];
		if (other->spectator == true || P_MobjWasRemoved(other->mo) == true)
		{
			continue;
		}

		distance = P_AproxDistance(
			P_AproxDistance(
				other->mo->x - player->mo->x,
				other->mo->y - player->mo->y
			),
			other->mo->z - player->mo->z
		);

		if (distance < closeDistance)
		{
			// A player is relatively close.
			break;
		}
	}

	if (i == MAXPLAYERS)
	{
		// Players are nearby, stay as neutral priority.
		return;
	}

	// Players are far away enough to give you breathing room.
	if (player->position == 1)
	{
		// Frontrunning, so pick frontrunner items!
		player->botvars.roulettePriority = BOT_ITEM_PR_FRONTRUNNER;
	}
	else
	{
		// Behind, so pick speed items!
		player->botvars.roulettePriority = BOT_ITEM_PR_SPEED;
	}
}
