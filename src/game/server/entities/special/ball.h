#ifndef GAME_SERVER_ENTITIES_SPECIAL_BALL_H
#define GAME_SERVER_ENTITIES_SPECIAL_BALL_H

class CBall : public CEntity
{
public:
	CBall(CGameWorld *pGameWorld, vec2 Pos, int Owner);
	

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	inline vec2 GetPos() const { return m_Pos; }

private:

	int m_Owner;
	int m_aIDs[2];


	int m_LaserLifeSpan;
	int m_LaserDirAngle;
	int m_LaserInputDir;
	bool m_IsRotating;

	vec2 m_Pos2;

	int m_TableDirV[4];
};

#endif
