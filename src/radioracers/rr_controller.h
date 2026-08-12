// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_controller.h
/// \brief Extended controller rumble events

#ifndef RR_CONTROLLER_H
#define RR_CONTROLLER_H

#include "../doomtype.h"    // uint16_t
#include "../d_player.h"    // player_t

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EVT_WALL = 1,
    EVT_RING,
    EVT_SPRUNG,
    EVT_DRIFT,
    EVT_WAVEDASHED,
    EVT_WHIP_CHARGING,
    EVT_WHIP_OVERCHARGING,
    EVT_SPINDASH_GENTLE,
    EVT_SPINDASH_VIOLENT
} rumbleevent_e ;

extern dboolean RR_ShouldUseMoreRumbleEvents(void);
extern rumbleevent_e RR_GetRumbleEvent(const player_t *player);
extern uint16_t RR_GetRumbleStrength(const rumbleevent_e event);

// All the event checks
dboolean RR_RumbleJustBumpedWall(const player_t *player);                    // Bumped Wall
dboolean RR_RumbleIsSpindashingGently(const player_t *player);               // Spindashing Gently
dboolean RR_RumbleIsSpindashingViolently(const player_t *player);            // Spindashing Violently
dboolean RR_RumbleRingCollected(const player_t *player);                     // Ring picked up
dboolean RR_RumbleRingConsumed(const player_t *player);                      // Ring used
dboolean RR_RumbleJustDrifted(const player_t *player);                       // Drifts
dboolean RR_RumbleJustFastFallBounced(const player_t *player);               // Fast Fall Bounce
dboolean RR_RumbleJustWaveDashed(const player_t *player);                    // Wavedash/Sliptide
dboolean RR_RumbleChargingTailWhip(const player_t *player);                  // Charging Tailwhip
dboolean RR_RumbleOverchargingTailWhip(const player_t *player);              // Overcharging Tailwhip

// Extra variables for checking events for rumble triggers
extern dboolean localPlayerJustBootyBounced;
extern int16_t localPlayerWavedashClickTimer;

// mimicking `pickuprings` behaviour
extern uint8_t localPlayerPickupSpheres; 
extern int16_t localPlayerPickupSpheresDelay;

#ifdef __cplusplus
} // extern "C"
#endif

#endif