#ifndef GAME_SERVER_ENTITIES_SPECIAL_LOVELY_H
#define GAME_SERVER_ENTITIES_SPECIAL_LOVELY_H

class CLovely : public CEntity
{
public:
	CLovely(CGameWorld *pGameWorld, vec2 Pos);
	

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	inline vec2 GetPos() const { return m_Pos; }

private:

	int m_ID;
	vec2 m_Pos;

	int m_LifeSpan;
};

#endif
