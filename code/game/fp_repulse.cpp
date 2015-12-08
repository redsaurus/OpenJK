/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"

extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean WP_ForceThrowable( gentity_t *ent, gentity_t *forwardEnt, gentity_t *self, qboolean pull, float cone, float radius, vec3_t forward );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_ForceThrowHazardTrooper( gentity_t *self, gentity_t *trooper, qboolean pull );
extern void	WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty );
extern int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
extern qboolean ShouldPlayerResistForceThrow( gentity_t *player, gentity_t *attacker, qboolean pull );
extern qboolean Rosh_BeingHealed( gentity_t *self );
extern void G_KnockOffVehicle( gentity_t *pRider, gentity_t *self, qboolean bPull );
extern void WP_ForceKnockdown( gentity_t *self, gentity_t *pusher, qboolean pull, qboolean strongKnockdown, qboolean breakSaberLock );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void WP_KnockdownTurret( gentity_t *self, gentity_t *pas );
extern void WP_SaberDrop( gentity_t *self, gentity_t *saber );
extern void WP_SaberReturn( gentity_t *self, gentity_t *saber );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );

extern cvar_t	*g_timescale;

extern int forcePowerNeeded[NUM_FORCE_POWERS];

void RepulseDamage( gentity_t *self, gentity_t *enemy, vec3_t location, int damageLevel )
{
	switch (damageLevel) {
  case FORCE_LEVEL_1:
			G_Damage( enemy, self, self, NULL, location, 10, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN );
			break;
  case FORCE_LEVEL_2:
			G_Damage( enemy, self, self, NULL, location, 25, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN );
			break;
  case FORCE_LEVEL_3:
			G_Damage( enemy, self, self, NULL, location, 50, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN );
			break;
  default:
			break;
	}
}

