#ifndef GAME_SERVER_ENTITIES_SPECIAL_EPICCIRCLE_H
#define GAME_SERVER_ENTITIES_SPECIAL_EPICCIRCLE_H

class CEpicCircle : public CEntity
{
	enum
	{
		MAX_PARTICLES=9 
	};

public:
	CEpicCircle(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_aIDs[MAX_PARTICLES];

	vec2 m_RotatePos[MAX_PARTICLES];
};

#endif
