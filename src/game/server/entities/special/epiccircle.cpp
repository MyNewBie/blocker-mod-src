/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "epiccircle.h"

CEpicCircle::CEpicCircle(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Owner = Owner;
	m_Pos = Pos;

	for(int i = 0; i < MAX_PARTICLES; i ++) 
	{
		m_aIDs[i] = Server()->SnapNewID();
	}
	GameWorld()->InsertEntity(this);
}

void CEpicCircle::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
	for(int i = 0; i < MAX_PARTICLES; i++)
   	{
   		Server()->SnapFreeID(m_aIDs[i]);
   	}
}

void CEpicCircle::Tick()
{
	if(!GameServer()->GetPlayerChar(m_Owner) || !GameServer()->GetPlayerChar(m_Owner)->IsAlive())
	{
		Reset();
		return;
	}

	if(GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_InLMB == 2)
    {
    	Reset();
    	return;
    }
	
	if (GameServer()->m_KOHActive)	
	{
		GameServer()->SendChatTarget(m_Owner, "For the greater good, we disabled your epic circles :)");
		GameServer()->m_apPlayers[m_Owner]->m_EpicCircle = false;
		Reset();
	}

	m_Pos = GameServer()->GetPlayerChar(m_Owner)->m_Pos;

	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		float rad = 16.0f * powf(sinf(Server()->Tick() / 30.0f), 3) * 1 + 50;
		float TurnFac = 0.025f;
		m_RotatePos[i].x = cosf(2 * pi * (i / (float)MAX_PARTICLES) + Server()->Tick()*TurnFac) * rad;
		m_RotatePos[i].y = sinf(2 * pi * (i / (float)MAX_PARTICLES) + Server()->Tick()*TurnFac) * rad;
	}
}

void CEpicCircle::Snap(int SnappingClient)
{	
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter *pOwnerChar = 0;
    int64_t TeamMask = -1LL;

    if(m_Owner >= 0)
        pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

    if (pOwnerChar && pOwnerChar->IsAlive())
            TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

    if(!CmaskIsSet(TeamMask, SnappingClient))
        return;

	CNetObj_Projectile *pParticle[MAX_PARTICLES];
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		pParticle[i] = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[i], sizeof(CNetObj_Projectile)));
		if (pParticle[i])
		{
			pParticle[i]->m_X = m_Pos.x + m_RotatePos[i].x;
			pParticle[i]->m_Y = m_Pos.y + m_RotatePos[i].y;
			pParticle[i]->m_VelX = 4;
			pParticle[i]->m_VelY = 4;
			pParticle[i]->m_StartTick = Server()->Tick() - 4;
		}
	}
}
