#pragma once

typedef void (*SqlResultFunction)(bool Failed, void *pResultData, void *pData);

class CDatabase
{
private:
	char m_aAddr[32]; char m_aName[128]; char m_aPass[128]; char m_aSchema[64];

protected:
	void Init(const char *pAddress, const char *pName, const char *pPassword, const char *pSchema);

	void CreateNewQuery(char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema = true, bool Threading = true);
	static void CreateNewQuery(const char *pAddress, const char *pName, const char *pPassword, const char *pSchema, char *pQuery, SqlResultFunction ResultCallback, void *pData, bool ExpectResult, bool SetSchema = true, bool Threading = true);

	static void DatabaseStringAppend(char *pDst, const char *pStr, int DstSize);
	static void DatabaseStringCopy(char *pDst, const char *pStr, int DstSize);
	static void DatabaseStringCopyRevert(char *pDst, const char *pStr, int DstSize);

	static const int QUERY_MAX_LEN = 512;

public:

	struct CThreadFeed
	{
		SqlResultFunction m_ResultCallback;
		void *m_pResultData;
		char m_aCommand[QUERY_MAX_LEN];
		char m_aAddr[32]; char m_aName[128]; char m_aPass[128]; char m_aSchema[64];
		bool m_SetSchema;
		bool m_ExpectResult;
	};
};