void ForceRepulseThrow( gentity_t *self, int chargeTime )
{
	//shove things around you away
	qboolean fake = false;
	float		dist;
	gentity_t	*ent, *forwardEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities = 0;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	trace_t		tr;
	int			anim, hold, soundIndex, cost;
	int			damageLevel = FORCE_LEVEL_0;
	
	if ( self->health <= 0 )
	{
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-throw while leaning
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_REPULSE] > level.time )
	{//already pushing- now you can't haul someone across the room, sorry
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{
		if ( self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_3 )
		{//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}
	
	if ( self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoAnimTimer > 400)
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoAnimTimer > 900)
		|| (self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoAnimTimer > 500)
		|| (self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoAnimTimer > 300)
		|| (self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoAnimTimer > 500) )
	{//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}
	
	radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_REPULSE]];
	
	if ( !radius )
	{//no ability to do this yet
		return;
	}
	
	if ( chargeTime > 2500.0f )
	{
		damageLevel = FORCE_LEVEL_3;
	}
	else if ( chargeTime > 1250.0f )
	{
		damageLevel = FORCE_LEVEL_2;
	}
	else if ( chargeTime > 500.0f )
	{
		damageLevel = FORCE_LEVEL_1;
	}
	
	cost = forcePowerNeeded[FP_REPULSE];
	if ( !WP_ForcePowerUsable( self, FP_REPULSE, cost ) )
	{
		return;
	}
	//make sure this plays and that you cannot press fire for about 1 second after this
	anim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
	soundIndex = G_SoundIndex( "sound/weapons/force/repulse.wav" );
	hold = 650;
	
	int parts = SETANIM_TORSO;
	if ( !PM_InKnockDown( &self->client->ps ) )
	{
		if ( self->client->ps.saberLockTime > level.time )
		{
			self->client->ps.saberLockTime = 0;
			self->painDebounceTime = level.time + 2000;
			hold += 1000;
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		hold = floor( hold*g_timescale->value );
	}
	self->client->ps.weaponTime = hold;//was 1000, but want to swing sooner
	//do effect... FIXME: build-up or delay this until in proper part of anim
	self->client->ps.powerups[PW_FORCE_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	//reset to 0 in case it's still > 0 from a previous push
	self->client->pushEffectFadeTime = 0;
	
	G_Sound( self, soundIndex );
	
	VectorCopy( self->client->ps.viewangles, fwdangles );
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->currentOrigin, center );
	
	if ( !numListedEntities )
	{
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		
		numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
		
		for ( e = 0 ; e < numListedEntities ; e++ )
		{
			ent = entityList[ e ];
			
			if ( !WP_ForceThrowable( ent, forwardEnt, self, qfalse, 0.0f, radius, forward ) )
			{
				continue;
			}
			
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ )
			{
				if ( center[i] < ent->absmin[i] )
				{
					v[i] = ent->absmin[i] - center[i];
				} else if ( center[i] > ent->absmax[i] )
				{
					v[i] = center[i] - ent->absmax[i];
				} else
				{
					v[i] = 0;
				}
			}
			
			VectorSubtract( ent->absmax, ent->absmin, size );
			VectorMA( ent->absmin, 0.5, size, ent_org );
			
			//see if they're in front of me
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			
			dist = VectorLength( v );
			
			if ( dist >= radius )
			{
				continue;
			}
			
			//in PVS?
			if ( !ent->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.eyePoint ) )
			{//must be in PVS
				continue;
			}
			
			if ( ent != forwardEnt )
			{//don't need to trace against forwardEnt again
				//really should have a clear LOS to this thing...
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_FORCE_PUSH, (EG2_Collision)0, 0 );//was MASK_SHOT, but changed to match above trace and crosshair trace
				if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
				{//must have clear LOS
					continue;
				}
			}
			
			// ok, we are within the radius, add us to the incoming list
			push_list[ent_count] = ent;
			ent_count++;
		}
	}
	
	for ( int x = 0; x < ent_count; x++ )
	{
		if ( push_list[x]->client )
		{
			vec3_t	pushDir;
			float	knockback = 200;
			
			//SIGH band-aid...
			if ( push_list[x]->s.number >= MAX_CLIENTS
				&& self->s.number < MAX_CLIENTS )
			{
				if ( (push_list[x]->client->ps.forcePowersActive&(1<<FP_GRIP))
					//&& push_list[x]->client->ps.forcePowerDebounce[FP_GRIP] < level.time
					&& push_list[x]->client->ps.forceGripEntityNum == self->s.number )
				{
					WP_ForcePowerStop( push_list[x], FP_GRIP );
				}
				if ( (push_list[x]->client->ps.forcePowersActive&(1<<FP_DRAIN))
					//&& push_list[x]->client->ps.forcePowerDebounce[FP_DRAIN] < level.time
					&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number )
				{
					WP_ForcePowerStop( push_list[x], FP_DRAIN );
				}
			}
			
			if ( Rosh_BeingHealed( push_list[x] ) )
			{
				continue;
			}
			if ( push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER
				&& push_list[x]->health > 0 )
			{//living hazard troopers resist push/pull
				WP_ForceThrowHazardTrooper( self, push_list[x], qfalse );
				continue;
			}
			if ( fake )
			{//always resist
				WP_ResistForcePush( push_list[x], self, qfalse );
				continue;
			}

			int powerLevel, powerUse;
			powerLevel = self->client->ps.forcePowerLevel[FP_REPULSE];
			powerUse = FP_REPULSE;

			int modPowerLevel = WP_AbsorbConversion( push_list[x], push_list[x]->client->ps.forcePowerLevel[FP_ABSORB], self, powerUse, powerLevel, forcePowerNeeded[self->client->ps.forcePowerLevel[powerUse]] );
			if (push_list[x]->client->NPC_class==CLASS_ASSASSIN_DROID ||
				push_list[x]->client->NPC_class==CLASS_HAZARD_TROOPER)
			{
				modPowerLevel = 0;	// devides throw by 10
			}
			
			//First, if this is the player we're push/pulling, see if he can counter it
			if ( modPowerLevel != -1
				&& InFront( self->currentOrigin, push_list[x]->client->renderInfo.eyePoint, push_list[x]->client->ps.viewangles, 0.3f ) )
			{//absorbed and I'm in front of them
				//counter it
				if ( push_list[x]->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2 )
				{//no reaction at all
				}
				else
				{
					WP_ResistForcePush( push_list[x], self, qfalse );
					push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
					push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
				}
				continue;
			}
			else if ( !push_list[x]->s.number )
			{//player
				if ( ShouldPlayerResistForceThrow(push_list[x], self, qfalse) )
				{
					WP_ResistForcePush( push_list[x], self, qfalse );
					push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
					push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
					continue;
				}
			}
			else if ( push_list[x]->client && Jedi_WaitingAmbush( push_list[x] ) )
			{
				WP_ForceKnockdown( push_list[x], self, qfalse, qtrue, qfalse );
				RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
				continue;
			}
			
			G_KnockOffVehicle( push_list[x], self, qfalse );
			
			if ( push_list[x]->client->ps.forceDrainEntityNum == self->s.number
				&& (self->s.eFlags&EF_FORCE_DRAINED) )
			{//stop them from draining me now, dammit!
				WP_ForcePowerStop( push_list[x], FP_DRAIN );
			}
			
			//okay, everyone else (or player who couldn't resist it)...
			if ( ((self->s.number == 0 && Q_irand( 0, 2 ) ) || Q_irand( 0, 2 ) ) && push_list[x]->client && push_list[x]->health > 0 //a living client
				&& push_list[x]->client->ps.weapon == WP_SABER //Jedi
				&& push_list[x]->health > 0 //alive
				&& push_list[x]->client->ps.forceRageRecoveryTime < level.time //not recobering from rage
				&& ((self->client->NPC_class != CLASS_DESANN&&Q_stricmp("Yoda",self->NPC_type)) || !Q_irand( 0, 2 ) )//only 30% chance of resisting a Desann push
				&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE //on the ground
				&& InFront( self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles, 0.3f ) //I'm in front of him
				&& ( push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time ||//he's pushing too
					(push_list[x]->s.number != 0 && push_list[x]->client->ps.weaponTime < level.time)//not the player and not attacking (NPC jedi auto-defend against pushes)
					)
				)
			{//Jedi don't get pushed, they resist as long as they aren't already attacking and are on the ground
				if ( push_list[x]->client->ps.saberLockTime > level.time )
				{//they're in a lock
					if ( push_list[x]->client->ps.saberLockEnemy != self->s.number )
					{//they're not in a lock with me
						continue;
					}
					else if ( self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_3 ||
							 push_list[x]->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
					{//they're in a lock with me, but my push is too weak
						continue;
					}
					else
					{//we will knock them down
						self->painDebounceTime = 0;
						self->client->ps.weaponTime = 500;
						if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
						{
							self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
						}
					}
				}
				int resistChance = Q_irand(0, 2);
				if ( push_list[x]->s.number >= MAX_CLIENTS )
				{//NPC
					if ( g_spskill->integer == 1 )
					{//stupid tweak for graham
						resistChance = Q_irand(0, 3);
					}
				}
				if ( modPowerLevel == -1
						&& self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2
						&& !resistChance
						&& push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
				{//a level 3 push can even knock down a jedi
					if ( PM_InKnockDown( &push_list[x]->client->ps ) )
					{//can't knock them down again
						continue;
					}
					WP_ForceKnockdown( push_list[x], self, qfalse, qfalse, qtrue );
					RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
				}
				else
				{
					WP_ResistForcePush( push_list[x], self, qfalse );
				}
			}
			else
			{
				//UGH: FIXME: for enemy jedi, they should probably always do force pull 3, and not your weapon (if player?)!
				//shove them
				if ( push_list[x]->NPC
					&& push_list[x]->NPC->jumpState == JS_JUMPING )
				{//don't interrupt a scripted jump
					//WP_ResistForcePush( push_list[x], self, qfalse );
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
					continue;
				}
				
				if ( push_list[x]->s.number
					&& (push_list[x]->message || (push_list[x]->flags&FL_NO_KNOCKBACK)) )
				{//an NPC who has a key
					//don't push me... FIXME: maybe can pull the key off me?
					WP_ForceKnockdown( push_list[x], self, qfalse, qfalse, qfalse );
					RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
					continue;
				}
				{
					VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
					knockback -= VectorNormalize( pushDir );
					if ( knockback < 100 )
					{
						knockback = 100;
					}
					//scale for push level
					if ( self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_2 )
					{//maybe just knock them down
						knockback /= 3;
					}
					else if ( self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2 )
					{//super-hard push
						//Hmm, maybe in this case can even nudge/knockdown a jedi?  Especially if close?
						//knockback *= 5;
					}
				}
				
				if ( modPowerLevel != -1 )
				{
					if ( !modPowerLevel )
					{
						knockback /= 10.0f;
					}
					else if ( modPowerLevel == 1 )
					{
						knockback /= 6.0f;
					}
					else// if ( modPowerLevel == 2 )
					{
						knockback /= 2.0f;
					}
				}
				//actually push/pull the enemy
				G_Throw( push_list[x], pushDir, knockback );
				//make it so they don't actually hurt me when pulled at me...
				push_list[x]->forcePuller = self->s.number;
				
				if ( push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//if on the ground, make sure they get shoved up some
					if ( push_list[x]->client->ps.velocity[2] < knockback )
					{
						push_list[x]->client->ps.velocity[2] = knockback;
					}
				}
				
				if ( push_list[x]->health > 0 )
				{//target is still alive
					if ( (push_list[x]->s.number||(cg.renderingThirdPerson&&!cg.zoomMode)) //NPC or 3rd person player
						&& ((self->client->ps.forcePowerLevel[FP_REPULSE] < FORCE_LEVEL_2 && push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1)) )
					{//NPC or third person player (without force push/pull skill), and force push/pull level is at 1
						WP_ForceKnockdown( push_list[x], self, qfalse, (knockback>150), qfalse );
						RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
					}
					else if ( !push_list[x]->s.number )
					{//player, have to force an anim on him
						WP_ForceKnockdown( push_list[x], self, qfalse, (knockback>150), qfalse );
						RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
					}
					else
					{//NPC and force-push/pull at level 2 or higher
						WP_ForceKnockdown( push_list[x], self, qfalse, (knockback>100), qfalse );
						RepulseDamage( self, push_list[x], tr.endpos, damageLevel );
					}
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			}
		}
		else if ( !fake )
		{//not a fake push/pull
			if ( push_list[x]->s.weapon == WP_SABER && (push_list[x]->contents&CONTENTS_LIGHTSABER) )
			{//a thrown saber, just send it back
				/*
				 if ( pull )
				 {//steal it?
				 }
				 else */if ( push_list[x]->owner && push_list[x]->owner->client && push_list[x]->owner->client->ps.SaberActive() && push_list[x]->s.pos.trType == TR_LINEAR && push_list[x]->owner->client->ps.saberEntityState != SES_RETURNING )
				 {//it's on and being controlled
					 //FIXME: prevent it from damaging me?
					 if ( self->s.number == 0 || Q_irand( 0, 2 ) )
					 {//certain chance of throwing it aside and turning it off?
						 //give it some velocity away from me
						 //FIXME: maybe actually push or pull it?
						 if ( Q_irand( 0, 1 ) )
						 {
							 VectorScale( right, -1, right );
						 }
						 G_ReflectMissile( self, push_list[x], right );
						 //FIXME: isn't turning off!!!
						 WP_SaberDrop( push_list[x]->owner, push_list[x] );
					 }
					 else
					 {
						 WP_SaberReturn( push_list[x]->owner, push_list[x] );
					 }
					 //different effect?
				 }
			}
			else if ( push_list[x]->s.eType == ET_MISSILE
					 && push_list[x]->s.pos.trType != TR_STATIONARY
					 && (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				vec3_t dir2Me;
				VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, dir2Me );
				float dot = DotProduct( push_list[x]->s.pos.trDelta, dir2Me );
				
				if ( push_list[x]->s.eFlags&EF_MISSILE_STICK )
				{//caught a sticky in-air
					push_list[x]->s.eType = ET_MISSILE;
					push_list[x]->s.eFlags &= ~EF_MISSILE_STICK;
					push_list[x]->s.eFlags |= EF_BOUNCE_HALF;
					push_list[x]->splashDamage /= 3;
					push_list[x]->splashRadius /= 3;
					push_list[x]->e_ThinkFunc = thinkF_WP_Explode;
					push_list[x]->nextthink = level.time + Q_irand( 500, 3000 );
				}
				if ( dot >= 0 )
				{//it's heading towards me
					G_ReflectMissile( self, push_list[x], forward );
				}
				else
				{
					VectorScale( push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta );
				}
				//deflect sound
				//G_Sound( push_list[x], G_SoundIndex( va("sound/weapons/blaster/reflect%d.wav", Q_irand( 1, 3 ) ) ) );
				//push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				
				if ( push_list[x]->s.eType == ET_MISSILE
					&& push_list[x]->s.weapon == WP_ROCKET_LAUNCHER
					&& push_list[x]->damage < 60 )
				{//pushing away a rocket raises it's damage to the max for NPCs
					push_list[x]->damage = 60;
				}
			}
			else if ( push_list[x]->svFlags & SVF_GLASS_BRUSH )
			{//break the glass
				trace_t tr;
				vec3_t	pushDir;
				float	damage = 800;
				
				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}
				
				
				VectorSubtract( tr.endpos, self->client->renderInfo.eyePoint, pushDir );
				
				damage -= VectorNormalize( pushDir );
				if ( damage < 200 )
				{
					damage = 200;
				}
				VectorScale( pushDir, damage, pushDir );
				
				G_Damage( push_list[x], self, self, pushDir, tr.endpos, damage, 0, MOD_UNKNOWN );
			}
			else if ( push_list[x]->s.eType == ET_MISSILE/*thermal resting on ground*/
					 || push_list[x]->s.eType == ET_ITEM
					 || push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp( "limb", push_list[x]->classname ) == 0 )
			{//general object, toss it
				vec3_t	pushDir, kvel;
				float	knockback = 200;
				float	mass = 200;
				
				if ( self->enemy //I have an enemy
					&& push_list[x]->s.eType != ET_ITEM //not an item
					&& self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2 //have push 3 or greater
					&& InFront(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)//object is generally in front of me
					&& InFront(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)//enemy is pretty much right in front of me
					&& InFront(push_list[x]->currentOrigin, self->enemy->currentOrigin, self->enemy->currentAngles, 0.25f)//object is generally in front of enemy
					//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
					&& ( (self->NPC&&(Q_irand(0,RANK_CAPTAIN)<self->NPC->rank) )//NPC with enough skill
						||( self->s.number<MAX_CLIENTS ) )
					)
				{//if I have an auto-enemy & he's in front of me, push it toward him!
					/*
					 if ( targetedObjectMassTotal + push_list[x]->mass > TARGETED_OBJECT_PUSH_MASS_MAX )
					 {//already pushed too many things
						//FIXME: pick closest?
						continue;
					 }
					 targetedObjectMassTotal += push_list[x]->mass;
					 */
					VectorSubtract( self->enemy->currentOrigin, push_list[x]->currentOrigin, pushDir );
				}
				else
				{
					VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
				}
				knockback -= VectorNormalize( pushDir );
				if ( knockback < 100 )
				{
					knockback = 100;
				}
				
				//FIXME: if pull a FL_FORCE_PULLABLE_ONLY, clear the flag, assuming it's no longer in solid?  or check?
				VectorCopy( push_list[x]->currentOrigin, push_list[x]->s.pos.trBase );
				push_list[x]->s.pos.trTime = level.time;								// move a bit on the very first frame
				if ( push_list[x]->s.pos.trType != TR_INTERPOLATE )
				{//don't do this to rolling missiles
					push_list[x]->s.pos.trType = TR_GRAVITY;
				}
				
				if ( push_list[x]->e_ThinkFunc == thinkF_G_RunObject && push_list[x]->physicsBounce )
				{//it's a pushable misc_model_breakable, use it's mass instead of our one-size-fits-all mass
					mass = push_list[x]->physicsBounce;//same as push_list[x]->mass, right?
				}
				if ( mass < 50 )
				{//???
					mass = 50;
				}
				if ( g_gravity->value > 0 )
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass * 0.8, kvel );
					kvel[2] = pushDir[2] * g_knockback->value * knockback / mass * 1.5;
				}
				else
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass, kvel );
				}
				VectorAdd( push_list[x]->s.pos.trDelta, kvel, push_list[x]->s.pos.trDelta );
				if ( g_gravity->value > 0 )
				{
					if ( push_list[x]->s.pos.trDelta[2] < knockback )
					{
						push_list[x]->s.pos.trDelta[2] = knockback;
					}
				}
				//no trDuration?
				if ( push_list[x]->e_ThinkFunc != thinkF_G_RunObject )
				{//objects spin themselves?
					//spin it
					//FIXME: messing with roll ruins the rotational center???
					push_list[x]->s.apos.trTime = level.time;
					push_list[x]->s.apos.trType = TR_LINEAR;
					VectorClear( push_list[x]->s.apos.trDelta );
					push_list[x]->s.apos.trDelta[1] = Q_irand( -800, 800 );
				}
				
				if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
				{//make sure it runs it's physics
					push_list[x]->e_ThinkFunc = thinkF_LimbThink;
					push_list[x]->nextthink = level.time + FRAMETIME;
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				push_list[x]->forcePuller = self->s.number;//remember this regardless
				if ( push_list[x]->item && push_list[x]->item->giTag == INV_SECURITY_KEY )
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_DISCOVERED );//security keys are more important
				}
				else
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_SUSPICIOUS );//hmm... or should this always be discovered?
				}
			}
			else if ( push_list[x]->s.weapon == WP_TURRET
					 && !Q_stricmp( "PAS", push_list[x]->classname )
					 && push_list[x]->s.apos.trType == TR_STATIONARY )
			{//a portable turret
				WP_KnockdownTurret( self, push_list[x] );
			}
		}
	}

	WP_ForcePowerDrain(self, FP_REPULSE, cost);

	if ( self->NPC )
	{//NPCs can push more often
		//FIXME: vary by rank and game skill?
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + 200;
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	}
}
