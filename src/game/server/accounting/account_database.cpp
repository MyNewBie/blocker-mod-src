/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

// TODO: most of this includes can probably be removed
#include <string.h>
#include <fstream>
#include <engine/config.h> 
#if defined(CONF_FAMILY_WINDOWS)
#include <tchar.h>
#include <direct.h>
#endif
#if defined(CONF_FAMILY_UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(CONF_SQL)
#include <cppconn/resultset.h>
#endif

#include <base/system.h>
#include <engine/storage.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <game/server/player.h>
//#include <game/server/gamecontext.h>

#define THREADING 1

#include "account.h"
#include "account_database.h"

struct CResultData
{
	char m_aUsername[32];
	char m_aPassword[32];
	int m_ID;
	CGameContext *m_pGameServer;
};

struct CResultDataReload
{
	void *m_pData;
	CAccountDatabase *m_pAccount;
	SqlResultFunction Func;
};

CAccountDatabase::CAccountDatabase(CPlayer *pPlayer)
		: CAccount(pPlayer)
{
#if defined(CONF_SQL)
	Init(g_Config.m_SvAccSqlIp, g_Config.m_SvAccSqlName, g_Config.m_SvAccSqlPassword, g_Config.m_SvAccSqlDatabase);
#endif
}

void CAccountDatabase::InitTables()
{
	//Init schema
	char aBuf[64];

#if defined(CONF_SQL)
	str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", g_Config.m_SvAccSqlDatabase);
	CreateNewQuery(g_Config.m_SvAccSqlIp, g_Config.m_SvAccSqlName, g_Config.m_SvAccSqlPassword, g_Config.m_SvAccSqlDatabase, aBuf, NULL, NULL, false, true, THREADING);

	CreateNewQuery(g_Config.m_SvAccSqlIp, g_Config.m_SvAccSqlName, g_Config.m_SvAccSqlPassword, g_Config.m_SvAccSqlDatabase, 
		"CREATE TABLE IF NOT EXISTS accounts (username VARCHAR(32) BINARY NOT NULL, password VARCHAR(32) BINARY NOT NULL, vip INT DEFAULT 0, pages INT DEFAULT 0, level INT DEFAULT 1, exp INT DEFAULT 0, ip VARCHAR(47), weaponkits INT DEFAULT 0, slot INT DEFAULT 0,  PRIMARY KEY (username)) CHARACTER SET utf8 ;", NULL, NULL, false, true, THREADING);
#endif
}

void CAccountDatabase::InsertAccount(char *pUsername, char *pPassword, int Vip, int Pages, int Level, int Exp, char *pIp, int WeaponKits, int Slot)
{
#if defined(CONF_SQL)

	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "INSERT INTO accounts VALUES('%s', '%s', %i, %i, %i, %i, '%s', %i, %i)",
		pUsername, pPassword, Vip, Pages, Level, Exp, pIp, WeaponKits, Slot);

	CreateNewQuery(g_Config.m_SvAccSqlIp, g_Config.m_SvAccSqlName, g_Config.m_SvAccSqlPassword, g_Config.m_SvAccSqlDatabase, aQuery, NULL, NULL, false, true, THREADING);
#endif
}

