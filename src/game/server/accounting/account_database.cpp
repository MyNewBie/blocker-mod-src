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
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#endif

#include <base/system.h>
#include <engine/storage.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <game/server/player.h>
//#include <game/server/gamecontext.h>

#include "account.h"
#include "account_database.h"

#define QUERY_MAX_LEN 512

LOCK QueryLock;

struct CResultData
{
	char m_aUsername[32];
	char m_aPassword[32];
	CAccountDatabase *m_pAccount;
};

struct CResultDataReload
{
	void *m_pData;
	CAccountDatabase *m_pAccount;
	SqlResultFunction Func;
};

struct CThreadFeed
{
	SqlResultFunction m_ResultCallback;
	void *m_pResultData;
	char m_aCommand[QUERY_MAX_LEN];
	bool m_SetSchema;
	bool m_ExpectResult;
};

static void QueryThreadFunction(void *pData)
{
	CThreadFeed *pFeed = (CThreadFeed *)pData;

#if defined(CONF_SQL)
	sql::Driver *pDriver = NULL;
	sql::Connection *pConnection = NULL;
	sql::Statement *pStatement = NULL;
	sql::ResultSet *pResults = NULL;

	lock_wait(QueryLock);

	try
	{
		sql::ConnectOptionsMap connection_properties;
		connection_properties["hostName"]      = sql::SQLString(g_Config.m_SvAccSqlIp);
		connection_properties["port"]          = 3306;
		connection_properties["userName"]      = sql::SQLString(g_Config.m_SvAccSqlName);
		connection_properties["password"]      = sql::SQLString(g_Config.m_SvAccSqlPassword);
		connection_properties["OPT_CONNECT_TIMEOUT"] = 10;
		connection_properties["OPT_READ_TIMEOUT"] = 10;
		connection_properties["OPT_WRITE_TIMEOUT"] = 20;
		connection_properties["OPT_RECONNECT"] = true;

		pDriver = get_driver_instance();
		pConnection = pDriver->connect(g_Config.m_SvAccSqlIp, g_Config.m_SvAccSqlName, g_Config.m_SvAccSqlPassword);
		if(pFeed->m_SetSchema == true)
			pConnection->setSchema(g_Config.m_SvAccSqlDatabase);

		pStatement = pConnection->createStatement();
		if(pFeed->m_SetSchema == true && pFeed->m_ExpectResult)
			pResults = pStatement->executeQuery(pFeed->m_aCommand);
		else
			pStatement->execute(pFeed->m_aCommand);

		if(pFeed->m_ResultCallback != NULL)//no error
			pFeed->m_ResultCallback(false, pResults, pFeed->m_pResultData);
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("sql", "ERROR: %s", e.what());
		log_file(e.what(), "Queries.log", g_Config.m_SvSecurityPath);
		if(pFeed->m_ResultCallback != NULL)
			pFeed->m_ResultCallback(true, NULL, pFeed->m_pResultData);
	}

	if (pResults)
			delete pResults;
	if (pConnection)
		delete pConnection;

	lock_unlock(QueryLock);

#endif

}


CAccountDatabase::CAccountDatabase(CPlayer *pPlayer)
		: CAccount(pPlayer)
{
}

/*
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#ifndef NON_HASED_VERSION
#include "generated/nethash.cpp"
#define GAME_VERSION "0.6.1"
#define GAME_NETVERSION "0.6 626fce9a778df4d4" //the std game version
#endif
#endif
*/

/*
	-----PLEASE READ------ 
	Keep in mind with slots they only get set to 0 on player disconnections
	So please remember when updating server do not force close it, please use the shutdown cmd
	So we can insure that all logged in users slots get set to 0,
	If we just force close the server nothing gets saved and their slots remain as 1
	And they cannot get back into their accounts and we will have many scrubs in our Dms requesting
	access back into their accounts :), Please keep this in Mind :)

	Or if you like work :) Like Captain teemo who hardly does shit for the Mod, you can give yourself work
	and force shut and have fun checking every users slot, and setting it to 0 manually <3 Great work team!
*/

