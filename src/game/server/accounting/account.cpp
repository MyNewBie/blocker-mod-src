
#include <game/server/player.h>

#include "account.h"

CAccount::CAccount(class CPlayer *pPlayer)
{
	m_pPlayer = pPlayer;
	m_pGameServer = pPlayer->GameServer();
}