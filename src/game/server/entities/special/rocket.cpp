/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/teams.h>
#include "rocket.h"

CRocket::CRocket(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Direction = Dir;
	m_LifeSpan = 10 * Server()->TickSpeed();
	m_Owner = Owner;
	m_Pos = Pos;

	GameWorld()->InsertEntity(this);
}

void CRocket::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CRocket::Tick()
{	
	int64_t TeamMask = -1LL;
	const bool Collide = GameServer()->Collision()->CheckPoint(m_Pos);

  CCharacter *pOwnerChar = 0;
	CCharacter *apEnts[MAX_CLIENTS];
	
	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	
	if(pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);
	
	if (!pOwnerChar)
		return;
	
	float Radius = 1000.0f;
	m_Pos += m_Direction * 15; /* Rocket Speed */
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
				m_Freeze = true;
			}
			else if (length(m_Pos - apEnts[i]->m_Pos) > Radius)
			{
				m_Gravity = false;
				m_Freeze = false;
			}

			if (m_Freeze)
			{
				if (length(m_Pos - apEnts[i]->m_Pos) < 50) /* Check Distance */
					apEnts[i]->Freeze();
			}
		}
	}

	m_LifeSpan--;

	if(Collide || m_LifeSpan < 0 || GameLayerClipped(m_Pos))
	{
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, (m_Owner != -1)? TeamMask : -1LL);
		Reset();
		return;
	}

	GameServer()->CreateExplosion(m_Pos, m_Owner, -1, true, (!pOwnerChar ? -1 : pOwnerChar->Team()), (m_Owner != -1)? TeamMask : -1LL);
}