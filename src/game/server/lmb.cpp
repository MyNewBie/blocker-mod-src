#include "lmb.h"
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <game/server/accounting/account_database.h>

struct CDatabaseUpdateData
{
	CGameContext *m_pGameServer;
	CPlayer *m_pPlayer;
};

CLMB::CLMB()
{
	m_State = STATE_STANDBY;
	//m_LastLMB = m_pGameServer->Server()->Tick();
}

CLMB::~CLMB()
{

}

void CLMB::OpenRegistration()
{
	if (m_State > STATE_STANDBY)
		return;

	m_Participants.clear();

	m_pGameServer->SendChatTarget(-1, "'Last man Blocking' [LMB] registration has opened.");
	m_pGameServer->SendChatTarget(-1, "Use /lmb to participate.");

	dbg_msg("LMB", "Start of an LMB round!");

	m_State = STATE_REGISTRATION;
	m_StartTick = m_pGameServer->Server()->Tick() + 1 + 3000 * g_Config.m_SvLMBRegDuration;	//3000 ticks in one minute! +1 so we get all messages right
}

void CLMB::Tick()
{
	if (m_State == STATE_REGISTRATION)
	{
		int Diff = m_StartTick - m_pGameServer->Server()->Tick();

		if (Diff % 3000 == 0 && Diff > 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "LMB will start in %d minutes.", (m_StartTick - m_pGameServer->Server()->Tick()) / 3000);
			m_pGameServer->SendChatTarget(-1, aBuf);
			m_pGameServer->SendChatTarget(-1, "Please use /lmb if you want to participate now");

			if (Diff == 3000)
				m_pGameServer->m_pController->DoWarmup(60);
		}
		else if (Diff <= 0)
		{
			if (m_Participants.size() >= (unsigned)g_Config.m_SvLMBMinPlayer)
			{
				m_pGameServer->SendChatTarget(-1, "LMB has started.");
				m_State = STATE_RUNNING;
				m_StartTick = m_pGameServer->Server()->Tick();
				TeleportParticipants();
				m_pGameServer->SendBroadcast("", -1);
			}
			else
			{
				dbg_msg("LMB", "Not enough players for a tournament");
				m_pGameServer->SendChatTarget(-1, "There were not enough players for a tournament.");
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "It needs at least %d players to start.", g_Config.m_SvLMBMinPlayer);
				m_pGameServer->SendChatTarget(-1, aBuf);

				m_State = STATE_STANDBY;
				for (unsigned int i = 0; i < m_Participants.size(); i++)
				{
					if (m_pGameServer->m_apPlayers[m_Participants[i]])
						m_pGameServer->m_apPlayers[m_Participants[i]]->m_InLMB = LMB_NONREGISTERED;
				}

				m_Participants.clear();
				m_pGameServer->SendBroadcast(" ", -1);
			}
		}

		if (m_pGameServer->Server()->Tick() % SERVER_TICK_SPEED == 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Player: [%d/%d] Use /lmb to participate.", ParticipantNum(), g_Config.m_SvLMBMaxPlayer, g_Config.m_SvLMBMinPlayer);
			m_pGameServer->SendBroadcast(aBuf, -1);
		}
	}
	else if (m_Participants.size() && m_State == STATE_STANDBY && m_pGameServer->Server()->Tick() > m_EndTick + 500)
	{
		dbg_msg("LMB", "Removing winner from arena (%d)", m_Participants[0]);

		m_pGameServer->m_apPlayers[m_Participants[0]]->m_InLMB = LMB_NONREGISTERED;
		if (m_pGameServer->m_apPlayers[m_Participants[0]]->GetCharacter())
		{
			m_pGameServer->m_apPlayers[m_Participants[0]]->KillCharacter();
			m_pGameServer->m_apPlayers[m_Participants[0]]->m_DieTick = m_pGameServer->Server()->Tick() - m_pGameServer->Server()->TickSpeed() * 3;
		}
		m_State = STATE_STANDBY;
		m_Participants.clear();
	}
	else if (m_State == STATE_RUNNING)
	{
		if (m_pGameServer->Server()->Tick() >= m_StartTick + 3000 * g_Config.m_SvLMBTournamentTime)	//force cancel
		{
			m_State = STATE_STANDBY;

			while (m_Participants.size())
			{		//killed player will get removed from the list; so we just have to kill ID 0 all the time!
					//m_pGameServer->m_apPlayers[m_Participants[0]]->m_InLMB = 0;
				m_pGameServer->m_apPlayers[m_Participants[0]]->m_InLMB = LMB_NONREGISTERED;
				if (m_pGameServer->m_apPlayers[m_Participants[0]] && m_pGameServer->m_apPlayers[m_Participants[0]]->GetCharacter())
				{
					dbg_msg("LMB", "Killing %d because tournament-time is over!", m_Participants[0]);
					//m_pGameServer->m_apPlayers[m_Participants[0]]->m_InLMB = 0;
					m_pGameServer->m_apPlayers[m_Participants[0]]->KillCharacter();
					m_pGameServer->m_apPlayers[m_Participants[0]]->m_DieTick = m_pGameServer->Server()->Tick() - m_pGameServer->Server()->TickSpeed() * 3;
				}
				m_Participants.erase(m_Participants.begin());
			}

			m_pGameServer->SendChatTarget(-1, "Tournament has been closed due to the timelimit.");

			m_LastLMB = m_pGameServer->Server()->Tick();
		}
	}
}

