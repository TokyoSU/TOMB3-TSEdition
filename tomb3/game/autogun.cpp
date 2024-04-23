#include "../tomb3/pch.h"
#include "autogun.h"
#include "objects.h"
#include "../specific/game.h"
#include "../3dsystem/3d_gen.h"
#include "sphere.h"
#include "effect2.h"
#include "box.h"
#include "missile.h"
#include "lot.h"
#include "items.h"
#include "control.h"
#include "people.h"
#include "sound.h"
#include "lara.h"

static BITE_INFO autogun_left = { 110, -30, -530, 2 };
static BITE_INFO autogun_right = { -110, -30, -530, 2 };
#define AUTOGUN_DAMAGE 5

void InitialiseAutogun(short item_number)
{
	ITEM_INFO* item = &items[item_number];
	item->anim_number = objects[ROBOT_SENTRY_GUN].anim_index + 1;
	item->frame_number = anims[item->anim_number].frame_base;
	item->current_anim_state = AUTOGUN_STILL;
	item->goal_anim_state = AUTOGUN_STILL;
	item->item_flags[0] = 0; // Still not fired !
}

void AutogunControl(short item_number)
{
	if (!CreatureActive(item_number))
		return;

	ITEM_INFO* item = &items[item_number];
	CREATURE_INFO* gun = GetCreatureInfo(item);
	if (gun == NULL)
		return;

	if (item->hit_status)
		item->really_active = TRUE;
	if (item->ai_bits & GUARD)
	{
		if (item->really_active == FALSE)
			item->really_active = TRUE;
		item->item_flags[1] = 1; // to get lara to avoid targeting autogun !
	}

	// Handle gunflash + light
	PHD_VECTOR pos = {};
	// NOTE: DrawAnimatingItem reduce the fired_weapon[] value itself, no need to do that here !
	if (item->fired_weapon[0] > 1) // LEFT
	{
		pos.x = autogun_left.x;
		pos.y = autogun_left.y;
		pos.z = autogun_left.z;
		GetJointAbsPosition(item, &pos, autogun_left.mesh_num);
		TriggerDynamic(pos.x, pos.y, pos.z, 2 * item->fired_weapon[0] + 8, 192, 128, 32);
	}
	if (item->fired_weapon[1] > 1) // RIGHT
	{
		pos.x = autogun_right.x;
		pos.y = autogun_right.y;
		pos.z = autogun_right.z;
		GetJointAbsPosition(item, &pos, autogun_right.mesh_num);
		TriggerDynamic(pos.x, pos.y, pos.z, 2 * item->fired_weapon[1] + 8, 192, 128, 32);
	}

	if (item->hit_points <= 0)
	{
		ExplodingDeath(item_number, -1, 0);
		DisableBaddieAI(item_number);
		KillItem(item_number);
		item->flags |= IFL_INVISIBLE;
		item->status = ITEM_DEACTIVATED;
	}
	else if (item->really_active)
	{
		if (item->ai_bits & GUARD)
		{
			ITEM_INFO* nearestTarget = GetNearTarget(item, SECTOR(8), RAPTOR);
			if (nearestTarget != NULL)
			{
				gun->enemy = nearestTarget;
				if (gun->hurt_by_lara)
					gun->enemy = lara_item;
			}
			else
			{
				gun->enemy = NULL;
			}
		}

		AI_INFO info;
		CreatureAIInfo(item, &info, !(item->ai_bits & GUARD), ANGLE(180)); // NOTE: target lara by default if AI_GUARD is not found !

		switch (item->current_anim_state)
		{
		case AUTOGUN_FIRE:
			if (!Targetable(item, &info))
			{
				item->goal_anim_state = AUTOGUN_STILL;
			}
			else if (item->frame_number == anims[item->anim_number].frame_base)
			{
				item->item_flags[0] = 1;
				ShotLara(item, &info, &autogun_left, info.angle, AUTOGUN_DAMAGE, TRUE, 1);
				ShotLara(item, &info, &autogun_right, info.angle, AUTOGUN_DAMAGE, FALSE, 1);
				SoundEffect(SFX_LARA_UZI_STOP, &item->pos, SFX_DEFAULT);
			}

			break;

		case AUTOGUN_STILL:
			if (Targetable(item, &info) && !item->item_flags[0])
			{
				item->goal_anim_state = AUTOGUN_FIRE;
			}
			else if (item->item_flags[0])
			{
				if (item->ai_bits & MODIFY)
				{
					item->item_flags[0] = 1;
					item->goal_anim_state = AUTOGUN_FIRE;
				}
				else
				{
					item->really_active = FALSE;
					item->item_flags[0] = 0;
				}
			}

			break;
		}

		CreatureJoint(item, 0, info.angle, ANGLE(10), ANGLE(90));
		CreatureJoint(item, 1, -info.x_angle);
		AnimateItem(item);
		if (info.angle > ANGLE(90))
		{
			item->pos.y_rot += ANGLE(180);
			if (info.angle > 0 || info.angle < 0)
				gun->joint_rotation[0] += ANGLE(180);
		}
		else if (info.angle < -ANGLE(90))
		{
			item->pos.y_rot += ANGLE(180);
			if (info.angle > 0 || info.angle < 0)
				gun->joint_rotation[0] += ANGLE(180);
		}
	}
}
