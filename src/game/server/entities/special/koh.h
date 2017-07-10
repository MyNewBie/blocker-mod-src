/*#ifndef GAME_SERVER_ENTITIES_SPECIAL_KOH_H
#define GAME_SERVER_ENTITIES_SPECIAL_KOH_H

class CKoh : public CEntity
{
	enum
	{
		MAX_BALLS=9 
	};

public:
	CKoh(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_aIDs[MAX_BALLS];

	vec2 m_RotatePos[MAX_BALLS];
};

#endif
*/