bool CAccountDatabase::PreventInjection(const char *pSrc)
{
	return str_find(pSrc, "'") != NULL || str_find(pSrc, ";") != NULL;
};

void CAccountDatabase::CreateNewQuery(char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema)
{
	CThreadFeed *pFeed = new CThreadFeed();
	pFeed->m_ResultCallback = ResultCallback;
	pFeed->m_pResultData = pData;
	str_copy(pFeed->m_aCommand, pQuery, sizeof(pFeed->m_aCommand));
	pFeed->m_SetSchema = SetSchema;
	pFeed->m_ExpectResult = ExpectResult;

	thread_init(QueryThreadFunction, pFeed);
	//QueryThreadFunction(pFeed);

	//dbg_msg("QUERY", pQuery);
	log_file(pQuery, "Queries.log", g_Config.m_SvSecurityPath);
}

void CAccountDatabase::InitTables()
{
	QueryLock = lock_create();

	//Init schema
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", g_Config.m_SvAccSqlDatabase);
	CreateNewQuery(aBuf, NULL, NULL, false, false);

	CreateNewQuery("CREATE TABLE IF NOT EXISTS accounts (username VARCHAR(32) BINARY NOT NULL, password VARCHAR(32) BINARY NOT NULL, vip INT DEFAULT 0, pages INT DEFAULT 0, level INT DEFAULT 1, exp INT DEFAULT 0, ip VARCHAR(47), weaponkits INT DEFAULT 0, slot INT DEFAULT 0,  PRIMARY KEY (username)) CHARACTER SET utf8 ;", NULL, NULL, false);
}

void CAccountDatabase::InsertAccount(char *pUsername, char *pPassword, int Vip, int Pages, int Level, int Exp, char *pIp, int WeaponKits, int Slot)
{
	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "INSERT INTO accounts VALUES('%s', '%s', %i, %i, %i, %i, '%s', %i, %i)",
		pUsername, pPassword, Vip, Pages, Level, Exp, pIp, WeaponKits, Slot);

	CreateNewQuery(aQuery, NULL, NULL, false);
}

void CAccountDatabase::LoginResult(bool Failed, void *pResultData, void *pData)
{
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	CAccountDatabase *pAccount = pResult->m_pAccount;

	if(Failed == false && pResultData != NULL)
	{
#if defined(CONF_SQL)
		sql::ResultSet *pResults = (sql::ResultSet *)pResultData;
		sql::ResultSetMetaData *pResultsMeta = pResults->getMetaData();
		if(pResults->isLast() == true)
		{
			dbg_msg("account", "Account login failed ('%s' - Missing)", pResult->m_aUsername);
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "This account does not exist.");
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Please register first. (/register <user> <pass>)");
			return;
		}

		//fill account
		pResultsMeta = pResults->getMetaData();
		int ColumnCount = pResultsMeta->getColumnCount();

		pResults->last();

		if(str_comp(pResult->m_aPassword, pResults->getString(2).c_str()) != 0)
		{
			dbg_msg("account", "Account login failed ('%s' - Wrong password)", pResult->m_aUsername);
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Wrong username or password");
			return;
		}

		for(int i = 0; i < ColumnCount; i++)
		{
			sql::SQLString str = pResults->getString(i + 1);

			switch(i)
			{
			case 0: str_copy(pAccount->m_pPlayer->m_AccData.m_aUsername, str.c_str(), sizeof(pAccount->m_pPlayer->m_AccData.m_aUsername)); break;
			case 1: str_copy(pAccount->m_pPlayer->m_AccData.m_aPassword, str.c_str(), sizeof(pAccount->m_pPlayer->m_AccData.m_aPassword)); break;
			case 2: pAccount->m_pPlayer->m_AccData.m_Vip = str_toint(str.c_str()); break;
			case 3: pAccount->m_pPlayer->m_QuestData.m_Pages = str_toint(str.c_str()); break;
			case 4: pAccount->m_pPlayer->m_Level.m_Level = str_toint(str.c_str()); break;
			case 5: pAccount->m_pPlayer->m_Level.m_Exp = str_toint(str.c_str()); break;
			case 6: str_copy(pAccount->m_pPlayer->m_AccData.m_aIp, str.c_str(), sizeof(pAccount->m_pPlayer->m_AccData.m_aIp)); break;
			case 7: pAccount->m_pPlayer->m_AccData.m_Weaponkits = str_toint(str.c_str()); break;
			case 8: pAccount->m_pPlayer->m_AccData.m_Slot = str_toint(str.c_str()); break;
			}
		}

#endif

		pAccount->m_pPlayer->m_AccData.m_UserID = 1;//logged in

		CCharacter *pOwner = pAccount->GameServer()->GetPlayerChar(pAccount->m_pPlayer->GetCID());
		if (pOwner)
		{
			if (pOwner->IsAlive())
				pOwner->Die(pAccount->m_pPlayer->GetCID(), WEAPON_GAME);
		}

		if (pAccount->m_pPlayer->GetTeam() == TEAM_SPECTATORS)
			pAccount->m_pPlayer->SetTeam(TEAM_RED);

		dbg_msg("account", "Account login sucessful ('%s')", pResult->m_aUsername);
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Login successful");

		if (pOwner)
		{
			pAccount->m_pPlayer->m_AccData.m_Slot++;
			pAccount->m_pPlayer->m_DeathNote = true;
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "You have reveived a Deathnote.");
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Write /Deathnoteinfo for more information");
			pAccount->Apply();

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
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
	}
}