void CAccountDatabase::LoginResult(bool Failed, void *pResultData, void *pData)
{

	char aUsername[32], aPassword[32];
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	int ClientID = pResult->m_ID;
	CGameContext *pGameServer = pResult->m_pGameServer;
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(Failed == false && pResultData != NULL && pPlayer != NULL)
	{
#if defined(CONF_SQL)
		CAccount *pAccount = pPlayer->m_pAccount;

		DatabaseStringCopyRevert(aUsername, pResult->m_aUsername, sizeof(aUsername));
		DatabaseStringCopyRevert(aPassword, pResult->m_aPassword, sizeof(aPassword));

		sql::ResultSet *pResults = (sql::ResultSet *)pResultData;
		sql::ResultSetMetaData *pResultsMeta = pResults->getMetaData();
		if(pResults->isLast() == true)
		{
			dbg_msg("account", "Account login failed ('%s' - Missing)", aUsername);
			pGameServer->SendChatTarget(ClientID, "This account does not exist.");
			pGameServer->SendChatTarget(ClientID, "Please register first. (/register <user> <pass>)");
			return;
		}

		//fill account
		pResultsMeta = pResults->getMetaData();
		int ColumnCount = pResultsMeta->getColumnCount();

		pResults->last();

		if(str_comp(aPassword, pResults->getString(2).c_str()) != 0)
		{
			dbg_msg("account", "Account login failed ('%s' Wrong password)", aUsername);
			pGameServer->SendChatTarget(ClientID, "Wrong username or password");
			return;
		}

		for(int i = 0; i < ColumnCount; i++)
		{
			sql::SQLString str = pResults->getString(i + 1);

			switch(i)
			{
			case 0: str_copy(pPlayer->m_AccData.m_aUsername, str.c_str(), sizeof(pPlayer->m_AccData.m_aUsername)); break;
			case 1: str_copy(pPlayer->m_AccData.m_aPassword, str.c_str(), sizeof(pPlayer->m_AccData.m_aPassword)); break;
			case 2: pPlayer->m_AccData.m_Vip = str_toint(str.c_str()); break;
			case 3: pPlayer->m_QuestData.m_Pages = str_toint(str.c_str()); break;
			case 4: pPlayer->m_Level.m_Level = str_toint(str.c_str()); break;
			case 5: pPlayer->m_Level.m_Exp = str_toint(str.c_str()); break;
			case 6: str_copy(pPlayer->m_AccData.m_aIp, str.c_str(), sizeof(pPlayer->m_AccData.m_aIp)); break;
			case 7: pPlayer->m_AccData.m_Weaponkits = str_toint(str.c_str()); break;
			case 8: pPlayer->m_AccData.m_Slot = str_toint(str.c_str()); break;
			}
		}

#endif

		pPlayer->m_AccData.m_UserID = 1;//logged in

		CCharacter *pOwner = pPlayer->GetCharacter();
		if (pOwner && pOwner->IsAlive())
			pOwner->Die(ClientID, WEAPON_GAME);

		if (pPlayer->GetTeam() == TEAM_SPECTATORS)
			pPlayer->SetTeam(TEAM_RED);

		dbg_msg("account", "Account login sucessful ('%s')", aUsername);
		pGameServer->SendChatTarget(ClientID, "Login successful");

		if (pOwner)
		{
			pPlayer->m_AccData.m_Slot++;
			pPlayer->m_DeathNote = true;
			pGameServer->SendChatTarget(ClientID, "You have reveived a Deathnote.");
			pGameServer->SendChatTarget(ClientID, "Write /Deathnoteinfo for more information");
#if defined(CONF_SQL)
			pAccount->Apply();
#endif

			/*if(pOwner->GetPlayer()->m_AccData.m_Slot > 1)
			{
				dbg_msg("account", "Account login failed ('%s' - already in use (extern))", pResult->m_aUsername);
				pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Account already in use.");
				pAccount->m_pPlayer->m_pAccount->SetStorage(pAccount->Storage());
				pAccount->m_pPlayer->m_AccData.m_Slot--;
				pAccount->m_pPlayer->m_pAccount->Apply();
				pAccount->m_pPlayer->m_pAccount->Reset();
			}*/
		}
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pGameServer->SendChatTarget(ClientID, "Internal Server Error. Please contact an admin. (0)");
	}
}

