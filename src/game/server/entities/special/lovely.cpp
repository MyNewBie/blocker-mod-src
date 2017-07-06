/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "lovely.h"

CLovely::CLovely(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Owner = Owner;
	m_Pos = Pos;

  	m_LifeSpan = Server()->TickSpeed()/2;

  	GameWorld()->InsertEntity(this);
}

void CLovely::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLovely::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	m_LifeSpan--;
	if(m_LifeSpan < 0)
	{
		Reset();
		return;
	}

	if(pOwnerChar && GameServer()->Collision()->IntersectLine(pOwnerChar->m_Pos, vec2(m_Pos.x, m_Pos.y-10), NULL, NULL))
	{
		Reset();
		return;
	}

	if(GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_InLMB == 2)
    {
    	Reset();
    	return;
    }

	m_Pos.y -= 5.0f;
}

void CLovely::Snap(int SnappingClient)
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
	
	CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(pObj)
	{
		pObj->m_X = m_Pos.x;
		pObj->m_Y = m_Pos.y;
		pObj->m_Type = POWERUP_HEALTH;
		pObj->m_Subtype = 0;
	}
}
