/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

// TODO: most of this includes can probably be removed
#include <string.h>
#include <fstream>
#include <engine/config.h> 
#include "account.h"
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

#include <base/system.h>
#include <engine/storage.h>
#include <engine/config.h>
#include <game/server/player.h>
//#include <game/server/gamecontext.h>
#include "account.h"


CAccount::CAccount(CPlayer *pPlayer, CGameContext *pGameServer)
		: m_pPlayer(pPlayer),
		  m_pGameServer(pGameServer)
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

void CAccount::Login(const char *pUsername, const char *pPassword)
{
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
	else if (!Exists(pUsername))
	{
		dbg_msg("account", "Account login failed ('%s' - Missing)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "This account does not exist.");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please register first. (/register <user> <pass>)");
		return;
	}

	char aFullPath[512];
	str_format(aBuf, sizeof(aBuf), "accounts/+%s.acc", pUsername);
	io_close(Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_SAVE, aFullPath, sizeof(aFullPath)));

	char AccUsername[32];
	char AccPassword[32];
	char AccRcon[32];
	int AccID;



	FILE *Accfile;
	Accfile = fopen(aFullPath, "r");
	fscanf(Accfile, "%s\n%s\n%s\n%d", AccUsername, AccPassword, AccRcon, &AccID);
	fclose(Accfile);



	for (int i = 0; i < MAX_SERVER; i++)
	{
		for (int j = 0; j < MAX_CLIENTS; j++)
		{
			if (GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->m_AccData.m_UserID == AccID)
			{
				dbg_msg("account", "Account login failed ('%s' - already in use (local))", pUsername);
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use");
				return;
			}

			if (!GameServer()->m_aaExtIDs[i][j])
				continue;

			if (AccID == GameServer()->m_aaExtIDs[i][j])
			{
				dbg_msg("account", "Account login failed ('%s' - already in use (extern))", pUsername);
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use");
				return;
			}
		}
	}

	if (str_comp(pUsername, AccUsername))
	{
		dbg_msg("account", "Account login failed ('%s' - Wrong username)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Wrong username or password");
		return;
	}

	if (str_comp(pPassword, AccPassword))
	{
		dbg_msg("account", "Account login failed ('%s' - Wrong password)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Wrong username or password");
		return;
	}


	Accfile = fopen(aFullPath, "r");

	fscanf(Accfile, "%s\n%s\n%s\n%d\n%d\n%d",
		m_pPlayer->m_AccData.m_Username, // Done
		m_pPlayer->m_AccData.m_Password, // Done
		m_pPlayer->m_AccData.m_RconPassword,
		&m_pPlayer->m_AccData.m_UserID,
		&m_pPlayer->m_AccData.m_Vip,
		&m_pPlayer->m_QuestData.m_Pages); // Done

	fclose(Accfile);

	CCharacter *pOwner = GameServer()->GetPlayerChar(m_pPlayer->GetCID());

	if (pOwner)
	{
		if (pOwner->IsAlive())
			pOwner->Die(m_pPlayer->GetCID(), WEAPON_GAME);
	}

	if (m_pPlayer->GetTeam() == TEAM_SPECTATORS)
		m_pPlayer->SetTeam(TEAM_RED);

	dbg_msg("account", "Account login sucessful ('%s')", pUsername);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Login successful");

	if (pOwner)
	{
		m_pPlayer->m_DeathNote = true;
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You have reveived a Deathnote.");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Write /Deathnoteinfo for more information");
	}
}

void CAccount::Register(const char *pUsername, const char *pPassword)
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
	else if (Exists(pUsername))
	{
		dbg_msg("account", "Account registration failed ('%s' - Already exists)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already exists.");
		return;
	}

