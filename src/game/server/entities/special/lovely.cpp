/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "lovely.h"

CLovely::CLovely(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	/*m_Owner = Owner;*/
	m_Pos = Pos;

  	m_LifeSpan = Server()->TickSpeed()/2;

	m_ID = Server()->SnapNewID();
  	GameWorld()->InsertEntity(this);
}

void CLovely::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
	Server()->SnapFreeID(m_ID);
}

void CLovely::Tick()
{
	m_LifeSpan--;
	if(m_LifeSpan < 0)
	{
		Reset();
	}

	m_Pos.y -= 5.0f;
}

void CLovely::Snap(int SnappingClient)
{	
	if(NetworkClipped(SnappingClient))
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
