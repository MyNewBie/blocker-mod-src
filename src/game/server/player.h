/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"
#include "accounting/account.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

	friend class CSaveTee;

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Reset();

	void TryRespawn();
	void Respawn(bool WeakHook = false); // with WeakHook == true the character will be spawned after all calls of Tick from other Players
	CCharacter* ForceSpawn(vec2 Pos); // required for loading savegames
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();

	// will be called after all Tick and PostTick calls from other players
	void PostPostTick();
	void Snap(int SnappingClient);
	void FakeSnap();

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void ThreadKillCharacter(int Weapon = WEAPON_GAME);
	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	void FindDuplicateSkins();

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;
	int m_TuneZone;
	int m_TuneZoneOld;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;
	
	//special
	int m_Vacuum;
    
	bool m_RconFreeze;
	bool m_IsReady;
	bool m_IsRocket;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;
	int m_LastCommands[4];
	int m_LastCommandPos;
	int m_LastWhisperTo;

	int m_SendVoteIndex;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_DieTick;
	int m_Score;
	int m_JoinTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	bool m_StolenSkin;
	int m_TeamChangeTick;
	bool m_SentSemicolonTip;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;
	
	struct
	{
		int m_ZoneXp;
		int m_ZonePoints;

		bool m_InZone;

		void Reset()
		{
			m_ZoneXp = 0;
			m_ZonePoints = 0;

			m_InZone = false;
		}
	}m_Koh;

	// Some things for rewards, user can get temporary access to X for X amount of time
	struct
	{
		bool m_PassiveMode;

		int m_Weaponcalls;
		int64 m_PassiveModeTime;
		int m_PassiveTimeLength;
	}Temporary;

	struct		//saved player infos when entering tournament; restores them after
	{
		vec2 m_SavedSpawn;
		bool m_SavedShotgun;
		bool m_SavedGrenade;
		bool m_SavedLaser;
		bool m_SavedEHook;
		
		int m_SavedStartTick;
		
		void Reset()
		{
			m_SavedSpawn = vec2(0,0);
			m_SavedShotgun = false;
			m_SavedGrenade = false;
			m_SavedLaser = false;
			m_SavedEHook = false;
			m_SavedStartTick = 0;
		}
	}m_SavedStats;
	
	void SaveStats();
	
	int m_InLMB;

	struct
	{
		bool m_SmartHammer;
		bool m_AutoHook;
		bool m_Grenadebot;
		
		bool m_Active;
	} m_Bots;

	enum
	{
		QUEST_PART1 = 1,
		QUEST_PART2,
		QUEST_PART3,
	};

	struct
	{
		// Main
		bool m_CompletedQuest,
			m_RifledTarget,
			m_HookedTarget,
			m_ShotgunedTarget,
			m_HammeredTarget;

		bool m_QuestInSession;
		bool m_Rstartkill;

		int m_RandomID;
		int m_RaceTime;
		int m_Pages;
		int m_QuestPart;
	} m_QuestData;

	void QuestReset();

	bool m_EpicCircle;
	bool m_Rainbowepiletic; // Epiletic rainbow!
	bool m_Rainbow;
	bool m_Called;
	int m_OldColorBody;
	int m_OldColorFeet;
	int m_OldCustom;
	int m_LastRainbow;
	int m_LastRainbow2;
	bool m_DeathNote;
	int m_Killedby;
	bool m_Blackhole;
	bool m_IsEmote;

	// City
	struct
	{
		// Main
		char m_Username[32];
		char m_Password[32];
		char m_RconPassword[32];
		int m_UserID;
		int m_Vip;

	} m_AccData;

	class CAccount *m_pAccount;//(CPlayer *m_Player, CGameContext *gameserver);

private:
	CCharacter *m_pCharacter;
	int m_NumInputs;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	bool m_WeakHookSpawn;
	int m_ClientID;
	int m_Team;


	// DDRace

public:
	enum
	{
		PAUSED_NONE=0,
		PAUSED_SPEC,
		PAUSED_PAUSED,
		PAUSED_FORCE
	};

	int m_Paused;
	bool m_DND;
	int64 m_FirstVoteTick;
	int64 m_NextPauseTick;
	char m_TimeoutCode[64];

	void ProcessPause();
	int m_ForcePauseTime;
	bool IsPlaying();
	int64 m_Last_KickVote;
	int64 m_Last_Team;
	int m_Authed;
	int m_ClientVersion;
	bool m_ShowOthers;
	bool m_ShowAll;
	bool m_SpecTeam;
	bool m_NinjaJetpack;
	bool m_Afk;
	int m_KillMe;

	int m_ChatScore;

	bool AfkTimer(int new_target_x, int new_target_y); //returns true if kicked
	void AfkVoteTimer(CNetObj_PlayerInput *NewTarget);
	int64 m_LastPlaytime;
	int64 m_LastEyeEmote;
	int m_LastTarget_x;
	int m_LastTarget_y;
	CNetObj_PlayerInput m_LastTarget;
	int m_Sent1stAfkWarning; // afk timer's 1st warning after 50% of sv_max_afk_time
	int m_Sent2ndAfkWarning; // afk timer's 2nd warning after 90% of sv_max_afk_time
	char m_pAfkMsg[160];
	bool m_EyeEmote;
	int m_TimerType;
	int m_DefEmote;
	int m_DefEmoteReset;
	bool m_Halloween;
	bool m_FirstPacket;
#if defined(CONF_SQL)
	int64 m_LastSQLQuery;
#endif
};

#endif
