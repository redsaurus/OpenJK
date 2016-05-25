#include "g_local.h"
#include "cgame/cg_local.h"
#include "cgame/cg_media.h"

#define DRAWBOX_REFRESH	500	// How long it takes to redraw the boxes
#define BOX_VERTICES 8

qboolean inAIWorkshop = qfalse;
qboolean showDisplay = qtrue;

int drawBoxesTime = 0;
int selectedAI = ENTITYNUM_NONE;

extern stringID_table_t BSTable[];
extern stringID_table_t BSETTable[];
extern stringID_table_t teamTable[];
extern stringID_table_t NPCClassTable[];
extern stringID_table_t RankTable[];
extern stringID_table_t MoveTypeTable[];
extern stringID_table_t FPTable[];

#define OL_S	0.5f
#define OL_Y	30
#define OL_H	6
static void WorkshopDrawEntityInformation(gentity_t* ent, int x, const char* title) {
	int add = OL_H;
	vec4_t textcolor = { 0.4f, 0.4f, 0.8f, 1.0f };

	cgi_R_Font_DrawString(x, OL_Y, title, textcolor, cgs.media.qhFontSmall, -1, OL_S);

	cgi_R_Font_DrawString(x, OL_Y + add, va("NPC_type: %s", ent->NPC_type), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	cgi_R_Font_DrawString(x, OL_Y + add, va("health: %i/%i", ent->health, ent->max_health), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	if (ent->script_targetname) {
		cgi_R_Font_DrawString(x, OL_Y + add, va("script_targetname: %s", ent->script_targetname), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	if (ent->NPC->goalEntity) {
		cgi_R_Font_DrawString(x, OL_Y + add, va("goalEnt = %i", ent->NPC->goalEntity->s.number), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}
	cgi_R_Font_DrawString(x, OL_Y + add, va("bs = %s", BSTable[ent->NPC->behaviorState].name), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;
	if (ent->NPC->combatMove) {
		cgi_R_Font_DrawString(x, OL_Y + add, "-- in combat move --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}
	
	cgi_R_Font_DrawString(x, OL_Y + add, va("class = %s", NPCClassTable[ent->client->NPC_class].name), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;
	cgi_R_Font_DrawString(x, OL_Y + add, va("rank = %s (%i)", RankTable[ent->NPC->rank].name, ent->NPC->rank), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	if (ent->NPC->scriptFlags) {
		cgi_R_Font_DrawString(x, OL_Y + add, va("scriptFlags: %i", ent->NPC->scriptFlags), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}
	if (ent->NPC->aiFlags) {
		cgi_R_Font_DrawString(x, OL_Y + add, va("aiFlags: %i", ent->NPC->aiFlags), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	if (ent->client->noclip) {
		cgi_R_Font_DrawString(x, OL_Y + add, "cheat: NOCLIP", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	if (ent->flags & FL_GODMODE) {
		cgi_R_Font_DrawString(x, OL_Y + add, "cheat: GODMODE", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	if (ent->flags & FL_NOTARGET) {
		cgi_R_Font_DrawString(x, OL_Y + add, "cheat: NOTARGET", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	if (ent->flags & FL_UNDYING) {
		cgi_R_Font_DrawString(x, OL_Y + add, "cheat: UNDEAD", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
	}

	cgi_R_Font_DrawString(x, OL_Y + add, va("playerTeam: %s", teamTable[ent->client->playerTeam].name), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	cgi_R_Font_DrawString(x, OL_Y + add, va("enemyTeam: %s", teamTable[ent->client->enemyTeam].name), textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	cgi_R_Font_DrawString(x, OL_Y + add, "-- assigned scripts --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;

	for (int i = BSET_SPAWN; i < NUM_BSETS; i++) {
		if (ent->behaviorSet[i]) {
			cgi_R_Font_DrawString(x, OL_Y + add, va("%s: %s", BSETTable[i].name, ent->behaviorSet[i]), textcolor, cgs.media.qhFontSmall, -1, OL_S);
			add += OL_H;
		}
	}

	if (ent->parms) {
		cgi_R_Font_DrawString(x, OL_Y + add, "-- parms --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		for (int i = 0; i < MAX_PARMS; i++) {
			if (ent->parms->parm[i][0]) {
				cgi_R_Font_DrawString(x, OL_Y + add, va("parm%i : %s", i + 1, ent->parms->parm[i]), textcolor, cgs.media.qhFontSmall, -1, OL_S);
				add += OL_H;
			}
		}
	}

	if (ent->NPC->group && ent->NPC->group->numGroup > 1) {
		cgi_R_Font_DrawString(x, OL_Y + add, "-- squad data --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		cgi_R_Font_DrawString(x, OL_Y + add, va("morale: %i", ent->NPC->group->morale), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		cgi_R_Font_DrawString(x, OL_Y + add, va("morale debounce: %i", ent->NPC->group->moraleDebounce), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		cgi_R_Font_DrawString(x, OL_Y + add, va("last seen enemy: %i milliseconds", level.time - ent->NPC->group->lastSeenEnemyTime), textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		if (ent->NPC->group->commander) {
			cgi_R_Font_DrawString(x, OL_Y + add, va("commander: %i", ent->NPC->group->commander->s.number), textcolor, cgs.media.qhFontSmall, -1, OL_S);
			add += OL_H;
		}
		
		cgi_R_Font_DrawString(x, OL_Y + add, "-- squad members --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
		add += OL_H;
		for (int i = 0; i < ent->NPC->group->numGroup; i++) {
			AIGroupMember_t* memberAI = &ent->NPC->group->member[i];
			int memberNum = memberAI->number;
			gentity_t* member = &g_entities[memberNum];
			char* memberText = va("* entity %i, closestBuddy: %i, class: %s, rank: %s (%i), health: %i/%i",
				memberNum, memberAI->closestBuddy, NPCClassTable[member->client->NPC_class].name,
				RankTable[member->NPC->rank].name, member->NPC->rank, member->health, member->max_health);
			cgi_R_Font_DrawString(x, OL_Y + add, memberText, textcolor, cgs.media.qhFontSmall, -1, OL_S);
			add += OL_H;
		}
	}

	cgi_R_Font_DrawString(x, OL_Y + add, "-- currently active timers --", textcolor, cgs.media.qhFontSmall, -1, OL_S);
	add += OL_H;
	auto timers = TIMER_List(ent);
	for (auto it = timers.begin(); it != timers.end(); ++it) {
		if (it->second >= 0) {
			cgi_R_Font_DrawString(x, OL_Y + add, va("%s : %i", it->first.c_str(), it->second), textcolor, cgs.media.qhFontSmall, -1, OL_S);
			add += OL_H;
		}
	}
}

//
// Draw textual information about the NPC in our crosshairs and also the currently selected one
//
void WorkshopDrawClientsideInformation() {
	if (inAIWorkshop == qfalse) {
		return;
	}
	if (showDisplay == qfalse) {
		return;
	}
	// Draw the information for the NPC that is in our crosshairs
	if (cg.crosshairClientNum != ENTITYNUM_NONE && cg.crosshairClientNum != 0 && g_entities[cg.crosshairClientNum].client) {
		gentity_t* crossEnt = &g_entities[cg.crosshairClientNum];
		WorkshopDrawEntityInformation(crossEnt, 10, "Crosshair AI");
	}

	// Draw the information for the AI that we have selected
	if (selectedAI != ENTITYNUM_NONE) {
		gentity_t* selectedEnt = &g_entities[selectedAI];
		WorkshopDrawEntityInformation(selectedEnt, 500, "Selected AI");
	}
}

//
//	Draws a box around the bounding box of the entity
//
void WorkshopDrawEntBox(gentity_t* ent, int colorOverride = -1) {
	unsigned color = 0x666666; // G_SoundOnEnt(ent, "satan.mp3");
	if (colorOverride == -1 && selectedAI == ent->s.number) {
		// Draw a box around our goal ent
		if (ent->NPC->goalEntity != NULL) {
			WorkshopDrawEntBox(ent->NPC->goalEntity, color);
		}
		color = 0x0000FFFF; // Yellow = selected AI
	}
	else if (colorOverride == -1 && ent->client->playerTeam == TEAM_ENEMY) {
		color = 0x000000FF; // Red = enemy
	}
	else if (colorOverride == -1 && ent->client->playerTeam == TEAM_PLAYER) {
		color = 0x0000FF00; // Green = ally
	}
	else if (colorOverride == -1 && ent->client->playerTeam == TEAM_NEUTRAL) {
		color = 0x00FF0000; // Blue = neutral
	}
	else if (colorOverride != -1) {
		color = colorOverride;
	}

	// This is going to look like a mess and I can't really comment on this, but I drew this out manually and it works.
	vec3_t vertices[BOX_VERTICES] = { 0 };
	for (int i = 0; i < BOX_VERTICES; i++) {
		VectorCopy(ent->currentOrigin, vertices[i]);
	}

	for (int i = 4; i < BOX_VERTICES; i++) {
		vertices[i][2] += ent->mins[2];
	}
	for (int i = 0; i < 4; i++) {
		vertices[i][2] += ent->maxs[2];
	}
	vertices[0][0] += ent->mins[0];
	vertices[1][0] += ent->mins[0];
	vertices[4][0] += ent->mins[0];
	vertices[5][0] += ent->mins[0];
	vertices[2][0] += ent->maxs[0];
	vertices[3][0] += ent->maxs[0];
	vertices[6][0] += ent->maxs[0];
	vertices[7][0] += ent->maxs[0];
	vertices[0][1] += ent->mins[1];
	vertices[2][1] += ent->mins[1];
	vertices[4][1] += ent->mins[1];
	vertices[6][1] += ent->mins[1];
	vertices[1][1] += ent->maxs[1];
	vertices[3][1] += ent->maxs[1];
	vertices[5][1] += ent->maxs[1];
	vertices[7][1] += ent->maxs[1];

	G_DebugLine(vertices[0], vertices[1], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[2], vertices[3], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[0], vertices[2], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[1], vertices[3], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[0], vertices[4], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[1], vertices[5], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[2], vertices[6], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[3], vertices[7], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[4], vertices[5], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[6], vertices[7], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[4], vertices[6], DRAWBOX_REFRESH, color, qtrue);
	G_DebugLine(vertices[5], vertices[7], DRAWBOX_REFRESH, color, qtrue);
}

//
//	Occurs every frame
//
void WorkshopThink() {
	if (!inAIWorkshop) {
		return;
	}

	// If the selected AI is dead, deselect it
	if (selectedAI != ENTITYNUM_NONE) {
		gentity_t* ent = &g_entities[selectedAI];
		if (ent->health <= 0) {
			selectedAI = ENTITYNUM_NONE;
		}
	}
	if (drawBoxesTime < level.time && showDisplay) {
		for (int i = 1; i < globals.num_entities; i++) {
			if (!PInUse(i)) {
				continue;
			}

			gentity_t* found = &g_entities[i];
			if (!found->client) {
				continue;
			}

			WorkshopDrawEntBox(found);
			drawBoxesTime = level.time + DRAWBOX_REFRESH - 50;	// -50 so we get an extra frame to draw, so the boxes don't blink
		}
	}
}

//
//	Workshop mode just got toggled by console command
//
void WorkshopToggle(gentity_t* ent) {
	inAIWorkshop = !inAIWorkshop;

	if (inAIWorkshop) {
		// Workshop mode just got enabled
		gi.Printf("AI Workshop enabled\n");
	}
	else {
		// Workshop mode just got disabled
		gi.Printf("AI Workshop disabled\n");
	}
}

//////////////////////
//
// Below are various commands that we can do
//
//////////////////////

void WorkshopSelect_f(gentity_t* ent) {
	vec3_t		src, dest, vf;
	trace_t		trace;
	VectorCopy(ent->client->renderInfo.eyePoint, src);
	AngleVectors(ent->client->ps.viewangles, vf, NULL, NULL);//ent->client->renderInfo.eyeAngles was cg.refdef.viewangles, basically
	//extend to find end of use trace
	VectorMA(src, 32967.0f, vf, dest);

	//Trace ahead to find a valid target
	gi.trace(&trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE, G2_NOCOLLIDE, 0);
	if (trace.fraction == 1.0f || trace.entityNum < 1)
	{
		return;
	}

	gentity_t* target = &g_entities[trace.entityNum];
	if (target->client) {
		selectedAI = target->s.number;
	}
	else {
		gi.Printf("No NPC in crosshair\n");
	}
}

// Deselect currently selected NPC
void WorkshopDeselect_f(gentity_t* ent) {
	selectedAI = ENTITYNUM_NONE;
}

// View all behavior states
void Workshop_List_BehaviorState_f(gentity_t* ent) {
	for (int i = 0; i < NUM_BSTATES; i++) {
		gi.Printf(va("%s\n", BSTable[i].name));
	}
}

// View all teams
void Workshop_List_Team_f(gentity_t* ent) {
	for (int i = 0; i < TEAM_NUM_TEAMS; i++) {
		gi.Printf(va("%s\n", teamTable[i].name));
	}
}

// View all ranks
void Workshop_List_Ranks_f(gentity_t* ent) {
	for (int i = 0; i < RANK_MAX; i++) {
		gi.Printf(va("%s\n", RankTable[i].name));
	}
}

// View all classes
void Workshop_List_Classes_f(gentity_t* ent) {
	for (int i = 0; i < CLASS_NUM_CLASSES; i++) {
		gi.Printf(va("%s\n", NPCClassTable[i].name));
	}
}

// View all movetypes
void Workshop_List_Movetypes_f(gentity_t* ent) {
	for (int i = 0; i <= MT_FLYSWIM; i++) {
		gi.Printf(va("%s\n", MoveTypeTable[i].name));
	}
}

// View all forcepowers
void Workshop_List_ForcePowers_f(gentity_t* ent) {
	for (int i = 0; i <= FP_SABER_OFFENSE; i++) {
		gi.Printf(va("%s\n", FPTable[i].name));
	}
}

// View all behaviorsets
void Workshop_List_BehaviorSets_f(gentity_t* ent) {
	for (int i = BSET_SPAWN; i < NUM_BSETS; i++) {
		gi.Printf(va("%s\n", BSETTable[i].name));
	}
}

// List all scriptflags
#define PRINTFLAG(x) gi.Printf("" #x " = %i\n", x)
void Workshop_List_Scriptflags_f(gentity_t* ent) {
	PRINTFLAG(SCF_CROUCHED);
	PRINTFLAG(SCF_WALKING);
	PRINTFLAG(SCF_MORELIGHT);
	PRINTFLAG(SCF_LEAN_RIGHT);
	PRINTFLAG(SCF_LEAN_LEFT);
	PRINTFLAG(SCF_RUNNING);
	PRINTFLAG(SCF_ALT_FIRE);
	PRINTFLAG(SCF_NO_RESPONSE);
	PRINTFLAG(SCF_FFDEATH);
	PRINTFLAG(SCF_NO_COMBAT_TALK);
	PRINTFLAG(SCF_CHASE_ENEMIES);
	PRINTFLAG(SCF_LOOK_FOR_ENEMIES);
	PRINTFLAG(SCF_FACE_MOVE_DIR);
	PRINTFLAG(SCF_IGNORE_ALERTS);
	PRINTFLAG(SCF_DONT_FIRE);
	PRINTFLAG(SCF_DONT_FLEE);
	PRINTFLAG(SCF_FORCED_MARCH);
	PRINTFLAG(SCF_NO_GROUPS);
	PRINTFLAG(SCF_FIRE_WEAPON);
	PRINTFLAG(SCF_NO_MIND_TRICK);
	PRINTFLAG(SCF_USE_CP_NEAREST);
	PRINTFLAG(SCF_NO_FORCE);
	PRINTFLAG(SCF_NO_FALLTODEATH);
	PRINTFLAG(SCF_NO_ACROBATICS);
	PRINTFLAG(SCF_USE_SUBTITLES);
	PRINTFLAG(SCF_NO_ALERT_TALK);
}

// List all aiflags
void Workshop_List_AIFlags_f(gentity_t* ent) {
	PRINTFLAG(NPCAI_CHECK_WEAPON);
	PRINTFLAG(NPCAI_BURST_WEAPON);
	PRINTFLAG(NPCAI_MOVING);
	PRINTFLAG(NPCAI_TOUCHED_GOAL);
	PRINTFLAG(NPCAI_PUSHED);
	PRINTFLAG(NPCAI_NO_COLL_AVOID);
	PRINTFLAG(NPCAI_BLOCKED);
	PRINTFLAG(NPCAI_OFF_PATH);
	PRINTFLAG(NPCAI_IN_SQUADPOINT);
	PRINTFLAG(NPCAI_STRAIGHT_TO_DESTPOS);
	PRINTFLAG(NPCAI_NO_SLOWDOWN);
	PRINTFLAG(NPCAI_LOST);
	PRINTFLAG(NPCAI_SHIELDS);
	PRINTFLAG(NPCAI_GREET_ALLIES);
	PRINTFLAG(NPCAI_FORM_TELE_NAV);
	PRINTFLAG(NPCAI_ENROUTE_TO_HOMEWP);
	PRINTFLAG(NPCAI_MATCHPLAYERWEAPON);
	PRINTFLAG(NPCAI_DIE_ON_IMPACT);
}

// Set a raw timer
void Workshop_Set_Timer_f(gentity_t* ent) {
	if (gi.argc() != 3) {
		gi.Printf("usage: workshop_set_timer <timer name> <milliseconds>\n");
		return;
	}
	const char* timerName = gi.argv(1);
	int ms = atoi(gi.argv(2));
	TIMER_Set(&g_entities[selectedAI], timerName, ms);
}

// View all timers, including ones which are dead
void Workshop_View_Timers_f(gentity_t* ent) {
	auto timers = TIMER_List(&g_entities[selectedAI]);
	for (auto it = timers.begin(); it != timers.end(); ++it) {
		gi.Printf("%s\t\t:\t\t%i\n", it->first.c_str(), it->second);
	}
}

// Set Behavior State
void Workshop_Set_BehaviorState_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_bstate <bstate>\n");
		return;
	}
	int bState = GetIDForString(BSTable, gi.argv(1));
	if (bState == -1) {
		gi.Printf("Invalid bstate\n");
		return;
	}
	g_entities[selectedAI].NPC->behaviorState = (bState_t)bState;
}

// Set goal entity
void Workshop_Set_GoalEntity_f(gentity_t* ent) {
	if (gi.argc() == 2) {
		// There's a second argument. Use that.
		if (!Q_stricmp(gi.argv(1), "me")) {
			g_entities[selectedAI].NPC->goalEntity = g_entities;
			return;
		}
		else {
			g_entities[selectedAI].NPC->goalEntity = &g_entities[atoi(gi.argv(1))];
		}
		if (g_entities[selectedAI].NPC->lastGoalEntity != NULL) {
			gi.Printf("New goal entity: %i (last goal entity was %i)\n", g_entities[selectedAI].NPC->goalEntity->s.number, g_entities[selectedAI].NPC->lastGoalEntity->s.number);
		}
		else {
			gi.Printf("New goal entity: %i\n", g_entities[selectedAI].NPC->goalEntity->s.number);
		}
	}

	vec3_t		src, dest, vf;
	trace_t		trace;
	VectorCopy(ent->client->renderInfo.eyePoint, src);
	AngleVectors(ent->client->ps.viewangles, vf, NULL, NULL);//ent->client->renderInfo.eyeAngles was cg.refdef.viewangles, basically
	//extend to find end of use trace
	VectorMA(src, 32967.0f, vf, dest);

	//Trace ahead to find a valid target
	gi.trace(&trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE, G2_NOCOLLIDE, 0);
	if (trace.fraction == 1.0f || trace.entityNum < 1 || trace.entityNum == ENTITYNUM_NONE || trace.entityNum == ENTITYNUM_WORLD)
	{
		gi.Printf("Invalid entity\n");
		return;
	}

	gentity_t* target = &g_entities[trace.entityNum];
	gentity_t* selected = &g_entities[selectedAI];
	selected->NPC->lastGoalEntity = selected->NPC->goalEntity;
	selected->NPC->goalEntity = target;
	if (selected->NPC->lastGoalEntity != NULL) {
		gi.Printf("New goal entity: %i (last goal entity was %i)\n", selected->NPC->goalEntity->s.number, selected->NPC->lastGoalEntity->s.number);
	}
	else {
		gi.Printf("New goal entity: %i\n", selected->NPC->goalEntity->s.number);
	}
}

// Set enemy
void Workshop_Set_Enemy_f(gentity_t* ent) {
	if (gi.argc() == 2) {
		// There's a second argument. Use that.
		if (!Q_stricmp(gi.argv(1), "me")) {
			g_entities[selectedAI].lastEnemy = g_entities[selectedAI].enemy;
			g_entities[selectedAI].enemy = g_entities;
		}
		else {
			g_entities[selectedAI].enemy = &g_entities[atoi(gi.argv(1))];
		}
		if (g_entities[selectedAI].lastEnemy != NULL) {
			gi.Printf("New enemy: %i (last enemy was %i)\n", g_entities[selectedAI].enemy->s.number, g_entities[selectedAI].lastEnemy->s.number);
		}
		else {
			gi.Printf("New enemy: %i\n", g_entities[selectedAI].enemy->s.number);
		}
	}

	vec3_t		src, dest, vf;
	trace_t		trace;
	VectorCopy(ent->client->renderInfo.eyePoint, src);
	AngleVectors(ent->client->ps.viewangles, vf, NULL, NULL);//ent->client->renderInfo.eyeAngles was cg.refdef.viewangles, basically
	//extend to find end of use trace
	VectorMA(src, 32967.0f, vf, dest);

	//Trace ahead to find a valid target
	gi.trace(&trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE, G2_NOCOLLIDE, 0);
	if (trace.fraction == 1.0f || trace.entityNum < 1 || trace.entityNum == ENTITYNUM_NONE || trace.entityNum == ENTITYNUM_WORLD)
	{
		gi.Printf("Invalid entity\n");
		return;
	}

	gentity_t* target = &g_entities[trace.entityNum];
	gentity_t* selected = &g_entities[selectedAI];
	selected->lastEnemy = selected->enemy;
	selected->enemy = target;
	if (selected->lastEnemy != NULL) {
		gi.Printf("New enemy: %i (last enemy was %i)\n", selected->enemy->s.number, selected->lastEnemy->s.number);
	}
	else {
		gi.Printf("New enemy: %i\n", selected->enemy->s.number);
	}
}

// Set scriptflags
void Workshop_Set_Scriptflags_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_scriptflags <script flags>\n");
		return;
	}
	int scriptFlags = atoi(gi.argv(1));
	if (scriptFlags < 0) {
		gi.Printf("Invalid scriptflags.\n");
		return;
	}
	g_entities[selectedAI].NPC->scriptFlags = scriptFlags;
}

// Set aiflags
void Workshop_Set_Aiflags_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_aiflags <ai flags>\n");
		return;
	}
	int scriptFlags = atoi(gi.argv(1));
	if (scriptFlags < 0) {
		gi.Printf("Invalid scriptflags.\n");
		return;
	}
	g_entities[selectedAI].NPC->aiFlags = scriptFlags;
}

// Set weapon
extern stringID_table_t WPTable[];
extern void ChangeWeapon(gentity_t *ent, int newWeapon);
extern int WP_SaberInitBladeData( gentity_t *ent );
extern void G_CreateG2AttachedWeaponModel(gentity_t *ent, const char *psWeaponModel, int boltNum, int weaponNum );
extern void WP_SaberAddG2SaberModels( gentity_t *ent, int specificSaberNum = -1 );
extern void WP_SaberAddHolsteredG2SaberModels( gentity_t *ent, int specificSaberNum = -1 );
void Workshop_Set_Weapon_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_weapon <weapon name or 'me'>\n");
		return;
	}
	int weaponNum;
	if (!Q_stricmp(gi.argv(1), "me")) {
		weaponNum = g_entities[0].s.weapon;
	}
	else {
		weaponNum = GetIDForString(WPTable, gi.argv(1));
		if (weaponNum == -1) {
			gi.Printf("Invalid weapon number. Maybe try WP_NONE, or WP_BLASTER for instance?\n");
            return;
		}
	}
	gentity_t* selected = &g_entities[selectedAI];
	selected->client->ps.weapon = weaponNum;
	selected->client->ps.weaponstate = WEAPON_RAISING;
	ChangeWeapon(selected, weaponNum);
	if (weaponNum == WP_SABER)
	{
        WP_SaberInitBladeData( selected );
        WP_SaberAddG2SaberModels( selected );
        G_RemoveHolsterModels( selected );
	}
	else
	{
		G_CreateG2AttachedWeaponModel(selected, weaponData[weaponNum].weaponMdl, selected->handRBolt, 0);
        WP_SaberAddHolsteredG2SaberModels( selected );
	}
}

// Set team
void Workshop_Set_Team_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_team <team name>\n");
		return;
	}
	int team = GetIDForString(teamTable, gi.argv(1));
	if (team == -1) {
		gi.Printf("invalid team\n");
        return;
	}
	g_entities[selectedAI].client->playerTeam = (team_t)team;
}

// Set enemyteam
void Workshop_Set_Enemyteam_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_enemyteam <team name>\n");
		return;
	}
	int team = GetIDForString(teamTable, gi.argv(1));
	if (team == -1) {
		gi.Printf("invalid team\n");
        return;
	}
	g_entities[selectedAI].client->enemyTeam = (team_t)team;
}

// Set class
void Workshop_Set_Class_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_class <class name>\n");
		return;
	}
	int team = GetIDForString(NPCClassTable, gi.argv(1));
	if (team == -1) {
		gi.Printf("invalid class\n");
        return;
	}
	g_entities[selectedAI].client->NPC_class = (class_t)team;
}

// Set rank
void Workshop_Set_Rank_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_rank <rank name>\n");
		return;
	}
	int team = GetIDForString(RankTable, gi.argv(1));
	if (team == -1) {
		gi.Printf("invalid rank\n");
        return;
	}
	g_entities[selectedAI].NPC->rank = (rank_t)team;
}

// Set leader
void Workshop_Set_Leader_f(gentity_t* ent) {
	if (gi.argc() == 2) {
		// There's a second argument. Use that.
		if (!Q_stricmp(gi.argv(1), "me")) {
			g_entities[selectedAI].client->leader = g_entities;
			//g_entities[selectedAI].client->team_leader = g_entities;
			return;
		}
		else {
			g_entities[selectedAI].client->leader = &g_entities[atoi(gi.argv(1))];
			//g_entities[selectedAI].client->team_leader = &g_entities[atoi(gi.argv(1))];
		}
	}

	vec3_t		src, dest, vf;
	trace_t		trace;
	VectorCopy(ent->client->renderInfo.eyePoint, src);
	AngleVectors(ent->client->ps.viewangles, vf, NULL, NULL);//ent->client->renderInfo.eyeAngles was cg.refdef.viewangles, basically
	//extend to find end of use trace
	VectorMA(src, 32967.0f, vf, dest);

	//Trace ahead to find a valid target
	gi.trace(&trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE, G2_NOCOLLIDE, 0);
	if (trace.fraction == 1.0f || trace.entityNum < 1 || trace.entityNum == ENTITYNUM_NONE || trace.entityNum == ENTITYNUM_WORLD)
	{
		gi.Printf("Invalid entity\n");
		return;
	}

	gentity_t* target = &g_entities[trace.entityNum];
	gentity_t* selected = &g_entities[selectedAI];
	selected->client->leader = target;
}

// Set movetype
void Workshop_Set_Movetype_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: workshop_set_movetype <move type>\n");
		return;
	}
	int team = GetIDForString(MoveTypeTable, gi.argv(1));
	if (team == -1) {
		gi.Printf("invalid movetype\n");
        return;
	}
	g_entities[selectedAI].client->moveType = (movetype_t)team;
}

// Set forcepower
void Workshop_Set_Forcepower_f(gentity_t* ent) {
	if (gi.argc() != 3) {
		gi.Printf("usage: workshop_set_forcepower <force power> <rank>\n");
		return;
	}
	int power = GetIDForString(FPTable, gi.argv(1));
	if (power == -1) {
		gi.Printf("invalid force power\n");
        return;
	}
	int rank = atoi(gi.argv(2));
	if (rank < 0) {
		gi.Printf("invalid rank\n");
        return;
	}

	gentity_t* selected = &g_entities[selectedAI];
	if (rank == 0) {
		selected->client->ps.forcePowersKnown ^= (1 << power);
	}
	else {
		selected->client->ps.forcePowersKnown |= (1 << power);
	}
	selected->client->ps.forcePowerLevel[power] = rank;
}

// Activates a behavior set
void Workshop_Activate_BSet_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Printf("usage: %s <bset>\n", gi.argv(0));
		return;
	}

	int bset = GetIDForString(BSETTable, gi.argv(1));
	if (bset == -1) {
		gi.Printf("Invalid Behavior Set.\n");
		return;
	}

	gentity_t* selected = &g_entities[selectedAI];
	G_ActivateBehavior(selected, bset);
}

// Set a spawnscript, angerscript, etc
void Workshop_Set_BSetScript_f(gentity_t* ent) {
	if (gi.argc() != 3) {
		gi.Printf("usage: %s <bset> <script path>\n", gi.argv(0));
		return;
	}

	int bset = GetIDForString(BSETTable, gi.argv(1));
	if (bset == -1) {
		gi.Printf("Invalid Behavior Set.\n");
		return;
	}

	char* path = gi.argv(2);
	gentity_t* selected = &g_entities[selectedAI];
	selected->behaviorSet[bset] = G_NewString(path);
}

extern void Q3_SetParm(int entID, int parmNum, const char *parmValue);
void Workshop_Set_Parm_f(gentity_t* ent) {
	if (gi.argc() != 3) {
		gi.Printf("usage: %s <parm num> <value>\n", gi.argv(0));
		return;
	}

	int parmNum = atoi(gi.argv(1)) - 1;
	if (parmNum < 0 || parmNum >= MAX_PARMS) {
		gi.Printf("Invalid parm. Must be between 1 and 16 (inclusive)\n");
		return;
	}

	char* text = gi.argv(2);
	if (text[0] == ' ' && text[1] == '\0') {
		gi.Printf("This parm will be blanked.\n");
		text[0] = '\0';
	}

	Q3_SetParm(selectedAI, parmNum, text);
}

void Workshop_Play_Dialogue_f(gentity_t* ent) {
    gentity_t* selected = &g_entities[selectedAI];
	if (gi.argc() != 2) {
		gi.Printf("usage: %s <sound to play>\n", gi.argv(0));
		return;
	}

	char* sound = gi.argv(1);
    G_SoundOnEnt(selected, CHAN_VOICE, sound);
}

// Godmode for the NPC
void Workshop_God_f(gentity_t* ent) {
	gentity_t* selected = &g_entities[selectedAI];
	if (selected->flags & FL_GODMODE) {
		gi.Printf("godmode OFF\n");
		selected->flags ^= FL_GODMODE;
	}
	else {
		gi.Printf("godmode ON\n");
		selected->flags |= FL_GODMODE;
	}
}

// Notarget mode for the NPC
void Workshop_Notarget_f(gentity_t* ent) {
	gentity_t* selected = &g_entities[selectedAI];
	if (selected->flags & FL_NOTARGET) {
		gi.Printf("notarget OFF\n");
		selected->flags ^= FL_NOTARGET;
	}
	else {
		gi.Printf("notarget ON\n");
		selected->flags |= FL_NOTARGET;
	}
}

// Toggle the detailed display of the workshop
void Workshop_ToggleDisplay_f(gentity_t* ent) {
	showDisplay = !showDisplay;
}

// Stubs
void Workshop_Commands_f(gentity_t* ent);
void Workshop_CmdHelp_f(gentity_t* ent);

#define WSFLAG_NEEDSELECTED		1
#define WSFLAG_ONLYINWS			2

struct workshopCmd_t {
	char* command;
	char* help;
	int flags;
	void(*Command)(gentity_t*);
};

workshopCmd_t workshopCommands[] = {
	{ "aiworkshop", "Toggles the AI workshop", 0, WorkshopToggle },
	{ "workshop_commands", "Lists all AI workshop commands", 0, Workshop_Commands_f },
	{ "workshop_cmdhelp", "Provides detailed information about an AI workshop command", 0, Workshop_CmdHelp_f },
	{ "workshop_toggle_display", "Toggles the detailed display of the AI workshop", WSFLAG_ONLYINWS, Workshop_ToggleDisplay_f },
	{ "workshop_select", "Selects an NPC that you are looking at.", WSFLAG_ONLYINWS, WorkshopSelect_f },
	{ "workshop_deselect", "Deselects your currently selected NPC.", WSFLAG_NEEDSELECTED | WSFLAG_ONLYINWS, WorkshopDeselect_f },
	{ "workshop_list_bstates", "Lists all of the Behavior States that an NPC can be in.", WSFLAG_ONLYINWS, Workshop_List_BehaviorState_f },
	{ "workshop_list_scriptflags", "Lists all of the Script Flags that an NPC can have.", WSFLAG_ONLYINWS, Workshop_List_Scriptflags_f },
	{ "workshop_list_teams", "Lists all of the Teams that an NPC can belong to or fight.", WSFLAG_ONLYINWS, Workshop_List_Team_f },
	{ "workshop_list_aiflags", "Lists all of the AI Flags that an NPC can posses.", WSFLAG_ONLYINWS, Workshop_List_AIFlags_f },
	{ "workshop_list_classes", "Lists all of the Classes that an AI can be in.", WSFLAG_ONLYINWS, Workshop_List_Classes_f },
	{ "workshop_list_ranks", "Lists all of the Ranks that an AI can be.", WSFLAG_ONLYINWS, Workshop_List_Ranks_f },
	{ "workshop_list_movetypes", "Lists all of the Move Types that an AI can use.", WSFLAG_ONLYINWS, Workshop_List_Movetypes_f },
	{ "workshop_list_forcepowers", "Lists all of the Force Powers that an AI can have.", WSFLAG_ONLYINWS, Workshop_List_ForcePowers_f },
	{ "workshop_list_bsets", "Lists all of the Behavior Sets that an NPC can attain.", WSFLAG_ONLYINWS, Workshop_List_BehaviorSets_f },
	{ "workshop_view_timers", "Lists all of the timers (alive or dead) that are active on the currently selected NPC.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_View_Timers_f },
	{ "workshop_set_timer", "Sets a timer on an NPC", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Timer_f },
	{ "workshop_set_bstate", "Changes the Behavior State of an NPC", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_BehaviorState_f },
	{ "workshop_set_goalent", "Sets the NPC's navgoal to be the thing that you are looking at.\nYou can use \"me\" or an entity number as an optional argument.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_GoalEntity_f },
	{ "workshop_set_leader", "Gives the NPC a leader - the thing you are looking at.\nYou can use \"me\" or an entity number as an optional argument.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Leader_f },
	{ "workshop_set_enemy", "Gives the NPC an enemy - the thing you are looking at.\nYou can use \"me\" or an entity number as an optional argument.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Enemy_f },
	{ "workshop_set_scriptflags", "Sets the NPC's scriptflags. Use workshop_list_scriptflags to get a list of these.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Scriptflags_f },
	{ "workshop_set_weapon", "Sets the NPC's weapon. You can use \"me\" instead of a weapon name to have them change to your weapon.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Weapon_f },
	{ "workshop_set_team", "Sets the NPC's team. This does not affect who they shoot at, only who shoots at them. Change their enemyteam for that.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Team_f },
	{ "workshop_set_enemyteam", "Sets who the NPC will shoot at. Stormtroopers shoot at TEAM_PLAYER for instance.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Enemyteam_f },
	{ "workshop_set_aiflags", "Sets the NPC's AI Flags. These are a second set of temporary flags.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Aiflags_f },
	{ "workshop_set_class", "Sets the NPC's class. Note that this is sometimes ignored, like if they are using a lightsaber.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Class_f },
	{ "workshop_set_rank", "Sets the NPC's rank. Rank has some minor effects on behavior.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Rank_f },
	{ "workshop_set_movetype", "Sets the NPC's movetype. Rather self-explanatory.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Movetype_f },
	{ "workshop_set_forcepower", "Gives the NPC a force power. Setting their force power to 0 takes the power away from them.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Forcepower_f },
	{ "workshop_set_bsetscript", "Makes the NPC use an ICARUS script when a certain behaviorset is activated. These are things like angerscript, spawnscript, etc.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_BSetScript_f },
	{ "workshop_set_parm", "Sets a parm that can be read with ICARUS.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Set_Parm_f },
	{ "workshop_play_dialogue", "Plays a line of dialogue from the NPC.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Play_Dialogue_f },
	{ "workshop_activate_bset", "Activates a Behavior Set on an NPC.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Activate_BSet_f },
	{ "workshop_god", "Uses the god cheat (invincibility) on an NPC.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_God_f },
	{ "workshop_notarget", "Uses the notarget cheat (can't be seen by enemies) on an NPC. Some NPCs have notarget on by default.", WSFLAG_ONLYINWS | WSFLAG_NEEDSELECTED, Workshop_Notarget_f },
	{ "", "", 0, NULL },
};

void Workshop_Commands_f(gentity_t* ent) {
	workshopCmd_t* cmd = workshopCommands;
	while (cmd->Command) {
		Com_Printf("%s\n", cmd->command);
		cmd++;
	}
	Com_Printf("You can receive help with any of these commands using the workshop_cmdhelp command.\n");
}

void Workshop_CmdHelp_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		Com_Printf("usage: workshop_cmdhelp <command name>\n");
		return;
	}

	workshopCmd_t* cmd = workshopCommands;
	char* text = gi.argv(1);
	while (cmd->Command) {
		if (!Q_stricmp(cmd->command, text)) {
			Com_Printf("%s\n", cmd->help);
			return;
		}
		cmd++;
	}
	Com_Printf("Command not found.\n");
}

qboolean TryWorkshopCommand(gentity_t* ent) {
	if (gi.argc() == 0) {
		// how in the fuck..
		return qfalse;
	}
	char* text = gi.argv(0);
	workshopCmd_t* cmd = workshopCommands;
	while (cmd->Command) {
		if (!Q_stricmp(cmd->command, text)) {
			if (!inAIWorkshop && cmd->flags & WSFLAG_ONLYINWS) {
				Com_Printf("You need to be in the AI workshop to use that command.\n");
			}
			else if (selectedAI == ENTITYNUM_NONE && cmd->flags & WSFLAG_NEEDSELECTED) {
				Com_Printf("You need to have selected an NPC first in order to use that command.\n");
			}
			else {
				cmd->Command(ent);
			}
			return qtrue;
		}
		cmd++;
	}
	return qfalse;
}