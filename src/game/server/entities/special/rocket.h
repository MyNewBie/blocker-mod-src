/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ENTITIES_ROCKET_H
#define GAME_SERVER_ENTITIES_ROCKET_H

class CRocket : public CEntity
{
public:
	CRocket(CGameWorld *pGameWorld, int Owner, vec2 Dir, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	
private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	
	bool m_Freeze;
	bool m_Gravity;
};

#endif
