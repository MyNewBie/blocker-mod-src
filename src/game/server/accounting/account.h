/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ACCOUNT_H
#define GAME_SERVER_ACCOUNT_H

class CAccount
{
	class CPlayer *m_pPlayer;
	class CGameContext *m_pGameServer;

	class CGameContext *GameServer() const { return m_pGameServer; }
	class IStorage *Storage() const;

public:
	CAccount(class CPlayer *pPlayer, class CGameContext *pGameServer);

	void Login(const char *pUsername, const char *pPassword);
	void Register(const char *pUsername, const char *pPassword);
	void Apply();
	void Reset();
	void Delete();
	void NewPassword(const char *pNewPassword);
	bool Exists(const char * Username);

	enum { MAX_SERVER = 2 };
	int NextID();

	//bool LoggedIn(const char * Username);
	//int NameToID(const char * Username);
};

#endif
