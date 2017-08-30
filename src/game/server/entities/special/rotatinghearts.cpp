#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "game/server/gamecontroller.h"

#include "rotatinghearts.h"
#include <game/server/teams.h>

CRotatingHearts::CRotatingHearts(CGameWorld *pGameWorld, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
    m_Owner = Owner;

    for(int i = 0; i < ENTITY_NUM; i ++)
        m_aIDs[i] = Server()->SnapNewID();

    GameWorld()->InsertEntity(this);
}


void CRotatingHearts::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
    for(int i = 0; i < ENTITY_NUM; i ++)
        Server()->SnapFreeID(m_aIDs[i]);
}

void CRotatingHearts::Tick()
{
    if(!GameServer()->GetPlayerChar(m_Owner))
    { 
        Reset();
        return;
    }

    m_Pos = GameServer()->GetPlayerChar(m_Owner)->m_Pos;
    const float DISTANCE = 64.f;
    int SPEED;

    for(int i = 0; i < ENTITY_NUM; i++)
    {
        SPEED = i % 2 == 0 ? 3 : -3;
        m_aRotatePos[i] = normalize(GetDir(pi/180 * ((Server()->Tick()*SPEED + 360/ENTITY_NUM*i)%360+1))) * DISTANCE;
    }
}

void CRotatingHearts::Snap(int SnappingClient)
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

    CNetObj_Pickup *apObjs[ENTITY_NUM];
    for(int i = 0; i < ENTITY_NUM; i ++)
    {
        apObjs[i] = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_aIDs[i], sizeof(CNetObj_Pickup)));

        if(!apObjs[i])
            return;

        apObjs[i]->m_X = (int)(m_Pos.x + m_aRotatePos[i].x);
        apObjs[i]->m_Y = (int)(m_Pos.y + m_aRotatePos[i].y);
        apObjs[i]->m_Type = POWERUP_HEALTH;
    }
}
