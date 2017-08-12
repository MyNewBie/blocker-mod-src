/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "ball.h"

CBall::CBall(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Owner = Owner;
	m_Pos = Pos;

	m_IsRotating = true;

	m_LaserLifeSpan = Server()->TickSpeed();
	m_LaserDirAngle = 0;
	m_LaserInputDir = 0;
    
    m_TableDirV[0] = 5;
    m_TableDirV[1] = 10;
    m_TableDirV[2] = -5;
    m_TableDirV[3] = -10;

	for(int i = 0; i < 2 ; i ++) 
		m_aIDs[i] = Server()->SnapNewID();

  	GameWorld()->InsertEntity(this);
}

void CBall::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
	for(int i = 0; i < 2 ; i ++) 
   		Server()->SnapFreeID(m_aIDs[i]);
}

void CBall::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!pOwnerChar || !pOwnerChar->IsAlive())
    {
        Reset();
        return;
    }

    if(GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_InLMB == 2)
    {
    	Reset();
    	return;
    }

    m_LaserLifeSpan--;

    if(m_LaserLifeSpan <= 0)
    {
    	m_IsRotating ^= true;
    	m_LaserInputDir = m_TableDirV[rand()%4];
    	m_LaserLifeSpan = m_IsRotating ? Server()->TickSpeed() + (rand() % 20 - 10) : Server()->TickSpeed() + 30;
    }   

	
	if(m_IsRotating)
		m_LaserDirAngle += m_LaserInputDir;
          
    m_Pos.x = pOwnerChar->m_Pos.x + 70 * sin(m_LaserDirAngle * pi/180.0f);
    m_Pos.y = pOwnerChar->m_Pos.y + 70 * cos(m_LaserDirAngle * pi/180.0f); 

    m_Pos2.x = m_Pos.x + 20 * sin(Server()->Tick()*13 * pi/180.0f);
    m_Pos2.y = m_Pos.y + 20 * cos(Server()->Tick()*13 * pi/180.0f);
}

void CBall::Snap(int SnappingClient)
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

	CNetObj_Laser *pObj;
	pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aIDs[0], sizeof(CNetObj_Laser)));

	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = Server()->Tick();

	CNetObj_Projectile *pObj2 = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[1], sizeof(CNetObj_Projectile)));
	if(!pObj2)
		return;

	pObj2->m_X = (int)m_Pos2.x;
	pObj2->m_Y = (int)m_Pos2.y;
}