void CAccountDatabase::Login(const char *pUsername, const char *pPassword)
{
	char aUsername[32], aPassword[32];

	if (m_pPlayer->m_LastLoginAttempt + 3 * GameServer()->Server()->TickSpeed() > GameServer()->Server()->Tick())
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "You have to wait %d seconds until you attempt to login again", (m_pPlayer->m_LastLoginAttempt + 3 * GameServer()->Server()->TickSpeed() - GameServer()->Server()->Tick()) / GameServer()->Server()->TickSpeed());
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}

	m_pPlayer->m_LastLoginAttempt = GameServer()->Server()->Tick();
	char aBuf[128];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		dbg_msg("account", "Account login failed ('%s' - Already logged in)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Already logged in");
		return;
	}
	else if (str_length(pUsername) > 15 || !str_length(pUsername))
	{
		str_format(aBuf, sizeof(aBuf), "Username too %s", str_length(pUsername) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}
	else if (str_length(pPassword) > 15 || !str_length(pPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Password too %s!", str_length(pPassword) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}
	
	DatabaseStringCopy(aUsername, pUsername, sizeof(aUsername));
	DatabaseStringCopy(aPassword, pPassword, sizeof(aPassword));

	for (int j = 0; j < MAX_CLIENTS; j++)
	{
		if (GameServer()->m_apPlayers[j] && str_comp(GameServer()->m_apPlayers[j]->m_AccData.m_aUsername, aUsername) == 0)
		{
			dbg_msg("account", "Account login failed ('%s' - already in use (local))", aUsername);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use");
			return;
		}
	}


	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM accounts WHERE username='%s'", aUsername);
	CResultData *pResult = new CResultData();
	str_copy(pResult->m_aUsername, aUsername, sizeof(pResult->m_aUsername));
	str_copy(pResult->m_aPassword, aPassword, sizeof(pResult->m_aPassword));
	pResult->m_pGameServer = GameServer();
	pResult->m_ID = m_pPlayer->GetCID();
	CreateNewQuery(aQuery, LoginResult, pResult, true, true, THREADING);
}

void CAccountDatabase::RegisterResult(bool Failed, void *pResultData, void *pData)
{
	char aUsername[32], aPassword[32];
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	int ClientID = pResult->m_ID;
	CGameContext *pGameServer = pResult->m_pGameServer;
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(Failed == false && pPlayer != NULL)
	{
		DatabaseStringCopyRevert(aUsername, pResult->m_aUsername, sizeof(aUsername));
		DatabaseStringCopyRevert(aPassword, pResult->m_aPassword, sizeof(aPassword));

		dbg_msg("account", "Registration successful ('%s')", aUsername);
		str_format(aBuf, sizeof(aBuf), "Registration successful - ('/login %s %s'): ", aUsername, aPassword);
		pGameServer->SendChatTarget(ClientID, aBuf);
		pPlayer->m_pAccount->Login(aUsername, aPassword);
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pGameServer->SendChatTarget(ClientID, "Internal Server Error. Please contact an admin. (1)");
	}
}

void CAccountDatabase::ExistsResultRegister(bool Failed, void *pResultData, void *pData)
{
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	int ClientID = pResult->m_ID;
	CGameContext *pGameServer = pResult->m_pGameServer;
	CPlayer *pPlayer = pGameServer->m_apPlayers[ClientID];

	if(Failed == false && pResultData != 0x0 && pPlayer != NULL)
	{

#if defined(CONF_SQL)
		sql::ResultSet *pResults = (sql::ResultSet *)pResultData;
		if(pResults->isLast() == false)
		{
			dbg_msg("account", "Account registration failed ('%s' - Already exists)", pResult->m_aUsername);
			pGameServer->SendChatTarget(ClientID, "Account already exists.");
			return;
		}
#endif

		char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "INSERT INTO accounts VALUES('%s', '%s', %i, %i, %i, %i, '%s', %i, %i)",
			pResult->m_aUsername, pResult->m_aPassword, pPlayer->m_AccData.m_Vip, pPlayer->m_QuestData.m_Pages, pPlayer->m_Level.m_Level,
			pPlayer->m_Level.m_Exp, pPlayer->m_AccData.m_aIp, pPlayer->m_AccData.m_Weaponkits, pPlayer->m_AccData.m_Slot);

		((CAccountDatabase *)pPlayer->m_pAccount)->CreateNewQuery(aQuery, RegisterResult, pResult, false, true, THREADING);
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pGameServer->SendChatTarget(ClientID, "Internal Server Error. Please contact an admin. (2)");
	}
}


void CAccountDatabase::Register(const char *pUsername, const char *pPassword)
{
	char aUsername[32], aPassword[32];
	char aBuf[125];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		dbg_msg("account", "Account registration failed ('%s' - Logged in)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Already logged in");
		return;
	}
	if (str_length(pUsername) > 15 || !str_length(pUsername))
	{
		str_format(aBuf, sizeof(aBuf), "Username too %s", str_length(pUsername) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}
	else if (str_length(pPassword) > 15 || !str_length(pPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Password too %s!", str_length(pPassword) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}
	
	DatabaseStringCopy(aUsername, pUsername, sizeof(aUsername));
	DatabaseStringCopy(aPassword, pPassword, sizeof(aPassword));


	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM accounts WHERE username='%s'", aUsername);
	CResultData *pResult = new CResultData();
	str_copy(pResult->m_aUsername, aUsername, sizeof(pResult->m_aUsername));
	str_copy(pResult->m_aPassword, aPassword, sizeof(pResult->m_aPassword));
	pResult->m_pGameServer = GameServer();
	pResult->m_ID = m_pPlayer->GetCID();
	CreateNewQuery(aQuery, ExistsResultRegister, pResult, true, true, THREADING);
}

void CAccountDatabase::Apply()
{
	char aUsername[32], aPassword[32];

	if(m_pPlayer->m_AccData.m_UserID == 0)
		return;

	DatabaseStringCopy(aUsername, m_pPlayer->m_AccData.m_aUsername, sizeof(aUsername));
	DatabaseStringCopy(aPassword, m_pPlayer->m_AccData.m_aPassword, sizeof(aPassword));

	char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "UPDATE  accounts SET username='%s', password='%s', vip=%i, level=%i, exp=%i, ip='%s', weaponkits=%i, slot=%i WHERE username='%s'",
			aUsername, aPassword, m_pPlayer->m_AccData.m_Vip, m_pPlayer->m_Level.m_Level,
			m_pPlayer->m_Level.m_Exp, m_pPlayer->m_AccData.m_aIp, m_pPlayer->m_AccData.m_Weaponkits, m_pPlayer->m_AccData.m_Slot, aUsername);

	CreateNewQuery(aQuery, NULL, NULL, false, true, THREADING);
}

void CAccountDatabase::ApplyUpdatedData()
{
	char aUsername[32], aPassword[32];
	char aQuery[QUERY_MAX_LEN];

	DatabaseStringCopy(aUsername, m_pPlayer->m_AccData.m_aUsername, sizeof(aUsername));

		str_format(aQuery, sizeof(aQuery), "UPDATE  accounts SET pages=%i WHERE username='%s'",
			m_pPlayer->m_QuestData.m_Pages, aUsername);

	CreateNewQuery(aQuery, NULL, NULL, false, true, THREADING);
}

void CAccountDatabase::Reset()
{
	mem_zero(m_pPlayer->m_AccData.m_aUsername, sizeof(m_pPlayer->m_AccData.m_aUsername));
	mem_zero(m_pPlayer->m_AccData.m_aPassword, sizeof(m_pPlayer->m_AccData.m_aPassword));
	mem_zero(m_pPlayer->m_AccData.m_aRconPassword, sizeof(m_pPlayer->m_AccData.m_aRconPassword));
	mem_zero(m_pPlayer->m_AccData.m_aIp, sizeof(m_pPlayer->m_AccData.m_aIp));
	m_pPlayer->m_AccData.m_UserID = 0;
	m_pPlayer->m_AccData.m_Vip = 0;
	m_pPlayer->m_QuestData.m_Pages = 0;
	m_pPlayer->m_Level.m_Level = 1;
	m_pPlayer->m_Level.m_Exp = 0;
}

void CAccountDatabase::Delete()
{
	char aUsername[32];
	char aBuf[128];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		DatabaseStringCopy(aUsername, m_pPlayer->m_AccData.m_aUsername, sizeof(aUsername));

		char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "DELETE FROM accounts WHERE username='%s'", aUsername);
		CreateNewQuery(aQuery, NULL, NULL, false, true, THREADING);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account deleted!");

		Reset();
	}
	else
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please, login to delete your account");
}

