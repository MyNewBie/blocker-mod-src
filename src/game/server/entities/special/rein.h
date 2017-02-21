/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_REIN_H
#define GAME_SERVER_ENTITIES_REIN_H

#include <game/server/entity.h>

class CSpecial2: public CEntity
{
public:
	CSpecial2(CGameWorld *pGameWorld, vec2 Pos, int Owner);
	
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	vec2 m_ReinPos;
private:
	int m_Owner;
	
	vec2 m_Pos;
};

#endif