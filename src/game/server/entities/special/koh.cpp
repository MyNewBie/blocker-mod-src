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
	
}

void CKoh::Tick()
{
	
}

void CKoh::Snap(int SnappingClient)
{	
	
}
*/