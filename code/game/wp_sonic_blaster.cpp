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
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

#define SONIC_BLAST_RADIUS 512
#define SONIC_BLAST_CONE 0.8f
extern qboolean WP_ForceThrowable( gentity_t *ent, gentity_t *forwardEnt, gentity_t *self, qboolean pull, float cone, float radius, vec3_t forward );
extern qboolean Rosh_BeingHealed( gentity_t *self );
extern void G_KnockOffVehicle( gentity_t *pRider, gentity_t *self, qboolean bPull );
extern void WP_ForceKnockdown( gentity_t *self, gentity_t *pusher, qboolean pull, qboolean strongKnockdown, qboolean breakSaberLock );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void WP_KnockdownTurret( gentity_t *self, gentity_t *pas );
extern void WP_SaberDrop( gentity_t *self, gentity_t *saber );
extern void WP_SaberReturn( gentity_t *self, gentity_t *saber );
void WP_SonicBlast( gentity_t *self )
{//FIXME: pass in a target ent so we (an NPC) can push/pull just one targeted ent.
	//shove things in front of you away
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
	float		dot1, cone;
	trace_t		tr;
	
	
	radius = SONIC_BLAST_RADIUS;
	
	self->client->ps.powerups[PW_REFRACT_MUZZLE] = level.time + 1000;
	self->client->pushEffectFadeTime = 0;
	
	VectorCopy( self->client->ps.viewangles, fwdangles );
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->currentOrigin, center );
	
	cone = SONIC_BLAST_CONE;
	
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
			
			if ( !WP_ForceThrowable( ent, forwardEnt, self, qfalse, cone, radius, forward ) )
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
			if ( cone < 1.0f )
			{//must be within the forward cone
				if ( ent->s.eType == ET_MISSILE )//&& ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )
				{//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
					if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
						continue;
				}
				else if ( (dot1 = DotProduct( dir, forward )) < cone )
				{
					continue;
				}
			}
			else if ( ent->s.eType == ET_MISSILE )
			{//a missile and we're at force level 1... just use a small cone, but not ridiculously small
				if ( (dot1 = DotProduct( dir, forward )) < 0.75f )
				{
					continue;
				}
			}//else is an NPC or brush entity that our forward trace would have to hit
			
			dist = VectorLength( v );
			
			//Now check and see if we can actually deflect it
			//method1
			//if within a certain range, deflect it
			if ( ent->s.eType == ET_MISSILE && cone >= 1.0f )
			{//smaller radius on missile checks at force push 1
				if ( dist >= 192 )
				{
					continue;
				}
			}
			else if ( dist >= radius )
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
	
	if ( ent_count )
	{
		for ( int x = 0; x < ent_count; x++ )
		{
			if ( push_list[x]->client )
			{
				vec3_t	pushDir;
				float	knockback = 200;
				
				if ( Rosh_BeingHealed( push_list[x] ) )
				{
					continue;
				}
				
				if ( push_list[x]->client && Jedi_WaitingAmbush( push_list[x] ) )
				{
					WP_ForceKnockdown( push_list[x], self, qfalse, qtrue, qfalse );
					continue;
				}
				
				G_KnockOffVehicle( push_list[x], self, qfalse );
				
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
						continue;
					}
					{
						VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
						knockback -= VectorNormalize( pushDir );
						if ( knockback < 100 )
						{
							knockback = 100;
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
						if ( !push_list[x]->s.number )
						{//player, have to force an anim on him
							WP_ForceKnockdown( push_list[x], self, qfalse, knockback>150, qfalse );
						}
						else
						{//NPC and force-push/pull at level 2 or higher
							WP_ForceKnockdown( push_list[x], self, qfalse, knockback>100, qfalse );
						}
					}
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				}
			}
			else
			{
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
					{
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
					}
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
					
					{
						VectorSubtract( tr.endpos, self->client->renderInfo.eyePoint, pushDir );
					}
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
					
					{
						if ( self->enemy //I have an enemy
							&& push_list[x]->s.eType != ET_ITEM //not an item
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 //have push 3 or greater
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
	}
}
