/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_VACUUM_H
#define GAME_SERVER_ENTITIES_VACUUM_H

#include <game/server/entity.h>

class CVacuum : public CEntity
{
public:
	CVacuum(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Gravity();
	virtual void CreateDeath();
	virtual void Tick();

private:

	int m_Owner;
	int m_Delay;
	int m_TimeTick;
	bool m_Kill;
	bool m_Gravity;
	vec2 m_Pos;
};

#endif