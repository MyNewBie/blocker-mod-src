/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_ACCOUNT_FILE_H
#define GAME_SERVER_ACCOUNT_FILE_H

#include "account.h"

class CAccountFile : public CAccount
{

public:
	CAccountFile(class CPlayer *pPlayer);

	void Login(const char *pUsername, const char *pPassword);
	void Register(const char *pUsername, const char *pPassword);
	void Apply();
	void Reset();
	void Delete();
	void NewPassword(const char *pNewPassword);
	bool IsValidChar(const char *pUsername);
	bool IsCorrectSizeData(const char *pUsername, const char *pPassword);
	bool Exists(const char * Username);

	int NextID();
};

#endif
