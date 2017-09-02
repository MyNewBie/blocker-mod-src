/*#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "koh.h"

CKoh::CKoh(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{


	for(int i = 0; i < MAX_PARTICLES; i ++) 
	{
		m_aIDs[i] = Server()->SnapNewID();
	}
	GameWorld()->InsertEntity(this);	
}

void CKoh::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
	for(int i = 0; i < MAX_PARTICLES; i ++) 
	{
		Server()->SnapFreeID(i);
	}
}

void CKoh::Tick()
{
	float rad     = g_Config.m_SvKOHCircleRadius;
	float TurnFac = 0.025f;

	for(int i = 0; i < MAX_BALLS; i++)
	{
		m_RotatePos[i].x = cosf(2 * pi * (i / (float)MAX_PARTICLES) + Server()->Tick()*TurnFac) * rad;
		m_RotatePos[i].y = sinf(2 * pi * (i / (float)MAX_PARTICLES) + Server()->Tick()*TurnFac) * rad;
	}
}

void CKoh::Snap(int SnappingClient)
{	
	CNetObj_Projectile *pKOH[MAX_BALLS];

	for (int i = 0; i < MAX_BALLS; i++)
	{
		
		pKOH[i] = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_aIDs[i], sizeof(CNetObj_Projectile)));
		if (pKOH[i])
		{
			pKOH[i]->m_X         = m_Pos.x + m_RotatePos[i].x;
			pKOH[i]->m_Y         = m_Pos.y + m_RotatePos[i].y;
			pKOH[i]->m_VelX      = 4;
			pKOH[i]->m_VelY      = 4;
			pKOH[i]->m_StartTick = Server()->Tick() - 4;
			pKOH[i]->m_Type      = g_Config.m_SvKOHCircleType;
		}
	}
}
*/