/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ACCOUNT_H
#define GAME_SERVER_ACCOUNT_H

class CPlayer;
class IStorage;
class CGameContext;

class CAccount
{
protected:
	CPlayer *m_pPlayer;
	IStorage *m_pStorage;
	CGameContext *m_pGameServer;

public:
	CAccount(CPlayer *pPlayer);
	void SetStorage(IStorage *pStorage) { m_pStorage = pStorage; }

	virtual void Login(const char *pUsername, const char *pPassword) = 0;
	virtual void Register(const char *pUsername, const char *pPassword) = 0;
	virtual void Apply() = 0;
	virtual void Reset() = 0;
	virtual void Delete() = 0;
	virtual void NewPassword(const char *pNewPassword) = 0;
	virtual bool Exists(const char * Username) { return true; };

	//bool LoggedIn(const char * Username);
	//int NameToID(const char * Username);
	IStorage *Storage() { return m_pStorage; }
	CGameContext *GameServer() { return m_pGameServer; }
};

#endif