void CAccountDatabase::NewPassword(const char *pNewPassword)
{
	char aPassword[32];
	char aBuf[128];
	if (!m_pPlayer->m_AccData.m_UserID)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please, login to change the password");
		return;
	}
	if (str_length(pNewPassword) > 15 || !str_length(pNewPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Password too %s!", str_length(pNewPassword) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}

	DatabaseStringCopy(aPassword, pNewPassword, sizeof(aPassword));

	str_copy(m_pPlayer->m_AccData.m_aPassword, aPassword, 32);
	Apply();


	dbg_msg("account", "Password changed - ('%s')", m_pPlayer->m_AccData.m_aUsername);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Password successfully changed!");
}

void CAccountDatabase::ReloadDataResult(bool Failed, void *pResultData, void *pData)
{
	char aBuf[125];
	CResultDataReload *pResult = (CResultDataReload *) pData;
	CAccountDatabase *pAccount = pResult->m_pAccount;

	if(Failed == false && pResultData != NULL)
	{
#if defined(CONF_SQL)
		sql::ResultSet *pResults = (sql::ResultSet *)pResultData;
		sql::ResultSetMetaData *pResultsMeta = pResults->getMetaData();
		if(pResults->isLast() == true)
		{
			dbg_msg("account", "Name not found for reloading");
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin. (3)");
			return;
		}

		//fill account
		pResultsMeta = pResults->getMetaData();
		int ColumnCount = pResultsMeta->getColumnCount();

		pResults->last();

		pAccount->m_pPlayer->m_QuestData.m_Pages = str_toint(pResults->getString(1).c_str());

#endif

		pAccount->m_pPlayer->m_AccData.m_UserID = 1;//for preventing buggs

		if(pResult->Func != NULL)
			pResult->Func(Failed, pResultData, pResult->m_pData);
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin. (4)");
	}
}

void CAccountDatabase::ReloadUpdatedData(SqlResultFunction Func, void *pData)
{
	char aUsername[32];

	if (m_pPlayer->m_AccData.m_UserID == 0)
		return;

	DatabaseStringCopy(aUsername, m_pPlayer->m_AccData.m_aUsername, sizeof(aUsername));

	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT pages FROM accounts WHERE username='%s'", aUsername);
	CResultDataReload *pResult = new CResultDataReload();
	pResult->m_pAccount = this;
	pResult->m_pData = pData;
	pResult->Func = Func;
	CreateNewQuery(aQuery, ReloadDataResult, pResult, true, true, THREADING);
}
