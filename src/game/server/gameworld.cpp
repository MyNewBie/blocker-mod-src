/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/system.h>

#include "gameworld.h"
#include "entity.h"
#include "gamecontext.h"
#include <algorithm>
#include <utility>
#include <engine/shared/config.h>
#include <game/mapitems.h>

bool distCompare(std::pair<float, int> a, std::pair<float, int> b)
{
	return (a.first < b.first);
}

struct CUpdateThreadData
{
	CGameWorld *m_pGameWorld;
	bool m_PlayerExists[MAX_CLIENTS];//online
	bool m_CharacterExists[MAX_CLIENTS];
	CPlayer *m_pMetaPlayers;
	CCharacter *m_aMetaCharacters;
	int m_Mapparts[MAX_CLIENTS];
} s_ThreadData;

void CGameWorld::UpdatePlayerMaps(void *pData)
{
	int64 s_ThreadStartTime = time_get();
	CUpdateThreadData *pThreadData = (CUpdateThreadData *)pData;
	CGameWorld *pThis = pThreadData->m_pGameWorld;

	//if (Server()->Tick() % g_Config.m_SvMapUpdateRate != 0) return;
	int IdMap[MAX_CLIENTS * DDNET_MAX_CLIENTS];
	mem_copy(&IdMap, pThis->Server()->GetIdMap(), sizeof(IdMap));

	std::pair<float, int> dist[MAX_CLIENTS];
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int* map = (int *) IdMap + i * DDNET_MAX_CLIENTS;
		if (pThreadData->m_PlayerExists[i] == false)
		{
			mem_set(map, -1, sizeof(int) * DDNET_MAX_CLIENTS);
			continue;
		}

		// compute distances
		for (int j = 0; j < MAX_CLIENTS; j++)
		{
			dist[j].second = j;
			if (pThreadData->m_PlayerExists[j] == false)
			{
				dist[j].first = 1e10;
				continue;
			}

			if(pThreadData->m_Mapparts[i] != pThreadData->m_Mapparts[j] /*&& pThreadData->m_pMetaPlayers[j].GetTeam() != TEAM_SPECTATORS && pThreadData->m_pMetaPlayers[i].GetTeam() != TEAM_SPECTATORS*/)
			{
				dist[j].first = 1e10;
				continue;
			}

			if (pThreadData->m_CharacterExists[j] == false)
			{
				dist[j].first = 1e9;
				continue;
			}
			// copypasted chunk from character.cpp Snap() follows
			CCharacter* SnapChar = &pThreadData->m_aMetaCharacters[i];
			if (SnapChar->m_Super && pThreadData->m_CharacterExists[i] &&
				!pThreadData->m_pMetaPlayers[i].m_Paused && pThreadData->m_pMetaPlayers[i].GetTeam() != -1 /*&&
				(pThreadData->m_pMetaPlayers[i].m_ClientVersion == VERSION_VANILLA ||
					(pThreadData->m_pMetaPlayers[i].m_ClientVersion >= VERSION_DDRACE &&
						!pThreadData->m_pMetaPlayers[i].m_ShowOthers
						)
					)*/
					)
				dist[j].first = 1e8;
			else
				dist[j].first = 0;

			dist[j].first += distance(pThreadData->m_pMetaPlayers[i].m_ViewPos, pThreadData->m_aMetaCharacters[j].m_Pos);
		}

		// always send the player himself
		dist[i].first = 0;

		// compute reverse map
		int rMap[MAX_CLIENTS];
		for (int j = 0; j < MAX_CLIENTS; j++)
		{
			rMap[j] = -1;
		}

		int Range = VANILLA_MAX_CLIENTS;
		if(pThreadData->m_pMetaPlayers[i].m_ClientVersion >= VERSION_DDNET_OLD)
			Range = DDNET_MAX_CLIENTS;

		for (int j = 0; j < Range; j++)
		{
			if (map[j] == -1) continue;
			if (dist[map[j]].first > 5e9) map[j] = -1;
			else rMap[map[j]] = j;
		}

		std::nth_element(&dist[0], &dist[Range - 1], &dist[MAX_CLIENTS], distCompare);

		int mapc = 0;
		int demand = 0;
		for (int j = 0; j < Range - 1; j++)
		{
			int k = dist[j].second;
			if (rMap[k] != -1 || dist[j].first > 5e9) continue;
			while (mapc < Range && map[mapc] != -1) mapc++;
			if (mapc < Range - 1)
				map[mapc] = k;
			else
				demand++;
		}
		for (int j = MAX_CLIENTS - 1; j > Range - 2; j--)
		{
			int k = dist[j].second;
			if (rMap[k] != -1 && demand-- > 0)
				map[rMap[k]] = -1;
		}
		map[Range - 1] = -1; // player with empty name to say chat msgs
	}

	pThis->Server()->WriteIdMap(IdMap);
}

