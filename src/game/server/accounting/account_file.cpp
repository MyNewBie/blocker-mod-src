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
#include <engine/shared/config.h>
#include <game/server/player.h>
//#include <game/server/gamecontext.h>
#include "account_file.h"


CAccountFile::CAccountFile(CPlayer *pPlayer)
		: CAccount(pPlayer)
{
}

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

void CAccountFile::Login(const char *pUsername, const char *pPassword)
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
	else if (!IsCorrectSizeData(pUsername, pPassword))
	{
		return;
	}
	else if (!Exists(pUsername))
	{
		dbg_msg("account", "Account login failed ('%s' - Missing)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "This account does not exist.");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please register first. (/register <user> <pass>)");
		return;
	}

	str_format(aBuf, sizeof(aBuf), "%s/+%s.acc", g_Config.m_SvAccountsPath, pUsername);

	char AccUsername[32];
	char AccPassword[32];
	char AccRcon[32];
	int AccID;

	FILE *Accfile;
	Accfile = fopen(aBuf, "r");
	fscanf(Accfile, "%s\n%s\n%s\n%d", AccUsername, AccPassword, AccRcon, &AccID);
	fclose(Accfile);

	for (int j = 0; j < MAX_CLIENTS; j++)
	{
		if (GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->m_AccData.m_UserID == AccID)
		{
			dbg_msg("account", "Account login failed ('%s' - already in use (local))", pUsername);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use");
			return;
		}
	}

	if (str_comp(pPassword, AccPassword))
	{
		dbg_msg("account", "Account login failed ('%s' - Wrong password)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Wrong username or password");
		return;
	}

	Accfile = fopen(aBuf, "r");

	// Always change the numbers when adding please. Makes it easy 
	fscanf(Accfile, "%s\n%s\n%s\n%d\n%d\n%d\n%d\n%d\n%s\n%d\n%d", // 11
		m_pPlayer->m_AccData.m_aUsername, // Done 1
		m_pPlayer->m_AccData.m_aPassword, // Done 2
		m_pPlayer->m_AccData.m_aRconPassword, // 3
		&m_pPlayer->m_AccData.m_UserID, // 4
		&m_pPlayer->m_AccData.m_Vip, // 5
		&m_pPlayer->m_QuestData.m_Pages, // 6
		&m_pPlayer->m_Level.m_Level, // 7
		&m_pPlayer->m_Level.m_Exp, // 8
		m_pPlayer->m_AccData.m_aIp, // 9
		&m_pPlayer->m_AccData.m_Weaponkits, // 10
		&m_pPlayer->m_AccData.m_Slot // 11
	); // Done

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
		m_pPlayer->m_AccData.m_Slot++;
		m_pPlayer->m_DeathNote = true;
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You have reveived a Deathnote.");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Write /Deathnoteinfo for more information");
		Apply();

		if(pOwner->GetPlayer()->m_AccData.m_Slot > 1)
		{
			dbg_msg("account", "Account login failed ('%s' - already in use (extern))", pUsername);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already in use.");
			m_pPlayer->m_AccData.m_Slot--;
			m_pPlayer->m_pAccount->Apply();
			m_pPlayer->m_pAccount->Reset();
		}
	}
}

void CAccountFile::Register(const char *pUsername, const char *pPassword)
{
	char aBuf[125];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		dbg_msg("account", "Account registration failed ('%s' - Logged in)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Already logged in");
		return;
	}
	if (!IsCorrectSizeData(pUsername, pPassword))
	{
		return;
	}
	else if (Exists(pUsername))
	{
		dbg_msg("account", "Account registration failed ('%s' - Already exists)", pUsername);
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account already exists.");
		return;
	}

	if(!IsValidChar(pUsername))
		return;

	str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_SvAccountsPath);

	if(fs_makedir(aBuf))
        dbg_msg("account.cpp", "Failed to create accounts folder (line %d)", __LINE__);

	str_format(aBuf, sizeof(aBuf), "%s/+%s.acc", g_Config.m_SvAccountsPath, pUsername);

	IOHANDLE Accfile = io_open(aBuf, IOFLAG_WRITE);
	if(!Accfile)
	{
		dbg_msg("account/error", "Register: failed to open '%s' for writing");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Internal Server Error. Please contact an admin.");
		return;
	}

	// Always change the numbers when adding please. Makes it easy 
	str_format(aBuf, sizeof(aBuf), "%s\n%s\n%s\n%d\n%d\n%d\n%d\n%d\n%s\n%d\n%d", // 11
		pUsername, // 1
		pPassword, // 2
		"0", // 3
		NextID(), // 4
		m_pPlayer->m_AccData.m_Vip, // 5
		m_pPlayer->m_QuestData.m_Pages, // 6
		m_pPlayer->m_Level.m_Level, // 7 
		m_pPlayer->m_Level.m_Exp, // 8
		m_pPlayer->m_AccData.m_aIp, // 9
		m_pPlayer->m_AccData.m_Weaponkits, // 10
		m_pPlayer->m_AccData.m_Slot // 11
	);

	io_write(Accfile, aBuf, (unsigned int)str_length(aBuf));
	io_close(Accfile);

	dbg_msg("account", "Registration successful ('%s')", pUsername);
	str_format(aBuf, sizeof(aBuf), "Registration successful - ('/login %s %s'): ", pUsername, pPassword);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
	Login(pUsername, pPassword);
}

