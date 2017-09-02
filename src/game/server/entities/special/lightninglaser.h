/*
	by Loic KASSEL (Rei)
*/
#ifndef GAME_SERVER_ENTITIES_SPECIAL_LIGHTNINGLASER_H
#define GAME_SERVER_ENTITIES_SPECIAL_LIGHTNINGLASER_H

#include <game/server/entity.h>

#define LIGHTNING_COUNT 7
#define LIGHTNING_LEN 70.f

class CLightningLaser : public CEntity
{
	enum
	{
		POS_START = 0,
		POS_END,

		POS_COUNT
	};
	
public:
	CLightningLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	void HitCharacter();
	void GenerateLights();

private:
	vec2 m_Dir;
	int m_Owner;
	
	int m_iLifespan;
	int m_iStartLifespan;
	
	int m_TeamMask;

	float m_iStartTick;

	int m_aiIDs[LIGHTNING_COUNT];
	vec2 m_aaPositions[LIGHTNING_COUNT][POS_COUNT];

	int m_LockedPlayer;
};

#endif
