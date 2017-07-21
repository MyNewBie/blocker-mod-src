
#include <engine/server.h>

#include "player.h"
#include "botprotections.h"

CBotProtections::CBotProtections(CPlayer *pPlayer)
{
	m_pPlayer = pPlayer;
	m_pGameContext = pPlayer->GameServer();
	m_pServer = pPlayer->Server();

	m_HAFinishedAnalyse = false;
	m_HAHooking = false;
	m_HAHookingTee = -1;
	m_HALastFire = 0;
	m_HACountTotal = 0;
	m_HACountConspicuous = 0;
	m_HALastGotHooked = 0;
	m_HALastHammered = 0;
}

void CBotProtections::HAGetHookedData()
{
	int OwnID = Player()->GetCID();
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		if(pChr == NULL || pChr->IsAlive() == false || i == OwnID)
			continue;

		if(pChr->m_Core.m_HookedPlayer == OwnID)
			m_HALastGotHooked = Server()->Tick();
	}
}

void CBotProtections::HAGetHammeredData(CNetObj_PlayerInput *pInput)
{
	CCharacter *pChr = Player()->GetCharacter();
	if(pChr == NULL || pChr->IsAlive() == false)
		return;

	if(pInput->m_Fire != m_HALastFire && (pChr->GetActiveWeapon() == WEAPON_HAMMER || pChr->GetActiveWeapon() == WEAPON_GRENADE))
	{
		m_HALastHammered = Server()->Tick();
		m_HALastFire = pInput->m_Fire;
	}
}

void CBotProtections::HATick()
{
	HAGetHookedData();

	if(m_HAFinishedAnalyse == true)
		return;

	CCharacter *pChr = Player()->GetCharacter();
	if(pChr == NULL || pChr->IsAlive() == false)
		return;

	CCharacterCore *pCore = &pChr->m_Core;
	if(pCore->m_HookState != HOOK_FLYING && pCore->m_HookState != HOOK_GRABBED)
	{
		m_HAFinishedAnalyse = false;//no tee got hooked
		return;
	}

	if(pCore->m_HookedPlayer == -1)
		return;

	//filter some shit for prevent bypassing this protection
	if(Server()->Tick() - m_HALastHammered > Server()->TickSpeed() * 3.0f ||
		Server()->Tick() - m_HALastGotHooked > Server()->TickSpeed() * 2.0f)
		return;

	if(pCore->m_HookedPlayer == m_HAHookingTee)
		m_HACountConspicuous++;

	m_HACountTotal++;
	m_HAFinishedAnalyse = true;
}

void CBotProtections::Tick()
{
	HATick();
}

void CBotProtections::HAInspectInput(CNetObj_PlayerInput *pInput)
{
	CCharacter *pChr = Player()->GetCharacter();
	if(pChr == NULL || pChr->IsAlive() == false)
		return;

	m_HAHookingTee = -1; //not Conspicuous yet
	m_HAFinishedAnalyse = false; // need to analys this hook

	vec2 MousePos = pChr->m_Pos + vec2(pInput->m_TargetX, pInput->m_TargetY);
	CCharacter *pHookingTee = GameServer()->m_World.ClosestCharacter(MousePos, 30.0f, pChr);
	if(pHookingTee == NULL || pHookingTee->IsAlive() == false)
		return;

	if(distance(pChr->m_Pos, pHookingTee->m_Pos) > GameServer()->Tuning()->m_HookLength + 32.0f)//little tollerance
		return;

	m_HAHookingTee = pHookingTee->GetPlayer()->GetCID();//Conspicuous
}

void CBotProtections::HANewInput(CNetObj_PlayerInput *pInput)
{
	if(m_HAHooking == false)
	{
		if(pInput->m_Hook == 1)
		{
			HAInspectInput(pInput);
			m_HAHooking = true;
		}
	}
	else
		m_HAHooking = (bool) pInput->m_Hook;

	HAGetHammeredData(pInput);
}


void CBotProtections::NewInput(CNetObj_PlayerInput *pInput)
{
	HANewInput(pInput);
}