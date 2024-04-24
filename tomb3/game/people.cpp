#include "../tomb3/pch.h"
#include "people.h"
#include "../3dsystem/phd_math.h"
#include "../specific/game.h"
#include "items.h"
#include "sphere.h"
#include "objects.h"
#include "effect2.h"
#include "control.h"
#include "gameflow.h"
#include "larafire.h"
#include "effects.h"
#include "sound.h"
#include "box.h"
#include "../specific/smain.h"
#include "draw.h"
#include "lara.h"

short GunShot(long x, long y, long z, short speed, short yrot, short room_number)
{
	return -1;
}

short GunHit(long x, long y, long z, short speed, short yrot, short room_number)
{
	PHD_VECTOR pos;

	pos.x = 0;
	pos.y = 0;
	pos.z = 0;
	GetJointAbsPosition(lara_item, &pos, 25 * GetRandomControl() / 0x7FFF);
	DoBloodSplat(pos.x, pos.y, pos.z, lara_item->speed, lara_item->pos.y_rot, lara_item->room_number);
	SoundEffect(SFX_LARA_BULLETHIT, &lara_item->pos, SFX_DEFAULT);
	return GunShot(x, y, z, speed, yrot, room_number);
}

short GunMiss(long x, long y, long z, short speed, short yrot, short room_number)
{
	GAME_VECTOR pos;

	pos.x = lara_item->pos.x_pos + ((GetRandomControl() - 0x4000) << 9) / 0x7FFF;
	pos.y = lara_item->floor;
	pos.z = lara_item->pos.z_pos + ((GetRandomControl() - 0x4000) << 9) / 0x7FFF;
	pos.room_number = lara_item->room_number;
	Richochet(&pos);
	return GunShot(x, y, z, speed, yrot, room_number);
}

long ShotLara(ITEM_INFO* item, AI_INFO* info, BITE_INFO* bite, short extra_rotation, long damage, BOOL isLeft, ushort firedCount)
{
	ITEM_INFO* enemy;
	CREATURE_INFO* creature;
	PHD_VECTOR pos;
	GAME_VECTOR s;
	GAME_VECTOR t;
	long hit, targetable, dist, rnd, objLos;

	creature = (CREATURE_INFO*)item->data;
	enemy = creature->enemy;

	if (info->distance <= 0x4000000 && Targetable(item, info))
	{
		dist = 0x4000000 * ((enemy->speed * phd_sin(info->enemy_facing)) >> W2V_SHIFT) / 300;
		dist = info->distance + SQUARE(dist);

		if (dist <= 0x4000000)
		{
			rnd = (0x4000000 - info->distance) / 0xCCC + 0x2000;
			hit = (GetRandomControl() < rnd);
		}
		else
			hit = 0;

		targetable = 1;
	}
	else
	{
		targetable = 0;
		hit = 0;
	}

	item->fired_weapon[isLeft == TRUE ? 0 : 1] = firedCount;
	TriggerWeaponSmokeAndShell(item, bite);

	if (damage)
	{
		if (enemy == lara_item)
		{
			if (hit)
			{
				CreatureEffect(item, bite, GunHit);
				lara_item->hit_points -= (short)damage;
				lara_item->hit_status = 1;
			}
			else if (targetable)
				CreatureEffect(item, bite, GunMiss);
		}
		else
		{
			CreatureEffect(item, bite, GunShot);

			if (hit)
			{
				enemy->hit_status = 1;
				if (damage < 11)
					enemy->hit_points--;
				else
					enemy->hit_points -= short(damage / 10);
				rnd = GetRandomControl() & 0xF;

				if (rnd > 14)
					rnd = 0;

				pos.x = 0;
				pos.y = 0;
				pos.z = 0;
				GetJointAbsPosition(enemy, &pos, rnd);
				if (enemy->object_number == RAPTOR && (enemy->flags & IFL_CODEBITS_1))
					DoBloodSplatGreen(pos.x, pos.y, pos.z, enemy->speed, enemy->pos.y_rot, enemy->room_number);
				else
					DoBloodSplat(pos.x, pos.y, pos.z, enemy->speed, enemy->pos.y_rot, enemy->room_number);
			}
		}
	}

	s.x = item->pos.x_pos;
	s.y = item->pos.y_pos - 768;
	s.z = item->pos.z_pos;
	s.room_number = item->room_number;
	t.x = enemy->pos.x_pos;
	t.y = enemy->pos.y_pos - 768;
	t.z = enemy->pos.z_pos;

	objLos = ObjectOnLOS(&s, &t);
	if (objLos != NO_ITEM && (items[objLos].object_number != SMASH_OBJECT1 || CurrentLevel != LV_CRASH))
		SmashItem((short)objLos, 0);

	return targetable;
}

long TargetVisible(ITEM_INFO* item, AI_INFO* info)
{
	ITEM_INFO* enemy;
	CREATURE_INFO* creature;
	GAME_VECTOR s;
	GAME_VECTOR t;
	short* bounds;

	creature = (CREATURE_INFO*)item->data;
	enemy = creature->enemy;

	if (!enemy || enemy->hit_points <= 0 || !enemy->data || info->angle - creature->joint_rotation[2] <= -0x4000 ||
		info->angle - creature->joint_rotation[2] >= 0x4000 || info->distance >= 0x4000000)
		return 0;

	bounds = GetBestFrame(enemy);

	s.x = item->pos.x_pos;
	s.y = item->pos.y_pos - 768;
	s.z = item->pos.z_pos;
	s.room_number = item->room_number;

	t.x = enemy->pos.x_pos;
	t.y = enemy->pos.y_pos + ((3 * bounds[2] + bounds[3]) >> 2);
	t.z = enemy->pos.z_pos;
	return LOS(&s, &t);
}

long Targetable(ITEM_INFO* item, AI_INFO* info)
{
	CREATURE_INFO* creature = GetCreatureInfo(item);
	ITEM_INFO* enemy = creature->enemy;
	if (enemy == NULL || enemy->hit_points <= 0 || enemy->data == NULL || info->ahead == FALSE || info->distance >= 0x4000000)
		return FALSE;
	GAME_VECTOR s = {}, t = {};
	short* bounds = GetBestFrame(item);
	s.x = item->pos.x_pos;
	s.y = item->pos.y_pos + ((bounds[3] + 3 * bounds[2]) >> 2);
	s.z = item->pos.z_pos;
	s.room_number = item->room_number;
	bounds = GetBestFrame(enemy);
	t.x = enemy->pos.x_pos;
	t.y = enemy->pos.y_pos + ((bounds[3] + 3 * bounds[2]) >> 2);
	t.z = enemy->pos.z_pos;
	return LOS(&s, &t);
}