bool CLMB::RegisterPlayer(int ID)
{
	std::vector<int>::iterator it = FindParticipant(ID);

	if (it == m_Participants.end() && m_pGameServer->m_apPlayers[ID])	//register him!
	{
		m_Participants.push_back(ID);
		m_pGameServer->m_apPlayers[ID]->m_InLMB = LMB_REGISTERED;
		return true;
	}

	if (m_pGameServer->m_apPlayers[ID])
		m_pGameServer->m_apPlayers[ID]->m_InLMB = LMB_NONREGISTERED;

	m_Participants.erase(it);
	return false;
}

bool CLMB::IsParticipant(int ID)
{
	return !(FindParticipant(ID) == m_Participants.end());

}

std::vector<int>::iterator CLMB::FindParticipant(int ID)
{
	std::vector<int>::iterator it = std::find(m_Participants.begin(), m_Participants.end(), ID);
	return it;
}

void CLMB::TeleportParticipants()
{
	for (unsigned int i = 0; i < m_Participants.size(); i++)
	{
		vec2 ActPos; bool CanSpawn;

		if ((CanSpawn = m_pGameServer->m_pController->CanSpawn(2, &ActPos)) && m_pGameServer->m_apPlayers[m_Participants[i]])
		{
			if (m_pGameServer->m_apPlayers[m_Participants[i]]->GetCharacter() && m_pGameServer->m_apPlayers[m_Participants[i]]->m_InLMB == LMB_REGISTERED)
			{
				m_pGameServer->m_World.ReleaseHooked(i);
				m_pGameServer->m_apPlayers[m_Participants[i]]->SaveStats();
				Reset(m_Participants[i]);
				m_pGameServer->m_apPlayers[m_Participants[i]]->KillCharacter();
				m_pGameServer->m_apPlayers[m_Participants[i]]->m_InLMB = LMB_PARTICIPATE;
				m_pGameServer->m_apPlayers[m_Participants[i]]->m_WasInLMB = true;
			}
			else
			{
				m_pGameServer->SendChatTarget(m_Participants[i], "Oops! Something went wrong! Please assure that you are alive when the tournament starts.");
				m_pGameServer->m_apPlayers[m_Participants[i]]->m_InLMB = LMB_NONREGISTERED;
				RemoveParticipant(m_Participants[i]);
			}
		}
		else
		{
			m_pGameServer->SendChatTarget(m_Participants[i], "Oops, some strange error happened. We're sorry. If you have information for us what exactly happened, please tell the team.");
			if (m_pGameServer->m_apPlayers[m_Participants[i]])
			{
				dbg_msg("LMB/ERROR", "Error while joining the tournament: ID=%d, SpawnPos=(%.0f %.0f; can%sspawn here)", m_Participants[i], ActPos.x, ActPos.y, CanSpawn ? " " : " NOT ");
				m_pGameServer->m_apPlayers[m_Participants[i]]->m_InLMB = LMB_NONREGISTERED;
			}
			else
				dbg_msg("LMB/ERROR", "Player with ID=%d failed to join the tournament: Pointer not valid anymore.");
			RemoveParticipant(m_Participants[i]);
		}
	}
}

