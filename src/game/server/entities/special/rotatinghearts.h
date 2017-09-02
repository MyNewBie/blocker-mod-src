#ifndef GAME_SERVER_ENTITIES_SPECIAL_ROTATINGHEARTS_H
#define GAME_SERVER_ENTITIES_SPECIAL_ROTATINGHEARTS_H

#include <game/server/entity.h>

class CRotatingHearts : public CEntity
{
    enum
    {
        ENTITY_NUM = 24,
    };
public:
	CRotatingHearts(CGameWorld *pGameWorld, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	vec2 m_aRotatePos[ENTITY_NUM];
	int m_aIDs[ENTITY_NUM];
};

#endif
