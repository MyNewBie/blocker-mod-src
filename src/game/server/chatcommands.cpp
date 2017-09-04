#include "gamecontext.h"
#include "entities/character.h"
#include "teams.h"
#include "player.h"
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/accounting/account_database.h>

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>

struct CDeathnoteUpdateData
{
	CGameContext *m_pGameServer;
	CPlayer *m_pPlayer;
	int m_id;
};

struct CGivePagesUpdateData
{
	CGameContext *m_pGameServer;
	CPlayer *m_pPlayer;
	int m_id;
	int m_Amount;
	char m_aReason[256];
};

void CGameContext::DeathnoteUpdate(bool Failed, void *pResultData, void *pData)
{
	CDeathnoteUpdateData *pUpdateData = (CDeathnoteUpdateData *) pData;
	CGameContext *pGameServer = pUpdateData->m_pGameServer;
	CPlayer *pPlayer = pUpdateData->m_pPlayer;
	int id = pUpdateData->m_id;
	int ClientID = pPlayer->GetCID();

	if (pPlayer->m_QuestData.m_Pages > 0)
    {
		pGameServer->m_apPlayers[id]->KillCharacter(WEAPON_WORLD);

        char aBuf[128];
        str_format(aBuf, sizeof(aBuf), "%s used a Deathnote to kill you!", pGameServer->Server()->ClientName(ClientID));
        pGameServer->SendChatTarget(id, aBuf);
        str_format(aBuf, sizeof(aBuf), "Successfully killed %s, Pages: %d", pGameServer->Server()->ClientName(id), pPlayer->m_QuestData.m_Pages);
        pGameServer->SendChatTarget(ClientID, aBuf);
        pPlayer->m_QuestData.m_Pages--;
        pPlayer->m_LastDeathnote = pGameServer->Server()->Tick();

		CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(pPlayer->m_pAccount);
		if(pAccDb)
			pAccDb->ApplyUpdatedData();
    }
    else
    {
        if (!pPlayer->m_QuestData.QuestActive())
            pGameServer->SendChatTarget(ClientID, "You don't have any pages, type /beginquest to start your quests to get some pages.");
        else
            pGameServer->SendChatTarget(ClientID, "You don't have any pages, complete your quests to get some pages.");
    }
}

void CGameContext::GivePagesUpdate(bool Failed, void *pResultData, void *pData)
{
	CGivePagesUpdateData *pUpdateData = (CGivePagesUpdateData *)pData;
	CGameContext *pGameServer = pUpdateData->m_pGameServer;
	CPlayer *pPlayer = pUpdateData->m_pPlayer;
	int id = pUpdateData->m_id;
	int Amount = pUpdateData->m_Amount;
	int ClientID = pPlayer->GetCID();

	char LogMsg[230];
	char Info[100];
	pGameServer->m_apPlayers[id]->m_QuestData.m_Pages += Amount;

	CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(pGameServer->m_apPlayers[id]->m_pAccount);
	if (pAccDb)
		pAccDb->ApplyUpdatedData();

	str_format(LogMsg, sizeof(LogMsg), "%s gave %d pages to %s", pGameServer->Server()->ClientName(ClientID), Amount, pGameServer->Server()->ClientName(id));
	str_format(Info, 100, "You have received %d pages from %s", Amount, pGameServer->Server()->ClientName(ClientID));

	pGameServer->SendChatTarget(id, Info);
	pGameServer->SendChatTarget(ClientID, LogMsg);
	log_file(LogMsg, "AdminPagesLogs.log", g_Config.m_SvSecurityPath);
}

