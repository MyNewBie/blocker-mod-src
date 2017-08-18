
#include <base/system.h>
#include <engine/shared/config.h>

#include "address_logger.h"

#define THREADING 1

void CAddressLogger::Init()
{
#if defined(CONF_SQL)
	CDatabase::Init(g_Config.m_SvAlAccSqlIp, g_Config.m_SvAlSqlName, g_Config.m_SvAlSqlPassword, g_Config.m_SvAlSqlDatabase);

	CreateTable();
#endif
}

void CAddressLogger::CreateTable()
{
	char aBuf[64];

#if defined(CONF_SQL)
	if(g_Config.m_SvAddresslogger == 0)
		return;

	str_format(aBuf, sizeof(aBuf), "CREATE DATABASE IF NOT EXISTS %s", g_Config.m_SvAlSqlDatabase);
	CreateNewQuery(aBuf, NULL, NULL, false, true, THREADING);

	CreateNewQuery("CREATE TABLE IF NOT EXISTS addresses (name VARCHAR(32) BINARY NOT NULL, address VARCHAR(64) BINARY NOT NULL, date DATETIME NOT NULL, PRIMARY KEY (name, address)) CHARACTER SET utf8 ;", NULL, NULL, false, true, THREADING);
#endif
}

void CAddressLogger::InsertEntry(const char *pName, const char *pAddress)
{
	char aQuery[QUERY_MAX_LEN];
	char aCleanName[32];

#if defined(CONF_SQL)
	if(g_Config.m_SvAddresslogger == 0)
		return;
#else
		return;
#endif

	DatabaseStringCopy(aCleanName, pName, sizeof(aCleanName));
	str_format(aQuery, sizeof(aQuery), "REPLACE INTO addresses(name, address, date) VALUES('%s', '%s', NOW())", aCleanName, pAddress);
	CreateNewQuery(aQuery, NULL, NULL, false, true, THREADING);

}