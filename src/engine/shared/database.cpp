

#if defined(CONF_SQL)
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#endif

#include <base/system.h>
#include <engine/shared/config.h>

#include "database.h"

static LOCK s_QueryLock = lock_create();

static void ExecuteQuery(void *pData)
{
	CDatabase::CThreadFeed *pFeed = (CDatabase::CThreadFeed *)pData;

#if defined(CONF_SQL)
	sql::Driver *pDriver = NULL;
	sql::Connection *pConnection = NULL;
	sql::Statement *pStatement = NULL;
	sql::ResultSet *pResults = NULL;

	lock_wait(s_QueryLock);

	try
	{
		pDriver = get_driver_instance();
		pConnection = pDriver->connect(pFeed->m_aAddr, pFeed->m_aName, pFeed->m_aPass);
		if(pFeed->m_SetSchema == true)
			pConnection->setSchema(pFeed->m_aSchema);

		pStatement = pConnection->createStatement();
		if(pFeed->m_SetSchema == true && pFeed->m_ExpectResult)
			pResults = pStatement->executeQuery(pFeed->m_aCommand);
		else
			pStatement->execute(pFeed->m_aCommand);

		if(pFeed->m_ResultCallback != NULL)//no error
			pFeed->m_ResultCallback(false, pResults, pFeed->m_pResultData);
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: %s", e.what());
		log_file(e.what(), "Queries.log", g_Config.m_SvSecurityPath);
		if(pFeed->m_ResultCallback != NULL)
			pFeed->m_ResultCallback(true, NULL, pFeed->m_pResultData);
	}

	if (pResults)
			delete pResults;
	if (pConnection)
		delete pConnection;

	lock_unlock(s_QueryLock);

	delete pFeed;

#endif

}


void CDatabase::Init(const char *pAddress, const char *pName, const char *pPassword, const char *pSchema)
{
	str_copy(m_aAddr, pAddress, sizeof(m_aAddr));
	str_copy(m_aName, pName, sizeof(m_aName));
	str_copy(m_aPass, pPassword, sizeof(m_aPass));
	str_copy(m_aSchema, pSchema, sizeof(m_aSchema));
}

void CDatabase::CreateNewQuery(char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema, bool Threading)
{
	CreateNewQuery(m_aAddr, m_aName, m_aPass, m_aSchema, pQuery, ResultCallback, pData, ExpectResult, SetSchema, Threading);
}

void CDatabase::CreateNewQuery(const char *pAddress, const char *pName, const char *pPassword, const char *pSchema, char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema, bool Threading)
{
	CThreadFeed *pFeed = new CThreadFeed();
	pFeed->m_ResultCallback = ResultCallback;
	pFeed->m_pResultData = pData;
	str_copy(pFeed->m_aCommand, pQuery, sizeof(pFeed->m_aCommand));
	pFeed->m_SetSchema = SetSchema;
	pFeed->m_ExpectResult = ExpectResult;
	str_copy(pFeed->m_aAddr, pAddress, sizeof(pFeed->m_aAddr));
	str_copy(pFeed->m_aName, pName, sizeof(pFeed->m_aName));
	str_copy(pFeed->m_aPass, pPassword, sizeof(pFeed->m_aPass));
	str_copy(pFeed->m_aSchema, pSchema, sizeof(pFeed->m_aSchema));

	if(Threading == false)
		ExecuteQuery(pFeed);
	else
		thread_init(ExecuteQuery, pFeed);

	dbg_msg("SQL", pQuery);
	log_file(pQuery, "Queries.log", g_Config.m_SvSecurityPath);
}

void CDatabase::DatabaseStringAppend(char *pDst, const char *pStr, int DstSize)
{
	int Len = str_length(pStr);
	int DstPos = str_length(pDst);
	for(int i = 0; i < Len; i++)
	{
		if(DstPos >= DstSize - 3)
		{
			dbg_msg(0, "size");
			return;
		}

		if(pStr[i] == '\\' || pStr[i] == '\'' || pStr[i] == '\"')
			pDst[DstPos++] = '\\';

		pDst[DstPos++] = pStr[i];
	}
	pDst[DstPos] = '\0';
}

void CDatabase::DatabaseStringCopy(char *pDst, const char *pStr, int DstSize)
{
	int Len = str_length(pStr);
	int DstPos = 0;
	for(int i = 0; i < Len; i++)
	{
		if(DstPos >= DstSize - 3)
			return;

		if(pStr[i] == '\\' || pStr[i] == '\'' || pStr[i] == '\"')
			pDst[DstPos++] = '\\';

		pDst[DstPos++] = pStr[i];
	}
	pDst[DstPos] = '\0';
}

void CDatabase::DatabaseStringCopyRevert(char *pDst, const char *pStr, int DstSize)
{
	int Len = str_length(pStr);
	int DstPos = 0;
	for(int i = 0; i < Len; i++)
	{
		if(DstPos >= DstSize - 3)
			return;

		if(pStr[i] == '\\')
			continue;

		pDst[DstPos++] = pStr[i];
	}
	pDst[DstPos] = '\0';
}