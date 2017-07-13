/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/teams.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include "vacuum.h"

CVacuum::CVacuum(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE) // TODO: I don't believe this is desired to be CGameWorld::ENTTYPE_PROJECTILE? (passing 'NULL' made it be that)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Kill = false;
	m_TimeTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
}

void CVacuum::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CVacuum::Gravity()
{
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *apEnts[MAX_CLIENTS];
	
	if (!pOwnerChar)
		return;
	
	float Radius = 1000.0f;
	int Num = GameServer()->m_World.FindEntities(m_Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; i++)
	{
		if (apEnts[i] && ((CGameControllerDDRace*)GameServer()->m_pController)->m_Teams.m_Core.Team(apEnts[i]->GetPlayer()->GetCID()) != ((CGameControllerDDRace*)GameServer()->m_pController)->m_Teams.m_Core.Team(m_Owner)) /* Check Team */
			continue;

		if (apEnts[i] && GameServer()->Collision()->IntersectLine(m_Pos, apEnts[i]->m_Pos, 0x0, 0))
			continue;

		if (!apEnts[i])
			continue;

		if (apEnts[i] != pOwnerChar)
		{
			if (length(m_Pos - apEnts[i]->m_Pos) < Radius) /* Check Distance */
			{
				vec2 Temp = apEnts[i]->m_Core.m_Vel + (normalize(m_Pos - apEnts[i]->m_Pos)*GameServer()->Tuning()->m_Gravity) * 4; /* Gravity Power */
				apEnts[i]->m_Core.m_Vel = Temp;
				m_Gravity = true;
				m_Kill = true;
			}
			else if (length(m_Pos - apEnts[i]->m_Pos) > Radius)
			{
				m_Gravity = false;
				m_Kill = false;
			}

			if (m_Kill && g_Config.m_SvBlackHoleKills)
			{
				if (length(m_Pos - apEnts[i]->m_Pos) < 50) /* Check Distance To Die */
					apEnts[i]->Die(m_Owner, WEAPON_HAMMER);
			}

			/*if(m_Owner >= 0)
			    pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

			if(pOwnerChar && pOwnerChar->IsAlive())
			    TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, false, (!pOwnerChar ? -1 : pOwnerChar->Team()), (m_Owner != -1)? TeamMask : -1LL);
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, (m_Owner != -1)? TeamMask : -1LL);*/
		}
	}
}

void CVacuum::CreateDeath()
{
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	
	if (!pOwnerChar)
		return;
	
	m_Delay++;
	
	if(m_Delay > 5)
	{
		GameServer()->CreateDeath(m_Pos, m_Owner, pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner));
		m_Delay = 1;
	}
}

void CVacuum::Tick()
{
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	
	if(m_Owner != -1 && !pOwnerChar)
		Reset();

	if((Server()->Tick() - m_TimeTick) / Server()->TickSpeed() < g_Config.m_SvBlackHoleExpiretime && (pOwnerChar && !pOwnerChar->GetPlayer()->m_IsEmote)) /* Life Time */
	{
		Gravity();
		CreateDeath();
	}
	else
	{
		Reset();
		if(pOwnerChar && pOwnerChar->IsAlive() && pOwnerChar->GetPlayer() && pOwnerChar->GetPlayer()->m_Vacuum > 0) /* Item Value */
			pOwnerChar->GetPlayer()->m_Vacuum--;
	}
}