void CGameWorld::UpdatePlayerMapsThread(void *pData)
{
	while(1)
		UpdatePlayerMaps(pData);
}

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;

	m_Paused = false;
	m_ResetRequested = false;
	for (int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for (int i = 0; i < NUM_ENTTYPES; i++)
		while (m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

void CGameWorld::InitThreadData()
{
	s_ThreadData.m_pGameWorld = this;
	mem_zero(&s_ThreadData.m_PlayerExists, sizeof(s_ThreadData.m_PlayerExists));
	mem_zero(&s_ThreadData.m_CharacterExists, sizeof(s_ThreadData.m_CharacterExists));
	s_ThreadData.m_pMetaPlayers = (CPlayer *)mem_alloc(sizeof(CPlayer) * MAX_CLIENTS, 1);
	s_ThreadData.m_aMetaCharacters = (CCharacter *)mem_alloc(sizeof(CCharacter) * MAX_CLIENTS, 1);
}

int CGameWorld::TranslateMappartTiles(int Index)
{
	int Mappart = -1;
	switch(Index)
	{
	case TILE_MAPPARTS_LOBBY: Mappart = MAPPART_LOBBY; break;
	case TILE_MAPPARTS_CHILL: Mappart = MAPPART_CHILL; break;
	case TILE_MAPPARTS_ROYAL: Mappart = MAPPART_ROYAL; break;
	case TILE_MAPPARTS_PEPE: Mappart = MAPPART_PEPE; break;
	case TILE_MAPPARTS_STARBLOCK: Mappart = MAPPART_STARBLOCK; break;
	case TILE_MAPPARTS_BAAM: Mappart = MAPPART_BAAM; break;
	case TILE_MAPPARTS_TOUCHUP: Mappart = MAPPART_TOUCHUP; break;
	case TILE_MAPPARTS_V3: Mappart = MAPPART_V3; break;
	case TILE_MAPPARTS_V2: Mappart = MAPPART_V2; break;
	case TILE_MAPPARTS_V5: Mappart = MAPPART_V5; break;
	}
	return Mappart;
}

void CGameWorld::UpdateThreadData()
{
	IServer::CClientInfo info;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		s_ThreadData.m_PlayerExists[i] = pPlayer != NULL;
		if(s_ThreadData.m_PlayerExists[i] == false)
			continue;
		mem_copy(&s_ThreadData.m_pMetaPlayers[i], pPlayer, sizeof(CPlayer));
		Server()->GetClientInfo(i, &info);
		s_ThreadData.m_Mapparts[i] = Server()->GetClientMappart(i);

		CCharacter *pChar = pPlayer->GetCharacter();
		s_ThreadData.m_CharacterExists[i] = pChar != NULL && pChar->IsAlive();
		if(s_ThreadData.m_CharacterExists[i] == false)
			continue;
		mem_copy(&s_ThreadData.m_aMetaCharacters[i], pChar, sizeof(CCharacter));
	}
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();

	InitThreadData();
	thread_init(UpdatePlayerMapsThread, &s_ThreadData);
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type, int Mappart)
{
	if (Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	for (CEntity *pEnt = m_apFirstEntityTypes[Type]; pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if(Mappart != pEnt->GetMappart()
			|| abs(pEnt->m_Pos.x - Pos.x) > Radius + pEnt->m_ProximityRadius
			|| abs(pEnt->m_Pos.y - Pos.y) > Radius + pEnt->m_ProximityRadius)
			continue;
		if (distance(pEnt->m_Pos, Pos) < Radius + pEnt->m_ProximityRadius)
		{
			if (ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if (Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for (CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
		dbg_assert(pCur != pEnt, "err");
#endif

	// insert it
	if (m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if (!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if (pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if (pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if (m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for (int i = 0; i < NUM_ENTTYPES; i++)
		for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Reset()
{
	// reset all entities
	for (int i = 0; i < NUM_ENTTYPES; i++)
		for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for (int i = 0; i < NUM_ENTTYPES; i++)
		for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if (pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Tick()
{
	if (m_ResetRequested)
		Reset();

	if (!m_Paused)
	{
		if (GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for (int i = 0; i < NUM_ENTTYPES; i++)
			for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		for (int i = 0; i < NUM_ENTTYPES; i++)
			for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}
	}
	else
	{
		// update all objects
		for (int i = 0; i < NUM_ENTTYPES; i++)
			for (CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();

	UpdateThreadData();
}

// TODO: should be more general
//CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, int Mappart, CCharacter *pNotThis, int CollideWith, class CCharacter *pThisOnly)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for (; p; p = (CCharacter *)p->TypeNext())
	{
		if (p == pNotThis || p->GetMappart() != Mappart)
			continue;

		if (CollideWith != -1 && !p->CanCollide(CollideWith))
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if (Len < p->m_ProximityRadius + Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if (Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis, int Mappart)
{
	// Find other players
	float ClosestRange = Radius * 2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER);
	for (; p; p = (CCharacter *)p->TypeNext())
	{
		if (p == pNotThis || Mappart != p->GetMappart())
			continue;

		float Len = distance(Pos, p->m_Pos);
		if (Len < p->m_ProximityRadius + Radius)
		{
			if (Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

std::list<class CCharacter *> CGameWorld::IntersectedCharacters(vec2 Pos0, vec2 Pos1, float Radius, class CEntity *pNotThis)
{
	std::list< CCharacter * > listOfChars;

	CCharacter *pChr = (CCharacter *)FindFirst(CGameWorld::ENTTYPE_CHARACTER);
	for (; pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if (pChr == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, pChr->m_Pos);
		float Len = distance(pChr->m_Pos, IntersectPos);
		if (Len < pChr->m_ProximityRadius + Radius)
		{
			pChr->m_Intersection = IntersectPos;
			listOfChars.push_back(pChr);
		}
	}
	return listOfChars;
}

void CGameWorld::ReleaseHooked(int ClientID)
{
	CCharacter *pChr = (CCharacter *)CGameWorld::FindFirst(CGameWorld::ENTTYPE_CHARACTER);
	for (; pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		CCharacterCore* Core = pChr->Core();
		if (Core->m_HookedPlayer == ClientID && !pChr->m_Super)
		{
			Core->m_HookedPlayer = -1;
			Core->m_HookState = HOOK_RETRACTED;
			Core->m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
			Core->m_HookState = HOOK_RETRACTED;
		}
	}
}