#if defined(CONF_FAMILY_UNIX)
	char Filter[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_";
	// "äöü<>|!§$%&/()=?`´*'#+~«»¢“”æßðđŋħjĸł˝;,·^°@ł€¶ŧ←↓→øþ\\";
	if (!strpbrk(pUsername, Filter))
#elif defined(CONF_FAMILY_WINDOWS)
	static TCHAR * ValidChars = _T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_");
	if (_tcsspnp(pUsername, ValidChars))
#else
#error not implemented
#endif
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Don't use invalid chars for username!");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "A - Z, a - z, 0 - 9, . - _");
		return;
	}

	str_format(aBuf, sizeof(aBuf), "accounts/+%s.acc", pUsername);

	IOHANDLE Accfile = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!Accfile)
	{
		dbg_msg("account/error", "Register: failed to open '%s' for writing");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
		return;
	}

	str_format(aBuf, sizeof(aBuf), "%s\n%s\n%s\n%d\n%d\n%d",
		pUsername,
		pPassword,
		"0",
		NextID(),
		m_pPlayer->m_AccData.m_Vip,
		m_pPlayer->m_QuestData.m_Pages
	);

	io_write(Accfile, aBuf, (unsigned int)str_length(aBuf));
	io_close(Accfile);

	dbg_msg("account", "Registration successful ('%s')", pUsername);
	str_format(aBuf, sizeof(aBuf), "Registration successful - ('/login %s %s'): ", pUsername, pPassword);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
	Login(pUsername, pPassword);
}
bool CAccount::Exists(const char *Username)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "accounts/+%s.acc", Username);
	IOHANDLE Accfile = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_SAVE);
	if (Accfile)
	{
		io_close(Accfile);
		return true;
	}
	return false;
}

void CAccount::Apply()
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "accounts/+%s.acc", m_pPlayer->m_AccData.m_Username);
	IOHANDLE Accfile = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!Accfile)
	{
		dbg_msg("account/error", "Apply: failed to open '%s' for writing", aBuf);
		return;
	}

	str_format(aBuf, sizeof(aBuf), "%s\n%s\n%s\n%d\n%d\n%d",
		m_pPlayer->m_AccData.m_Username,
		m_pPlayer->m_AccData.m_Password,
		m_pPlayer->m_AccData.m_RconPassword,
		m_pPlayer->m_AccData.m_UserID,
		m_pPlayer->m_AccData.m_Vip,
		m_pPlayer->m_QuestData.m_Pages);

	io_write(Accfile, aBuf, (unsigned int)str_length(aBuf));
	io_close(Accfile);
}

void CAccount::Reset()
{
	mem_zero(m_pPlayer->m_AccData.m_Username, sizeof(m_pPlayer->m_AccData.m_Username));
	mem_zero(m_pPlayer->m_AccData.m_Password, sizeof(m_pPlayer->m_AccData.m_Password));
	mem_zero(m_pPlayer->m_AccData.m_RconPassword, sizeof(m_pPlayer->m_AccData.m_RconPassword));
	m_pPlayer->m_AccData.m_UserID = 0;
	m_pPlayer->m_AccData.m_Vip = 0;
}

void CAccount::Delete()
{
	char aBuf[128];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		Reset();
		str_format(aBuf, sizeof(aBuf), "accounts/+%s.acc", m_pPlayer->m_AccData.m_Username);
		if(Storage()->RemoveFile(aBuf, IStorage::TYPE_SAVE))
			dbg_msg("account", "Account deleted ('%s')", m_pPlayer->m_AccData.m_Username);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account deleted!");
	}
	else
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please, login to delete your account");
}

void CAccount::NewPassword(const char *pNewPassword)
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

	str_copy(m_pPlayer->m_AccData.m_Password, pNewPassword, 32);
	Apply();


	dbg_msg("account", "Password changed - ('%s')", m_pPlayer->m_AccData.m_Username);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Password successfully changed!");
}


int CAccount::NextID()
{
	int UserID = 1;
	char aAccUserID[128];

	str_copy(aAccUserID, "accounts/++UserIDs++.acc", sizeof(aAccUserID));

	// read the current ID
	IOHANDLE Accfile = Storage()->OpenFile(aAccUserID, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(Accfile)
	{
		char aBuf[32];
		mem_zero(aBuf, sizeof(aBuf));
		io_read(Accfile, aBuf, sizeof(aBuf));
		io_close(Accfile);
		UserID = str_toint(aBuf);
	}

	// write the next ID
	Accfile = Storage()->OpenFile(aAccUserID, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(Accfile)
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", ++UserID);
		io_write(Accfile, aBuf, (unsigned int)str_length(aBuf));
		io_close(Accfile);
	}
	else
		dbg_msg("account/error", "NextID: failed to open '%s' for writing", aAccUserID);


	return UserID + 1;

	return 1;
}

class IStorage *CAccount::Storage() const
{
	return m_pGameServer->Storage();
}