void CGameContext::ChatCommands(const char *pMsg, int ClientID)
{
    CPlayer *pPlayer = m_apPlayers[ClientID];
    CCharacter *pChar = GetPlayerChar(ClientID);

    bool IsAdmin = Server()->IsAdmin(ClientID);
    bool IsMod = Server()->IsMod(ClientID);
    bool IsAuthed = Server()->IsAuthed(ClientID);

    if (!str_comp_num(pMsg + 1, "login", 5))
    {
        char Username[512];
        char Password[512];
        if (sscanf(pMsg, "/login %s %s", Username, Password) != 2)
        {
            SendChatTarget(ClientID, "Please, use '/login <username> <password>'");
            return;
        }
        pPlayer->m_pAccount->Login(Username, Password);
        return;
    }
    else if (!str_comp_nocase(pMsg + 1, "logout"))
    {
        if (!pPlayer->m_AccData.m_UserID)
        {
            SendChatTarget(ClientID, "Not logged in");
            return;
        }
        pPlayer->m_AccData.m_Slot--;
        pPlayer->m_pAccount->Apply();
        pPlayer->m_pAccount->Reset();

        SendChatTarget(ClientID, "Logout successful");

        CCharacter *pOwner = pChar;

        if (pOwner)
        {
            if (pOwner->IsAlive())
                pOwner->Die(ClientID, WEAPON_GAME);
        }

        return;
    }
    else if (!str_comp_num(pMsg + 1, "register", 8))
    {
        char aUsername[512];
        char aPassword[512];
        if (sscanf(pMsg, "/register %s %s", aUsername, aPassword) != 2)
        {
            SendChatTarget(ClientID, "Please, use '/register <username> <password>'");
            return;
        }
        pPlayer->m_pAccount->Register(aUsername, aPassword);
        return;
    }
    else if (!str_comp_num(pMsg + 1, "password", 8))
    {
        char aNewPassword[512];
        if (sscanf(pMsg, "/password %s", aNewPassword) != 1)
        {
            SendChatTarget(ClientID, "Please use '/password <password>'");
            return;
        }
        pPlayer->m_pAccount->NewPassword(aNewPassword);
        return;
    }
    /*
     *   Incase a player player wants to kick his dummy he cant disconnect
     *   or his old character that Timed out
    */
    else if (!str_comp_num(pMsg + 1, "dropconnections", 15))
    {
        char aAddrStr[NETADDR_MAXSTRSIZE] = { 0 };
        Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (!GetPlayerChar(i) || i == ClientID)
                continue;

            char aAddrStr2[NETADDR_MAXSTRSIZE] = { 0 };
            Server()->GetClientAddr(i, aAddrStr2, sizeof(aAddrStr2));

            if (!str_comp_nocase(aAddrStr, aAddrStr2) == 0)
                continue;
            else if (str_comp_nocase(aAddrStr, aAddrStr2) == 0)
            {
                if (m_apPlayers[i]->m_AccData.m_UserID)
                {
                    m_apPlayers[i]->m_AccData.m_Slot--;
                    m_apPlayers[i]->m_pAccount->Apply();
                    m_apPlayers[i]->m_pAccount->Reset();
                }

                Server()->Kick(i, "Requested Drop");
            }
        }
    }
    else if (str_comp_nocase_num(pMsg + 1, "Deathnote ", 10) == 0)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        if (pPlayer->m_LastDeathnote + g_Config.m_SvDeathNoteCoolDown * Server()->TickSpeed() > Server()->Tick())
        {
            char aBuf[256];
            str_format(aBuf, sizeof(aBuf), "You have to wait %d seconds until you can write down more players in your deathnote", (pPlayer->m_LastDeathnote + g_Config.m_SvDeathNoteCoolDown * Server()->TickSpeed() - Server()->Tick()) / Server()->TickSpeed());
            SendChatTarget(ClientID, aBuf);
            return;
        }

        if (m_KOHActive || m_LMB.State() == m_LMB.STATE_RUNNING || m_LMB.State() == m_LMB.STATE_REGISTRATION)
        {
            SendChatTarget(ClientID, "You cannot use deathnotes right now");
            return;
        }

		char aName[256];
        str_copy(aName, pMsg + 11, sizeof(aName));
        
        int id = ConvertNameToID(aName);

        if (id < 0 || !GetPlayerChar(id) || !GetPlayerChar(id)->IsAlive()) // Prevent crashbug (fix)
            return;

		CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(pPlayer->m_pAccount);

		CDeathnoteUpdateData *pResultData = new CDeathnoteUpdateData;
		pResultData->m_pGameServer = this;
		pResultData->m_pPlayer = pPlayer;
		pResultData->m_id = id;

		if(pAccDb != NULL)
			pAccDb->ReloadUpdatedData(DeathnoteUpdate, pResultData);
		else
			DeathnoteUpdate(false, NULL, pResultData);

    }
    else if (str_comp_nocase_num(pMsg + 1, "Deathnoteinfo", 13) == 0)
    {
        if (!m_apPlayers[ClientID]) // again character check useless, you can even check it by simply put player check
            return;

        SendChatTarget(ClientID, "You have received a Deathnote, but in order to kill players you must gather pages.");
        SendChatTarget(ClientID, "With a Deathnote you can write /deathnote Playername (Ex: /deathnote namelesstee) to kill any specific player!");
        SendChatTarget(ClientID, "You can type /pages to check your current amount of pages.");
        SendChatTarget(ClientID, "To obtain pages you must complete quests type /beginquest to start the quest. - Good luck!");
        SendChatTarget(ClientID, "For further information please go watch the anime - DeathNote :)");
    }
    // PAGES CHECK
    else if (str_comp_nocase_num(pMsg + 1, "pages", 5) == 0)
    {
        if (!m_apPlayers[ClientID])
            return;

        if (!pPlayer->m_DeathNote)
        {
            SendChatTarget(ClientID, "0 pages, You dont even have a Deathnote!");
            return;
        } 

        int Pages = pPlayer->m_QuestData.m_Pages;
        char Message[104];
        str_format(Message, 104, "You have %d pages in your Deathnote!", Pages);
        SendChatTarget(ClientID, Message);
    }
    else if (str_comp_nocase_num(pMsg + 1, "beginquest", 10) == 0)
    {
        if (m_PlayerCount < g_Config.m_SvQuestCount)
        {
            char aBuf[128];
            str_format(aBuf, sizeof(aBuf), "There need to be at least %i players on the server to start a quest", g_Config.m_SvQuestCount);
            SendChatTarget(ClientID, aBuf);
            return;
        }

        if (pPlayer->m_QuestData.QuestActive())
        {
            SendChatTarget(ClientID, "You are in a quest already, type /questinfo to show your objective");
            return;
        }

        if (!pPlayer->m_AccData.m_UserID)
        {
            SendChatTarget(ClientID, "Please login first");
            return;
        }

        pPlayer->QuestReset();
        pPlayer->QuestSetNextPart();
        SendChatTarget(ClientID, "You can stop the quest whenever you want by typing /stopquest (WARNING: Quest progress will reset)");
    }
    else if (str_comp_nocase_num(pMsg + 1, "stopquest", 9) == 0)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pPlayer->QuestReset();
        if (pPlayer->m_QuestData.QuestActive())
            SendChatTarget(ClientID, "Quest has been stopped and your progress has been reset!");
        else
            SendChatTarget(ClientID, "You are not currently in a quest, type /beginquest to start one!");
    }
    else if (str_comp_nocase_num(pMsg + 1, "questinfo", 9) == 0)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pPlayer->QuestTellObjective();
    }
    else if (str_comp_nocase_num(pMsg + 1, "vipinfo", 7) == 0 || str_comp_nocase_num(pMsg + 1, "vip info", 8) == 0)
    {
        SendChatTarget(ClientID, "===== VIP FEATURES =====");
        SendChatTarget(ClientID, "- VIP-Room (epic circle, rainbow, endless, weapons, bloody)");
        SendChatTarget(ClientID, "- PassiveMode (Anti-wayblock) by using /passive");
        SendChatTarget(ClientID, "- Able to use /weapons at any time (Non-Active-Tournaments)");
        SendChatTarget(ClientID, "- Able to use /rainbow (Epiletic)");
        SendChatTarget(ClientID, "- Able to use /circle");
        SendChatTarget(ClientID, "- Able to use /lovely");
        SendChatTarget(ClientID, "- Able to use /heartguns");
        SendChatTarget(ClientID, "- Able to use /ball");
        SendChatTarget(ClientID, "- Able to use /rainbowhook");
        SendChatTarget(ClientID, "- Able to use /getclientid");
        SendChatTarget(ClientID, "====================");
    }
    else if (str_comp_nocase_num(pMsg + 1, "lovely", 7) == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin)) 
    {
        /*if (!pChar || !pChar->IsAlive())
            return;*/
        pPlayer->m_Lovely ^= true;
        SendChatTarget(ClientID, pPlayer->m_Lovely ? "Lovely activated" : "Lovely deactivated");
    }
    else if (str_comp_nocase(pMsg + 1, "ball") == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin))
    {
        pPlayer->m_IsBallSpawned ^= true;
        SendChatTarget(ClientID, pPlayer->m_IsBallSpawned ? "Ball spawned" : "Ball removed");
                    
        if (pPlayer->m_IsBallSpawned && pChar)
            pPlayer->m_pBall = new CBall(&m_World, pChar->m_Pos, ClientID);
        else if (!pPlayer->m_IsBallSpawned && pChar)
            pPlayer->m_pBall->Reset();
    }   
    else if (str_comp_nocase(pMsg + 1, "heartguns") == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin)) 
    {   
        pPlayer->m_HeartGuns ^= true;
        SendChatTarget(ClientID, pPlayer->m_HeartGuns ? "Heart guns activated" : "Heart guns deactivated");
    }
    else if (str_comp_nocase(pMsg + 1, "passive") == 0) 
    {   
        pPlayer->m_Passive ^= true;

        if(pPlayer->m_AccData.m_Vip || IsAdmin || pPlayer->Temporary.m_PassiveMode)
            SendChatTarget(ClientID, pPlayer->m_Passive ? "Passive mode activated" : "Passive mode deactivated");
        else
            SendChatTarget(ClientID, pPlayer->m_Passive ? "Passive mode activated! But you do not have access yet to passive.(Win it with events!)" : "Passive mode deactivated");
    }   
    else if (str_comp_nocase(pMsg + 1, "weapons") == 0 && (pPlayer->m_AccData.m_Vip || (pPlayer->m_AccData.m_UserID && pPlayer->m_AccData.m_Weaponkits > 0) || IsAdmin))
    {
        if (!pChar || !pChar->IsAlive())
            return; // Tested and found a crashbug -- heres the fix 

        if (m_LMB.State() == m_LMB.STATE_RUNNING)
        {
            SendChatTarget(ClientID, "You cannot use weapons while in LMB");
            return;
        }
        if (pPlayer->m_AccData.m_Weaponkits > 0)
        {
            pPlayer->m_AccData.m_Weaponkits--;
            char aRemaining[64];
            str_format(aRemaining, sizeof(aRemaining), "Remaining kits: %d", pPlayer->m_AccData.m_Weaponkits);
            SendChatTarget(ClientID, aRemaining);
        }
        pChar->GiveAllWeapons();
        SendChatTarget(ClientID, "You received all weapons!");
    }
    else if (str_comp_nocase_num(pMsg + 1, "getclientid ", 12) == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin))
    {
        char Name[256];
        str_copy(Name, pMsg + 13, 256);
                    
        int id = ConvertNameToID(Name);
        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid id!");
            return;
        }

        char aBuf[246];
        str_format(aBuf, sizeof(aBuf), "[ClientID] [%s]: %d", Server()->ClientName(id), m_apPlayers[id]->m_ClientVersion);
        SendChatTarget(ClientID, aBuf);
    }           
    else if (str_comp_nocase(pMsg + 1, "rainbow") == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin))
    {
        if (!pChar || !pChar->IsAlive())
            return;
        pPlayer->m_Rainbowepiletic ^= 1;
        SendChatTarget(ClientID, pPlayer->m_Rainbowepiletic ? "Rainbow activated" : "Rainbow deactivated");
    }
    else if (str_comp_nocase(pMsg + 1, "circle") == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin))
    {
        pPlayer->m_EpicCircle ^= 1;
        SendChatTarget(ClientID, pPlayer->m_EpicCircle ? "Circle activated" : "Circle deactivated"); 

        if(pPlayer->m_EpicCircle && pChar)
            pPlayer->m_pEpicCircle = new CEpicCircle(&m_World, pChar->m_Pos, ClientID);
        else if (!pPlayer->m_EpicCircle && pChar)
            pPlayer->m_pEpicCircle->Reset();
    }
    else if (str_comp_nocase(pMsg + 1, "rainbowhook") == 0 && (pPlayer->m_AccData.m_Vip || IsAdmin))
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pPlayer->m_RainbowHook ^= 1;
        SendChatTarget(ClientID, pPlayer->m_RainbowHook ? "Rainbow hook activated" : "Rainbow hook deactivated"); 

        if(!pPlayer->m_RainbowHook)
        {
            pChar->HandleRainbowHook(true);
        }
    }
    else if (str_comp_nocase(pMsg + 1, "tele") == 0 && IsAdmin)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pChar->Core()->m_Pos = pChar->MousePos();
        pPlayer->m_LastChat = 0;
    }
    else if(str_comp_nocase(pMsg + 1, "invisible") == 0 && IsAdmin)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        if(pPlayer->m_Invisible)
        {
            CreatePlayerSpawn(pChar->m_Pos, pChar->Teams()->TeamMask(pChar->Team(), -1, ClientID));
        }
        else
        {
            CreateDeath(pChar->m_Pos, ClientID, pChar->Teams()->TeamMask(pChar->Team(), -1, ClientID));
        }

        pPlayer->m_Invisible ^= true;

        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), pPlayer->m_Invisible ? "'%s' has left the game" : "'%s' entered and joined the game", Server()->ClientName(ClientID));
        SendChat(-1, CGameContext::CHAT_ALL, aBuf);

        if(!pPlayer->m_Invisible)
        {
            pChar->HandleCollision(true);
        }
    }
    else if(str_comp_nocase(pMsg + 1, "showwhispers") == 0 && (IsAdmin || IsMod))
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pPlayer->m_ShowWhispers ^= true;
        SendChatTarget(ClientID, pPlayer->m_ShowWhispers ? "You can see whispers" : "You can't see whispers");
    }
    else if(str_comp_nocase(pMsg + 1, "stars") == 0 && IsAuthed)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        pPlayer->m_Stars ^= true;
        SendChatTarget(ClientID, pPlayer->m_Stars ? "Stars activated" : "Stars deactivated");
    }
    else if (str_comp_nocase_num(pMsg + 1, "givepage ", 9) == 0 && IsAuthed)
    {
		char aId[32];
		char aAmount[32];
		str_copy(aId, pMsg + 10, 32);
		str_copy(aAmount, pMsg + 12, 32);
		int id = str_toint(aId);

		if (!m_apPlayers[id]->GetCharacter() || m_apPlayers[id]->m_AccData.m_UserID == 0)
			return;

		CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(m_apPlayers[id]->m_pAccount);

		CGivePagesUpdateData *pResultData = new CGivePagesUpdateData;
		pResultData->m_pGameServer = this;
		pResultData->m_pPlayer = pPlayer;
		pResultData->m_id = id;
		pResultData->m_Amount = str_toint(aAmount);

		if (pAccDb != NULL)
			pAccDb->ReloadUpdatedData(GivePagesUpdate, pResultData);
		else
			GivePagesUpdate(false, NULL, pResultData);
    }
	else if (str_comp_nocase_num(pMsg + 1, "setlvl ", 7) == 0 && IsAuthed)
	{
		char LogMsg[230];
		char Info[100];

		char aId[32];
		char aLevel[32];
		str_copy(aId, pMsg + 8, 32);
		str_copy(aLevel, pMsg + 10, 32);
		int id = str_toint(aId);
		int Level = str_toint(aLevel);

		if (!m_apPlayers[id]->GetCharacter() || m_apPlayers[id]->m_AccData.m_UserID == 0)
			return;

		m_apPlayers[id]->m_Level.m_Level = Level;
		m_apPlayers[id]->m_pAccount->Apply();

		str_format(LogMsg, sizeof(LogMsg), "%s set level to %d to %s", Server()->ClientName(ClientID), Level, Server()->ClientName(id));
		str_format(Info, 100, "Your level has been set to %d from %s", Level, Server()->ClientName(ClientID));

		SendChatTarget(id, Info);
		SendChatTarget(ClientID, LogMsg);
		log_file(LogMsg, "AdminPagesLogs.log", g_Config.m_SvSecurityPath);
	}
    else if (str_comp_nocase_num(pMsg + 1, "vip ", 4) == 0 && IsAuthed)
    {
        char aId[32];
        char aReason[32];
        str_copy(aId, pMsg + 5, 32);
        str_copy(aReason, pMsg + 7, 32);
        int id = str_toint(aId);

        if (!m_apPlayers[id]->GetCharacter() || m_apPlayers[id]->m_AccData.m_UserID == 0)
            return;

        char LogMsg[230];
        m_apPlayers[id]->m_AccData.m_Vip ^= 1;

        if (m_apPlayers[id]->m_AccData.m_Vip)
        {
            SendChatTarget(id, "You are now vip!");
            str_format(LogMsg, sizeof(LogMsg), "%s gave vip to %s - Reason: \"%s\"", Server()->ClientName(ClientID), Server()->ClientName(id), aReason);
        }
        else
        {
            SendChatTarget(id, "You are no longer vip!");
            str_format(LogMsg, sizeof(LogMsg), "%s removed vip from %s - Reason: \"%s\"", Server()->ClientName(ClientID), Server()->ClientName(id), aReason);
        }

        log_file(LogMsg, "AdminVipLogs.log", g_Config.m_SvSecurityPath);
    }
    else if (str_comp_nocase_num(pMsg + 1, "Givetempassive ", 15) == 0 && IsAuthed)
    {
        char ID[256];
        str_copy(ID, pMsg + 16, 256);
        char Time[256];
        str_copy(Time, pMsg + 18, 256);

        int id = str_toint(ID);

        if (!m_apPlayers[id]->GetCharacter())
            return;

        SendChatTarget(ClientID, "Temporary passive mode set!");
        SendChatTarget(id, "Temporary passive mode has been set!");
        m_apPlayers[id]->Temporary.m_PassiveModeTime = Server()->Tick();
        m_apPlayers[id]->Temporary.m_PassiveTimeLength = str_toint(Time);
        m_apPlayers[id]->Temporary.m_PassiveMode = true;

        char LogMsg[123];
        str_format(LogMsg, 123, "%s gave %s temporary access to passive mode for %ds", Server()->ClientName(ClientID), Server()->ClientName(id), str_toint(Time));
        log_file(LogMsg, "AdminTempPassiveMode.log", g_Config.m_SvSecurityPath);
    }
    else if (str_comp_nocase_num(pMsg + 1, "getTOcode ", 10) == 0 && IsAuthed)
    {
        char Name[256];
        str_copy(Name, pMsg + 11, 256);
        int id = ConvertNameToID(Name);

        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid target, you must specify a name!");
            return;
        }

        char aBuf[246];
        str_format(aBuf, sizeof(aBuf), "[Code] [%s]: %d", Server()->ClientName(id), m_apPlayers[id]->m_TimeoutCode);
        SendChatTarget(ClientID, aBuf);
    }
    else if (str_comp_nocase_num(pMsg + 1, "getip ", 6) == 0 && IsAuthed) // Tired of using status
    {
        char Name[256];
        str_copy(Name, pMsg + 7, 256);
        int id = ConvertNameToID(Name);

        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid target, you must specify a name!");
            return;
        }

        char aAddrStr[NETADDR_MAXSTRSIZE] = { 0 };
        Server()->GetClientAddr(id, aAddrStr, sizeof(aAddrStr));
        
        char aBuf[246];
        str_format(aBuf, sizeof(aBuf), "[IP] [%s]: %s", Server()->ClientName(id), aAddrStr);
        SendChatTarget(ClientID, aBuf);
    }
    else if (str_comp_nocase_num(pMsg + 1, "clientban ", 10) == 0 && IsAdmin)
    {
        if(!pPlayer)
            return;

        char aClientID[256];
        str_copy(aClientID, pMsg + 11, sizeof(aClientID));
        int id = str_toint(aClientID);

        if(id < 0)
        {
            SendChatTarget(ClientID, "Invalid client id!");
            return;
        }

        char aMsg[256];
        str_format(aMsg, sizeof(aMsg), "Autobanning all players with clientid %d", id);
        SendChatTarget(ClientID, aMsg);

        char aLogBan[64];
        str_format(aLogBan, sizeof(aLogBan), "%d", id);
        log_file(aLogBan, "ClientsBanlist.txt", g_Config.m_SvSecurityPath);

        char aCmd[100];

        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(!m_apPlayers[i] || m_apPlayers[i]->m_ClientVersion != id)
                continue;

            // save his ip to ban him later
            Server()->GetClientAddr(i, aBanAddr, sizeof(aBanAddr));
            // Now Kick him
            Server()->Kick(i, "");

            str_format(aCmd, sizeof(aCmd), "ban %s 5 %s", aBanAddr, g_Config.m_SvClientbanMessage);
            Console()->ExecuteLine(aCmd);               
        }
    }
    else if (str_comp_nocase_num(pMsg + 1, "find_clientbanlist ", 19) == 0 && IsAdmin)
    {
        char aClientID[256];
        char aFullPath[256];

        str_copy(aClientID, pMsg + 20, 256);
        str_format(aFullPath, sizeof(aFullPath), "%s/ClientsBanlist.txt", g_Config.m_SvSecurityPath);

        std::ifstream theFile(aFullPath);
        std::string   line;
        char*         search = aClientID; // test variable to search in file                      
        unsigned int  curLine = 0;

        while (getline(theFile, line))
        { // I changed this, see below
            curLine++;
            if (line.find(search, 0) != std::string::npos)
            {
                std::cout << "found: " << search << "line: " << curLine << std::endl;
                char aFound[200];
                str_format(aFound, 200, "Found: %s line: %i", search, curLine);
                SendChatTarget(ClientID, aFound);
            }
        }
    }
    else if (str_comp_nocase_num(pMsg + 1, "delete_clientbanlist ", 21) == 0 && IsAdmin)
    {
        char aLine[64];
        char aFullPath[256];
        str_copy(aLine, pMsg + 22, 256);
        str_format(aFullPath, sizeof(aFullPath), "%s/ClientsBanlist.txt", g_Config.m_SvSecurityPath);

        RemoveLine(aFullPath, str_toint(aLine));
        SendChatTarget(ClientID, "Successfully deleted");
    }
    else if (str_comp_nocase_num(pMsg + 1, "Autoban ", 8) == 0 && IsAdmin)
    {
        if (!pChar || !pChar->IsAlive())
            return;

        char aName[256];
        str_copy(aName, pMsg + 9, sizeof(aName));
        int id = ConvertNameToID(aName);

        if(id == -1)
        {
            SendChatTarget(ClientID, "Invalid target, you must specify a name!");
            return;
        }

        char aTimeoutCode[64];
        char aMsg[200];
        str_copy(aTimeoutCode, m_apPlayers[id]->m_TimeoutCode, 64);
        str_format(aMsg, 200, "Autobanning set on %s", Server()->ClientName(id));
        SendChatTarget(ClientID, aMsg);

        // Drop his info to kick list
        char aInfo[200];
        str_format(aInfo, 200, "%s %s", aTimeoutCode, aName);
        log_file(aInfo, "Banlist.txt", g_Config.m_SvSecurityPath);

        // save his ip to ban him later
        Server()->GetClientAddr(id, aBanAddr, sizeof(aBanAddr));
        // Now Kick him
        Server()->Kick(id, "");
        // & ban him
        char aCmd[100];
        str_format(aCmd, sizeof(aCmd), "ban %s 5 %s", aBanAddr, g_Config.m_SvAutobanMessage);
        Console()->ExecuteLine(aCmd);
    }
    else if (str_comp_nocase_num(pMsg + 1, "check_Banlistfor ", 17) == 0 && IsAdmin)
    {
        char aName[256];
        char aFullPath[256];

        str_copy(aName, pMsg + 18, 256);
        str_format(aFullPath, sizeof(aFullPath), "%s/Banlist.txt", g_Config.m_SvSecurityPath);

        std::ifstream theFile(aFullPath);
        std::string   line;
        char*         search = aName; // test variable to search in file                      
        unsigned int  curLine = 0;

        while (getline(theFile, line))
        { // I changed this, see below
            curLine++;
            if (line.find(search, 0) != std::string::npos)
            {
                std::cout << "found: " << search << "line: " << curLine << std::endl;
                char aFound[200];
                str_format(aFound, 200, "Found: %s line: %i", search, curLine);
                SendChatTarget(ClientID, aFound);
            }
        }
    }
    else if (str_comp_nocase_num(pMsg + 1, "Delete_BanlistLine ", 19) == 0 && IsAdmin)
    {
        char aLine[64];
        char aFullPath[256];
        str_copy(aLine, pMsg + 20, 256);
        str_format(aFullPath, sizeof(aFullPath), "%s/Banlist.txt", g_Config.m_SvSecurityPath);

        RemoveLine(aFullPath, str_toint(aLine));
        SendChatTarget(ClientID, "Successfully deleted");
    }
    else if (str_comp_nocase_num(pMsg + 1, "troll ", 6) == 0 && IsAuthed)
    {
        char aName[256];
        str_copy(aName, pMsg + 7, sizeof(aName));
        int id = ConvertNameToID(aName);

        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid id!");
            return;
        }

        char aBuf[128];
        m_apPlayers[id]->m_Troll ^= 1;
        str_format(aBuf, 128, m_apPlayers[id]->m_Troll ? "Now trolling %s" : "Stop trolling %s", Server()->ClientName(id));
        SendChatTarget(ClientID, aBuf);
    }
    else if (str_comp_nocase_num(pMsg + 1, "makedrunk ", 10) == 0 && IsAuthed)
    {
        char aName[256];
        str_copy(aName, pMsg + 11, sizeof(aName));
        int id = ConvertNameToID(aName);

        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid id!");
            return;
        }

        char aBuf[128];
        m_apPlayers[id]->m_Drunk ^= 1;
        
        str_format(aBuf, sizeof(aBuf), "Successfully %s %s drunk", m_apPlayers[id]->m_Drunk ? "made" : "unmade", Server()->ClientName(id));
        SendChatTarget(ClientID, aBuf);
    }
    else if (str_comp_nocase(pMsg + 1, "fixaccounts") == 0 && IsMod)
    {
        Server()->FixAccounts();
        SendChatTarget(ClientID, "Accounts fixed!");
    }
    else if (str_comp_nocase_num(pMsg + 1, "getid ", 6) == 0 && IsAuthed)
    {
        char aName[256];
        str_copy(aName, pMsg + 7, sizeof(aName));
        int id = ConvertNameToID(aName);

        if (id < 0)
        {
            SendChatTarget(ClientID, "Invalid name!");
            return;
        }

        char aMsg[256];
        str_format(aMsg, sizeof(aMsg), "[getid] %s: %d", aName, id);
        SendChatTarget(ClientID, aMsg);
    }
    else if (str_comp_nocase_num(pMsg + 1, "smarthammer", 11) == 0 && IsAuthed)
    {
        if (!pPlayer)
            return;

        pPlayer->m_Bots.m_SmartHammer ^= true;
        SendChatTarget(ClientID, pPlayer->m_Bots.m_SmartHammer ? "Smarthammer enabled" : "Smarthammer disabled");
    }
    else if (str_comp_nocase(pMsg + 1, "admincmd") == 0 && IsAdmin)
    {
        SendChatTarget(ClientID, "======= Admin <3 =======");
        SendChatTarget(ClientID, "- Givepage (id, amount, reason)");
        SendChatTarget(ClientID, "- Givetempassive (id, time, reason)");
        SendChatTarget(ClientID, "- Vip (id, reason)");
        SendChatTarget(ClientID, "- Togglebotmark (name)");
        SendChatTarget(ClientID, "- ToggleColl");
        SendChatTarget(ClientID, "- Makedrunk (name)");
        SendChatTarget(ClientID, "- Troll (name)");
        SendChatTarget(ClientID, "- Getid");
        SendChatTarget(ClientID, "====== /admincmd 2 =====");
    }
    else if (str_comp_nocase(pMsg + 1, "admincmd 2") == 0 && IsAdmin)
    {
        SendChatTarget(ClientID, "======= Admin <3 =======");
        SendChatTarget(ClientID, "- Autoban (name)");
        SendChatTarget(ClientID, "- Check_Banlistfor (stringTOcheck4)");
        SendChatTarget(ClientID, "- Delete_BanlistLine (FileLine)");
        SendChatTarget(ClientID, "- Tele");
        SendChatTarget(ClientID, "- Clientban (clientid (/getclientid #VIP))");
        SendChatTarget(ClientID, "- Find_ClientBanlist (clientid)");
        SendChatTarget(ClientID, "- Delete_ClientBanlist (clientid)");
        SendChatTarget(ClientID, "- ShowWhispers");
        SendChatTarget(ClientID, "- All VIP features (/vipinfo)");
        SendChatTarget(ClientID, "=======================");
    }
    else if (str_comp_nocase_num(pMsg + 1, "w ", 2) == 0)
    {
        char pWhisperMsg[256];
        str_copy(pWhisperMsg, pMsg + 3, 256);
        Whisper(ClientID, pWhisperMsg);
    }
    else if (str_comp_nocase_num(pMsg + 1, "whisper ", 8) == 0)
    {
        char pWhisperMsg[256];
        str_copy(pWhisperMsg, pMsg + 9, 256);
        Whisper(ClientID, pWhisperMsg);
    }
    else if (str_comp_nocase_num(pMsg + 1, "c ", 2) == 0)
    {
        char pWhisperMsg[256];
        str_copy(pWhisperMsg, pMsg + 3, 256);
        Converse(ClientID, pWhisperMsg);
    }
    else if (str_comp_nocase_num(pMsg + 1, "converse ", 9) == 0)
    {
        char pWhisperMsg[256];
        str_copy(pWhisperMsg, pMsg + 10, 256);
        Converse(ClientID, pWhisperMsg);
    }
    else
    {
        if (g_Config.m_SvSpamprotection && str_comp_nocase_num(pMsg + 1, "timeout ", 8) != 0
            && pPlayer->m_LastCommands[0] && pPlayer->m_LastCommands[0] + Server()->TickSpeed() > Server()->Tick()
            && pPlayer->m_LastCommands[1] && pPlayer->m_LastCommands[1] + Server()->TickSpeed() > Server()->Tick()
            && pPlayer->m_LastCommands[2] && pPlayer->m_LastCommands[2] + Server()->TickSpeed() > Server()->Tick()
            && pPlayer->m_LastCommands[3] && pPlayer->m_LastCommands[3] + Server()->TickSpeed() > Server()->Tick()
            )
            return;

        int64 Now = Server()->Tick();
        pPlayer->m_LastCommands[pPlayer->m_LastCommandPos] = Now;
        pPlayer->m_LastCommandPos = (pPlayer->m_LastCommandPos + 1) % 4;

        m_ChatResponseTargetID = ClientID;
        Server()->RestrictRconOutput(ClientID);
        Console()->SetFlagMask(CFGFLAG_CHAT);

        if (pPlayer->m_Authed)
            Console()->SetAccessLevel(pPlayer->m_Authed == CServer::AUTHED_ADMIN ? IConsole::ACCESS_LEVEL_ADMIN : pPlayer->m_Authed == CServer::AUTHED_MOD ? IConsole::ACCESS_LEVEL_MOD : IConsole::ACCESS_LEVEL_HELPER);
        else
            Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_USER);
        Console()->SetPrintOutputLevel(m_ChatPrintCBIndex, 0);

        bool InterpretSemicolons = !(pPlayer->m_PlayerFlags & PLAYERFLAG_CHATTING);
        Console()->ExecuteLine(pMsg + 1, ClientID, InterpretSemicolons);
        // pPlayer can be NULL, if the player used a
        // timeout code and replaced another client.
        if (InterpretSemicolons && pPlayer && !pPlayer->m_SentSemicolonTip)
        {
            bool FoundSemicolons = false;

            const char *pStr = pMsg + 1;
            int Length = str_length(pStr);
            bool InString = false;
            bool Escape = false;
            for (int i = 0; i < Length; i++)
            {
                char c = pStr[i];
                if (InString)
                {
                    if (Escape)
                    {
                        Escape = false;
                        if (c == '\\' || c == '"')
                        {
                            continue;
                        }
                    }
                    else if (c == '\\')
                    {
                        Escape = true;
                    }
                    else if (c == '"')
                    {
                        InString = false;
                    }
                }
                else
                {
                    if (c == '"')
                    {
                        InString = true;
                    }
                    else if (c == ';')
                    {
                        FoundSemicolons = true;
                        break;
                    }
                }
            }
            static const char s_aPrefix[] = "mc;";
            static const int s_PrefixLength = str_length(s_aPrefix);
            if (FoundSemicolons && !(Length >= s_PrefixLength && str_comp_num(pStr, s_aPrefix, s_PrefixLength) == 0))
            {
                SendChatTarget(ClientID, "Usage of semicolons without /mc is deprecated");
                char aBuf[256];
                str_format(aBuf, sizeof(aBuf), "Try changing your bind to '/mc;%s'", pStr);
                SendChatTarget(ClientID, aBuf);
                pPlayer->m_SentSemicolonTip = true;
            }
        }
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d used %s", ClientID, pMsg);
        Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "chat-command", aBuf);

        Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
        Console()->SetFlagMask(CFGFLAG_SERVER);
        m_ChatResponseTargetID = -1;
        Server()->RestrictRconOutput(-1);
    }
}
