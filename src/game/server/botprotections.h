#pragma once

class CPlayer;
class CGameContext;
class CServer;

class CBotProtections
{
private:
	CPlayer *m_pPlayer;
	CGameContext *m_pGameContext;
	IServer *m_pServer;

	//hook assistance
	bool m_HAFinishedAnalyse;
	bool m_HAHooking;
	int m_HAHookingTee;
	int m_HALastFire;
	unsigned int m_HACountTotal;
	unsigned int m_HACountConspicuous;
	int64 m_HALastGotHooked;
	int64 m_HALastHammered;

	void HAGetHookedData();
	void HAGetHammeredData(CNetObj_PlayerInput *pInput);
	void HATick();
	void HAInspectInput(CNetObj_PlayerInput *pInput);
	void HANewInput(CNetObj_PlayerInput *pInput);

public:
	CBotProtections(CPlayer *pPlayer);

	void Tick();
	void NewInput(CNetObj_PlayerInput *pInput);

	unsigned int GetHACountTotal() const { return m_HACountTotal; }
	unsigned int GetHACountConspicuous() const { return m_HACountConspicuous; }
	float GetHACountRatio() { return m_HACountTotal > 0 ? m_HACountConspicuous / (float) m_HACountTotal : 0; }

	CPlayer *Player() const { return m_pPlayer; }
	CGameContext *GameServer() const { return m_pGameContext; }
	IServer *Server() const { return m_pServer; }
};