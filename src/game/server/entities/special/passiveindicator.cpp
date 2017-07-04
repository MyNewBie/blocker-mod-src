/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "passiveindicator.h"

CPassiveIndicator::CPassiveIndicator(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Owner = Owner;
	m_Pos = Pos;

  	GameWorld()->InsertEntity(this);
}

void CPassiveIndicator::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CPassiveIndicator::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwnerChar || !pOwnerChar->IsAlive())
	{	
		Reset();
		return;
	}

	if(!pOwnerChar->m_PassiveMode)
	{
		GameServer()->CreateDeath(m_Pos, m_Owner, pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner));
		Reset();
		return;
	}     

	m_Pos = vec2(pOwnerChar->m_Pos.x, pOwnerChar->m_Pos.y - 50);
}

void CPassiveIndicator::Snap(int SnappingClient)
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
		pObj->m_Type = POWERUP_ARMOR;
		pObj->m_Subtype = 0;
	}
}
