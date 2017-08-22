/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_MOD_LIGHTSABER_H
#define GAME_SERVER_ENTITIES_MOD_LIGHTSABER_H

#include <game/server/entity.h>

class CLightSaber : public CEntity
{
	enum
	{
		POS_START = 0,
		POS_END,
	};
	
public:
	CLightSaber(CGameWorld *pGameWorld, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	void HitCharacter(int Character);

private:
	int m_Owner;

	int m_iLifespan;

	vec2 m_Pos[2];
	vec2 m_OwnerPos;
	vec2 m_Dir;

	int m_saberLenght;
};

#endif
