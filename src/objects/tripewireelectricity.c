// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  tripwireelectricity.c
/// \brief Unused Tripwire VFX code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"
#include "../m_random.h"


void Obj_TripWireEffects (mobj_t *tripwire)
{
    if (P_MobjWasRemoved(tripwire->target)
		|| tripwire->target->health == 0
		|| !tripwire->target->player)
    {
		if (tripwire->fuse < 25)
			P_RemoveMobj(tripwire);
		
    }
    else
    {
        mobj_t *mo = tripwire->target;

		if (mo)
		{
			tripwire->flags &= ~(MF_NOCLIPTHING);
			P_MoveOrigin(tripwire, mo->x, mo->y, mo->z + mo->height/2);
			tripwire->flags |= MF_NOCLIPTHING;
			P_SetScale(tripwire, (mo->scale*3)/2);
		}
        //tripwire->color = mo->color;

        //tripwire->renderflags &= ~RF_DONTDRAW;
		
		if (tripwire->fuse < 3 && tripwire->target != NULL)
		{
			if (tripwire->type == MT_TRIPWIRELOCKOUT)
				tripwire->target->player->tripwirelockout = NULL;
			else
				tripwire->target->player->tripwireok = NULL;
			
			tripwire->target = NULL;
		}
    }
}