bool CAccountFile::IsValidChar(const char *pUsername)
{
#if defined(CONF_FAMILY_UNIX)
	char Filter[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_";
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
		return false;
	}

	return true;
}

bool CAccountFile::IsCorrectSizeData(const char *pUsername, const char *pPassword)
{
	char aBuf[256];

	if (str_length(pUsername) > 15 || !str_length(pUsername))
	{
		str_format(aBuf, sizeof(aBuf), "Username too %s", str_length(pUsername) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return false;
	}
	
	if (str_length(pPassword) > 15 || !str_length(pPassword))
	{
		str_format(aBuf, sizeof(aBuf), "Password too %s!", str_length(pPassword) ? "long" : "short");
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		return false;
	}

	return true;
}

bool CAccountFile::Exists(const char *Username)
{
	char aPath[128];
	str_format(aPath, sizeof(aPath), "%s/+%s.acc",g_Config.m_SvAccountsPath, Username);
	IOHANDLE Accfile = io_open(aPath, IOFLAG_READ);
	
	if (Accfile)
	{
		io_close(Accfile);
		return true;
	}
	return false;
}

void CAccountFile::Apply()
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s/+%s.acc",g_Config.m_SvAccountsPath, m_pPlayer->m_AccData.m_aUsername);
	IOHANDLE Accfile = io_open(aBuf, IOFLAG_WRITE);

	if(!Accfile)
	{
		dbg_msg("account/error", "Apply: failed to open '%s' for writing", aBuf);
		return;
	}

	// Always change the numbers when adding please. Makes it easy 
	str_format(aBuf, sizeof(aBuf), "%s\n%s\n%s\n%d\n%d\n%d\n%d\n%d\n%s\n%d\n%d", // 11
		m_pPlayer->m_AccData.m_aUsername, // 1
		m_pPlayer->m_AccData.m_aPassword, // 2
		m_pPlayer->m_AccData.m_aRconPassword, // 3
		m_pPlayer->m_AccData.m_UserID, // 4
		m_pPlayer->m_AccData.m_Vip, // 5
		m_pPlayer->m_QuestData.m_Pages, // 6
		m_pPlayer->m_Level.m_Level, // 7
		m_pPlayer->m_Level.m_Exp, // 8
		m_pPlayer->m_AccData.m_aIp, // 9
		m_pPlayer->m_AccData.m_Weaponkits, // 10
		m_pPlayer->m_AccData.m_Slot); // 11

	io_write(Accfile, aBuf, (unsigned int)str_length(aBuf));
	io_close(Accfile);
}

void CAccountFile::Reset()
{
	mem_zero(m_pPlayer->m_AccData.m_aUsername, sizeof(m_pPlayer->m_AccData.m_aUsername));
	mem_zero(m_pPlayer->m_AccData.m_aPassword, sizeof(m_pPlayer->m_AccData.m_aPassword));
	mem_zero(m_pPlayer->m_AccData.m_aRconPassword, sizeof(m_pPlayer->m_AccData.m_aRconPassword));
	mem_zero(m_pPlayer->m_AccData.m_aIp, sizeof(m_pPlayer->m_AccData.m_aIp));
	m_pPlayer->m_AccData.m_UserID = 0;
	m_pPlayer->m_AccData.m_Vip = 0;
	m_pPlayer->m_QuestData.m_Pages = 0;
}

void CAccountFile::Delete()
{
	char aBuf[128];
	if (m_pPlayer->m_AccData.m_UserID)
	{
		Reset();
		str_format(aBuf, sizeof(aBuf), "%s/+%s.acc",g_Config.m_SvAccountsPath, m_pPlayer->m_AccData.m_aUsername);

		if(remove(aBuf))
		{
			dbg_msg("account", "Account deleted ('%s')", m_pPlayer->m_AccData.m_aUsername);
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Account deleted!");
		}
	}
	else
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Please, login to delete your account");
	}
}

void CAccountFile::NewPassword(const char *pNewPassword)
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

	str_copy(m_pPlayer->m_AccData.m_aPassword, pNewPassword, 32);
	Apply();


	dbg_msg("account", "Password changed - ('%s')", m_pPlayer->m_AccData.m_aUsername);
	GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Password successfully changed!");
}


int CAccountFile::NextID()
{
	int UserID = 1;
	char aAccUserID[128];

	str_format(aAccUserID, sizeof(aAccUserID), "%s/++UserIDs++.acc", g_Config.m_SvAccountsPath);

	// read the current ID
	IOHANDLE Accfile = io_open(aAccUserID, IOFLAG_READ);
	if(Accfile)
	{
		char aBuf[32];
		mem_zero(aBuf, sizeof(aBuf));
		io_read(Accfile, aBuf, sizeof(aBuf));
		io_close(Accfile);
		UserID = str_toint(aBuf);
	}

	// write the next ID
	Accfile = io_open(aAccUserID, IOFLAG_WRITE);
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