// by Rei

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/gamecore.h>
#include "lightsaber.h"

#include <engine/shared/config.h>
#include <game/server/teams.h>

CLightSaber::CLightSaber(CGameWorld *pGameWorld, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Owner = Owner;
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	m_saberLenght = 0;

	m_Pos[POS_START] = pOwnerChar->m_Pos + GetDir(GetAngle(m_Dir)) * 20;
	m_Pos[POS_END] = pOwnerChar->m_Pos + GetDir(GetAngle(m_Dir)) * 20;
	
	GameWorld()->InsertEntity(this);	
}

void CLightSaber::Reset()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(pOwnerChar)
		pOwnerChar->m_LightSaberActivated = false;
	
	GameServer()->m_World.DestroyEntity(this); // destroy the lights	
}

void CLightSaber::HitCharacter(int Character)
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pChr = GameServer()->GetPlayerChar(Character);


	if(pChr && Character != m_Owner && pChr->Team() == pOwnerChar->Team() && distance(closest_point_on_line(m_Pos[POS_END], m_Pos[POS_START], pChr->m_Pos), pChr->m_Pos) < 28)
	{
		int64_t TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);
	 	if(!g_Config.m_SvSaberKill)
	 	{
			GameServer()->CreateSound(pOwnerChar->m_Pos, 11, TeamMask);
		 	GameServer()->CreateDeath(pChr->m_Pos, pChr->GetPlayer()->GetCID(), TeamMask);
	 	}
	 	else
	 	{
		 	GameServer()->CreateSound(pOwnerChar->m_Pos, SOUND_RIFLE_FIRE, TeamMask);
		 	pChr->Die(m_Owner, WEAPON_RIFLE);
	 	}
	}	
}

void CLightSaber::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if((pOwnerChar && pOwnerChar->Core()->m_ActiveWeapon != WEAPON_RIFLE) || !pOwnerChar || !GameServer()->m_apPlayers[m_Owner]->m_LightSaber)
	{
		Reset();
		return;
	}

	if(!pOwnerChar->m_LightSaberActivated)
	{
		m_saberLenght-=15;
		if(m_saberLenght <= 0)
			Reset();
	}

	m_Dir = normalize(vec2(pOwnerChar->LatestInput()->m_TargetX, pOwnerChar->LatestInput()->m_TargetY));
	m_Pos[POS_END] = pOwnerChar->m_Pos + GetDir(GetAngle(m_Dir)) * 20;
	m_Pos[POS_START] = m_Pos[POS_END] + GetDir(GetAngle(m_Dir)) * m_saberLenght;
	
	vec2 CollisionPos;
	if(GameServer()->Collision()->IntersectLine(m_Pos[POS_END], m_Pos[POS_START], &CollisionPos, NULL))
		m_Pos[POS_START] = CollisionPos;
	
	for(int i = 0; i < MAX_CLIENTS; i++)
		HitCharacter(i);

	if(m_saberLenght < g_Config.m_ClSaberLenght && pOwnerChar->m_LightSaberActivated)
		m_saberLenght += 15;

	else if(m_saberLenght > g_Config.m_ClSaberLenght && pOwnerChar->m_LightSaberActivated)
		m_saberLenght -= 15;
}

void CLightSaber::Snap(int SnappingClient)
{
	CCharacter *pOwnerChar = 0;
	int64_t TeamMask = -1LL;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if (pOwnerChar && pOwnerChar->IsAlive())
			TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

	if(!CmaskIsSet(TeamMask, SnappingClient))
		return;

	CNetObj_Laser *pObj;	
	pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos[POS_START].x;
	pObj->m_Y = (int)m_Pos[POS_START].y;
	pObj->m_FromX = (int)m_Pos[POS_END].x;
	pObj->m_FromY = (int)m_Pos[POS_END].y;
	pObj->m_StartTick = Server()->Tick();
}
