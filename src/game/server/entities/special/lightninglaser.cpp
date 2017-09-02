/*
	by loic (Rei)
*/

#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/server/teams.h>

#include "lightninglaser.h"

CLightningLaser::CLightningLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Dir = Direction; //
	m_iStartTick = -1.6f;

	m_LockedPlayer = -1;

	m_iLifespan = m_iStartLifespan = Server()->TickSpeed()/4; // timer for the laser

	m_aaPositions[0][POS_START] = Pos;
	for(int i=0;i<LIGHTNING_COUNT;i++)
		m_aiIDs[i] = Server()->SnapNewID();
	
	GenerateLights(); // we generate position of lights

	GameWorld()->InsertEntity(this);
}

void CLightningLaser::GenerateLights()
{
	float randShot;
	for(int i = 0; i < LIGHTNING_COUNT; i ++)
	{
		m_aaPositions[i][POS_START] = i != 0 ? m_aaPositions[i - 1][POS_END] : m_Pos;
	
		for(int j = 0; j < 20; j ++)
		{
			randShot = GetAngle(m_Dir) + (rand() % 90 - 45) * pi / 180.f;
			
			m_aaPositions[i][POS_END] = m_aaPositions[i][POS_START] + GetDir(randShot) * LIGHTNING_LEN;

			if(!GameServer()->Collision()->CheckPoint(m_aaPositions[i][POS_END]))
			{
				break;
			}
		}

		vec2 CollisionPos;
		if (GameServer()->Collision()->IntersectLine(m_aaPositions[i][POS_START], m_aaPositions[i][POS_END], &CollisionPos, NULL)) 
		{
	        // if here is a wall, then don't go throught the wall
			m_aaPositions[i][POS_END] = CollisionPos;
		}
	}
}

void CLightningLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
	for(int i = 0; i < LIGHTNING_COUNT; i ++)
        Server()->SnapFreeID(m_aiIDs[i]);
}

void CLightningLaser::HitCharacter()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	for(int i = 0; i < LIGHTNING_COUNT; i ++)
	{
		for(int j = 0; j < MAX_CLIENTS; j++)
		{
			if (GameServer()->GetPlayerChar(j) && m_aaPositions[i-1][POS_END] == GameServer()->GetPlayerChar(j)->m_Pos)
			{
				m_aaPositions[i][POS_END] = m_aaPositions[i][POS_START] = m_aaPositions[i-1][POS_END];
				break;
			}
		}

		CCharacter *pClosestChar = GameWorld()->ClosestCharacter(m_aaPositions[i][POS_END], (float)LIGHTNING_LEN + 10.f, pOwnerChar);

		if(pClosestChar && pOwnerChar && GameServer()->GetPlayerChar(pClosestChar->GetPlayer()->GetCID())->Team() == pOwnerChar->Team())
		{
			// closest point in the light near of a tee
			vec2 CPoint        = closest_point_on_line(m_aaPositions[i][POS_END], m_aaPositions[i][POS_START], pClosestChar->m_Pos);
			bool IntersectLine = GameServer()->Collision()->IntersectLine(m_aaPositions[i][POS_START], pClosestChar->m_Pos, NULL, NULL);

			// hit
			if(distance(CPoint, pClosestChar->m_Pos) <= LIGHTNING_LEN + 10 && !IntersectLine)
			{
		     	m_aaPositions[i][POS_END] = pClosestChar->m_Pos;
			}

			if(distance(CPoint, pClosestChar->m_Pos) < 10)
			{
				// freeze if the player is near the light
				pClosestChar->Freeze(); 
				pClosestChar->Core()->m_Vel = vec2(0.f,0.f);
				m_LockedPlayer = pClosestChar->GetPlayer()->GetCID();
			}
		}
	}
}

void CLightningLaser::Tick()
{
	m_iLifespan--;

	if(m_iLifespan <= 0)
	{
		Reset();
		return;
	}
	else if(m_iLifespan <= 3)
		m_iStartTick--;

	HitCharacter();
}

void CLightningLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter *pOwnerChar = 0;
	int64_t TeamMask = -1LL;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if (pOwnerChar && pOwnerChar->IsAlive())
			TeamMask = pOwnerChar->Teams()->TeamMask(pOwnerChar->Team(), -1, m_Owner);

	if(!CmaskIsSet(TeamMask, SnappingClient))
		return;

	float fPercentage = 100.f - (m_iLifespan * 100.f / m_iStartLifespan);

	int iStart = (int)max((double)ceil(fPercentage * LIGHTNING_COUNT / 100.f), 1.0);
	CNetObj_Laser *apObjs[LIGHTNING_COUNT];

	for(int i = iStart - 1; i >= 0; i --)
	{
		apObjs[i] = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_aiIDs[i], sizeof(CNetObj_Laser)));
		if(!apObjs[i])
			return;

		// POS_END and POS_START reversed
		apObjs[i]->m_X = (int)m_aaPositions[i][POS_START].x;
		apObjs[i]->m_Y = (int)m_aaPositions[i][POS_START].y;
		apObjs[i]->m_FromX = (int)m_aaPositions[i][POS_END].x;
		apObjs[i]->m_FromY = (int)m_aaPositions[i][POS_END].y;
		apObjs[i]->m_StartTick = Server()->Tick()+m_iStartTick;
	}
}
