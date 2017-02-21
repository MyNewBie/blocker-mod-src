/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/teams.h>
#include <engine/server/server.h>
#include "rein.h"

CSpecial2::CSpecial2(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Owner = Owner;

	GameWorld()->InsertEntity(this);
}

void CSpecial2::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CSpecial2::Tick()
{
	CCharacter *pChr = static_cast<CCharacter *>(GameServer()->m_World.ClosestCharacter(m_Pos, 64.0f, NULL));
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(GameServer()->m_World.m_Paused == true)
		GameWorld()->DestroyEntity(this);

	if(m_Owner != -1 && !pOwnerChar)
		Reset();
	
	if(pOwnerChar && pOwnerChar->GetPlayer()->m_IsEmote)
	{
		pOwnerChar->GetPlayer()->m_IsLimited = false;
		Reset();
	}
	
	if(m_Owner != -1)
	{
		if(pOwnerChar)
		{
			vec2 m_ReinPos = vec2(pOwnerChar->m_LatestInput.m_TargetX, pOwnerChar->m_LatestInput.m_TargetY)+pOwnerChar->m_Pos;
			m_Pos = m_ReinPos;
		}

		if(pChr && pChr->GetPlayer()->GetCID() != m_Owner)
		{
			/*GameServer()->m_World.m_Core.m_apCharacters[pChr->GetPlayer()->GetCID()]->m_Pos = vec2(pOwnerChar->m_LatestInput.m_TargetX, pOwnerChar->m_LatestInput.m_TargetY)+pOwnerChar->m_Pos;*/
			/*GameServer()->m_World.m_Core.m_apCharacters[pChr->GetPlayer()->GetCID()]->m_Vel = vec2((m_Pos.x-pChr->m_Core.m_Pos.x)/3, (m_Pos.y-pChr->m_Core.m_Pos.y)/3);*/
			if(pOwnerChar && !pOwnerChar->GetPlayer()->m_IsRelease)
			{
			    if(((CGameControllerDDRace*)GameServer()->m_pController)->m_Teams.m_Core.Team(pChr->GetPlayer()->GetCID()) == ((CGameControllerDDRace*)GameServer()->m_pController)->m_Teams.m_Core.Team(m_Owner) && pChr->GetPlayer()->m_Authed != CServer::AUTHED_ADMIN)
				{
					pChr->m_Core.m_Pos = vec2(pOwnerChar->m_LatestInput.m_TargetX, pOwnerChar->m_LatestInput.m_TargetY)+pOwnerChar->m_Pos;
					pChr->m_Core.m_Vel = vec2((m_Pos.x-pChr->m_Core.m_Pos.x)/3, (m_Pos.y-pChr->m_Core.m_Pos.y)/3);
				}
			}
		}
	}
}

void CSpecial2::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos))
		return;
	
	CCharacter *pOwnerChar = 0;
	int64_t TeamMask = -1LL;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if (pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

	if(m_Owner != -1 && !CmaskIsSet(TeamMask, SnappingClient))
		return;
	
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser*>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;
	
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
}