void CLMB::DatabaseUpdate(bool Failed, void *pResultData, void *pData)
{
	CDatabaseUpdateData *pUpdateData = (CDatabaseUpdateData *) pData;
	CPlayer *Winner = pUpdateData->m_pPlayer;
	CGameContext *pGamecontext = pUpdateData->m_pGameServer;
	int ID = Winner->GetCID();

	if (Winner->m_AccData.m_UserID)
	{

		pGamecontext->SendChatTarget(ID, "+2 pages!");
		pGamecontext->SendChatTarget(ID, "(+3) WeaponKits (use /weapons)!");
		pGamecontext->SendChatTarget(ID, "Temoporary access to passivemode for 2Hours! (Anti WB)");

		Winner->m_QuestData.m_Pages += 2;
		Winner->m_AccData.m_Weaponkits += 3;
		Winner->Temporary.m_PassiveMode = true;
		Winner->Temporary.m_PassiveModeTime = pGamecontext->Server()->Tick();
		Winner->Temporary.m_PassiveTimeLength = 10800;

		CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(Winner->m_pAccount);
		if(pAccDb)
			pAccDb->ApplyUpdatedData();
	}
}

void CLMB::RemoveParticipant(int CID)
{
	std::vector<int>::iterator it = FindParticipant(CID);
	m_Participants.erase(it);

	dbg_msg("LMB", "Removing %d!", CID);

	char aBuf[256];

	if (m_Participants.size() == 1 && m_State == STATE_RUNNING)
	{
		int ID = m_Participants[0];
		CPlayer *Winner = m_pGameServer->m_apPlayers[ID];
		str_format(aBuf, sizeof(aBuf), "'%s' won!", m_pGameServer->Server()->ClientName(ID));
		m_pGameServer->SendBroadcast(aBuf, -1);

		char aBuf2[256];
		str_format(aBuf2, sizeof(aBuf2), Winner->m_AccData.m_UserID ? "'%s' has won the LMB tournament and has been given rewards! \n Congratulations!" : "'%s' has won the LMB tournament! Congratulations.", m_pGameServer->Server()->ClientName(ID));
		m_pGameServer->SendChatTarget(-1, aBuf2);

		CAccountDatabase *pAccDb = dynamic_cast<CAccountDatabase *>(Winner->m_pAccount);
		CDatabaseUpdateData *pResultData = new CDatabaseUpdateData();
		pResultData->m_pGameServer = m_pGameServer;
		pResultData->m_pPlayer = Winner;

		if(pAccDb != NULL)
			pAccDb->ReloadUpdatedData(DatabaseUpdate, pResultData);
		else
			DatabaseUpdate(false, NULL, pResultData);

		dbg_msg("LMB", "%d has won!", ID);

		m_EndTick = m_pGameServer->Server()->Tick();
		m_State = STATE_STANDBY;
		m_LastLMB = m_pGameServer->Server()->Tick();
	}
	else if (m_State == STATE_RUNNING)
	{
		str_format(aBuf, sizeof(aBuf), "Players alive: %d", m_Participants.size());
		m_pGameServer->SendBroadcast(aBuf, -1);
	}

}

void CLMB::Reset(int ClientID)
{
	CPlayer* pPlayer = m_pGameServer->m_apPlayers[ClientID];
	pPlayer->m_Lovely = false;
	pPlayer->m_HeartGuns = false;
	pPlayer->m_IsBallSpawned = false;
	pPlayer->m_Rainbow = false;
	pPlayer->m_Rainbowepiletic = false;
	pPlayer->m_EpicCircle = false;
	pPlayer->m_RainbowHook = false;
	pPlayer->m_DefEmote = EMOTE_NORMAL;
}
