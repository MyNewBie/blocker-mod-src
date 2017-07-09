/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ACCOUNT_DATABASE_H
#define GAME_SERVER_ACCOUNT_DATABASE_H

#include "account.h"

typedef void (*SqlResultFunction)(bool Failed, void *pResultData, void *pData);

class CAccountDatabase : public CAccount
{
private:
	static void CreateNewQuery(char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema = true);

	static void LoginResult(bool Failed, void *pResultData, void *pData);
	static void RegisterResult(bool Failed, void *pResultData, void *pData);
	static void ExistsResultRegister(bool Failed, void *pResultData, void *pData);
	static void ReloadDataResult(bool Failed, void *pResultData, void *pData);

	bool PreventInjection(const char *pSrc);

public:
	CAccountDatabase(class CPlayer *pPlayer);

	static void InitTables();
	static void InsertAccount(char *pUsername, char *pPassword, int Vip, int Pages, int Level, int Exp, char *pIp, int WeaponKits, int Slot);

	virtual void Login(const char *pUsername, const char *pPassword);
	virtual void Register(const char *pUsername, const char *pPassword);
	virtual void Apply();
	virtual void Reset();
	virtual void Delete();
	virtual void NewPassword(const char *pNewPassword);
	virtual void ApplyUpdatedData();
	virtual void ReloadUpdatedData(SqlResultFunction Func, void *pData);

	enum { MAX_SERVER = 2 };

	//bool LoggedIn(const char * Username);
	//int NameToID(const char * Username);
};

#endif