void CAccountDatabase::Login(const char *pUsername, const char *pPassword)
{
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
	else if(PreventInjection(pUsername) || PreventInjection(pPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Invalid chars used");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}

	for (int j = 0; j < MAX_CLIENTS; j++)
	{
		if (GameServer()->m_apPlayers[j] && str_comp(GameServer()->m_apPlayers[j]->m_AccData.m_aUsername, pUsername) == 0)
		{
			dbg_msg("account", "Account login failed ('%s' - already in use (local))", pUsername);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use");
			return;
		}
	}


	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM accounts WHERE username='%s'", pUsername);
	CResultData *pResult = new CResultData();
	str_copy(pResult->m_aUsername, pUsername, sizeof(pResult->m_aUsername));
	str_copy(pResult->m_aPassword, pPassword, sizeof(pResult->m_aPassword));
	pResult->m_pAccount = this;
	CreateNewQuery(aQuery, LoginResult, pResult, true);
}

void CAccountDatabase::RegisterResult(bool Failed, void *pResultData, void *pData)
{
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	CAccountDatabase *pAccount = pResult->m_pAccount;

	if(Failed == false)
	{
		dbg_msg("account", "Registration successful ('%s')", pResult->m_aUsername);
		str_format(aBuf, sizeof(aBuf), "Registration successful - ('/login %s %s'): ", pResult->m_aUsername, pResult->m_aPassword);
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), aBuf);
		pAccount->Login(pResult->m_aUsername, pResult->m_aPassword);
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
	}
}

void CAccountDatabase::ExistsResultRegister(bool Failed, void *pResultData, void *pData)
{
	char aBuf[125];
	CResultData *pResult = (CResultData *) pData;
	CAccountDatabase *pAccount = pResult->m_pAccount;

	if(Failed == false && pResultData != 0x0)
	{

#if defined(CONF_SQL)
		sql::ResultSet *pResults = (sql::ResultSet *)pResultData;
		if(pResults->isLast() == false)
		{
			dbg_msg("account", "Account registration failed ('%s' - Already exists)", pResult->m_aUsername);
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Account already exists.");
			return;
		}
#endif

#if defined(CONF_FAMILY_UNIX)
		char Filter[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_";
		// "äöü<>|!§$%&/()=?`´*'#+~«»¢“”æßðđŋħjĸł˝;,·^°@ł€¶ŧ←↓→øþ\\";
		if (!strpbrk(pResult->m_aUsername, Filter))
#elif defined(CONF_FAMILY_WINDOWS)
		static TCHAR * ValidChars = _T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_");
		if (_tcsspnp(pResult->m_aUsername, ValidChars))
#else
#error not implemented
#endif
		{
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Don't use invalid chars for username!");
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "A - Z, a - z, 0 - 9, . - _");
			return;
		}

		char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "INSERT INTO accounts VALUES('%s', '%s', %i, %i, %i, %i, '%s', %i, %i)",
			pResult->m_aUsername, pResult->m_aPassword, pAccount->m_pPlayer->m_AccData.m_Vip, pAccount->m_pPlayer->m_QuestData.m_Pages, pAccount->m_pPlayer->m_Level.m_Level,
			pAccount->m_pPlayer->m_Level.m_Exp, pAccount->m_pPlayer->m_AccData.m_aIp, pAccount->m_pPlayer->m_AccData.m_Weaponkits, pAccount->m_pPlayer->m_AccData.m_Slot);

		pAccount->CreateNewQuery(aQuery, RegisterResult, pResult, false);
	}
	else
	{
		dbg_msg("account", "No Result pointer");
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
	}
}


void CAccountDatabase::Register(const char *pUsername, const char *pPassword)
{
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
	else if(PreventInjection(pUsername) || PreventInjection(pPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Invalid chars used");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}


	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM accounts WHERE username='%s'", pUsername);
	CResultData *pResult = new CResultData();
	str_copy(pResult->m_aUsername, pUsername, sizeof(pResult->m_aUsername));
	str_copy(pResult->m_aPassword, pPassword, sizeof(pResult->m_aPassword));
	pResult->m_pAccount = this;
	CreateNewQuery(aQuery, ExistsResultRegister, pResult, true);
}

void CAccountDatabase::Apply()
{
	if(m_pPlayer->m_AccData.m_UserID == 0)
		return;

	char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "UPDATE  accounts SET username='%s', password='%s', vip=%i, level=%i, exp=%i, ip='%s', weaponkits=%i, slot=%i WHERE username='%s'",
			m_pPlayer->m_AccData.m_aUsername, m_pPlayer->m_AccData.m_aPassword, m_pPlayer->m_AccData.m_Vip, m_pPlayer->m_Level.m_Level,
			m_pPlayer->m_Level.m_Exp, m_pPlayer->m_AccData.m_aIp, m_pPlayer->m_AccData.m_Weaponkits, m_pPlayer->m_AccData.m_Slot, m_pPlayer->m_AccData.m_aUsername);

	CreateNewQuery(aQuery, NULL, NULL, false);
}

void CAccountDatabase::ApplyUpdatedData()
{
	char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "UPDATE  accounts SET pages=%i WHERE username='%s'",
			m_pPlayer->m_QuestData.m_Pages, m_pPlayer->m_AccData.m_aUsername);

	CreateNewQuery(aQuery, NULL, NULL, false);
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
	m_pPlayer->m_Level.m_Level = 0;
	m_pPlayer->m_Level.m_Exp = 0;
}

void CAccountDatabase::Delete()
{
	char aBuf[128];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		char aQuery[QUERY_MAX_LEN];
		str_format(aQuery, sizeof(aQuery), "DELETE FROM accounts WHERE username='%s'", m_pPlayer->m_AccData.m_aUsername);
		CreateNewQuery(aQuery, NULL, NULL, false);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account deleted!");

		Reset();
	}
	else
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please, login to delete your account");
}

void CAccountDatabase::NewPassword(const char *pNewPassword)
{
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
	else if(PreventInjection(pNewPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Invalid chars used");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return;
	}

	str_copy(m_pPlayer->m_AccData.m_aPassword, pNewPassword, 32);
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
			pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
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
		pAccount->GameServer()->SendChatTarget(pAccount->m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
	}
}

void CAccountDatabase::ReloadUpdatedData(SqlResultFunction Func, void *pData)
{
	if (m_pPlayer->m_AccData.m_UserID == 0)
		return;

	char aQuery[QUERY_MAX_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT pages FROM accounts WHERE username='%s'", m_pPlayer->m_AccData.m_aUsername);
	CResultDataReload *pResult = new CResultDataReload();
	pResult->m_pAccount = this;
	pResult->m_pData = pData;
	pResult->Func = Func;
	CreateNewQuery(aQuery, ReloadDataResult, pResult, true);
}
