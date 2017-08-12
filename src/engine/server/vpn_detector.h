#pragma once

#include <base/tl/array.h>

typedef void(VpnDetectorResult)(int ClientID, int State, char *pCountry, void *pServerData);

class CServer;

class CVpnDetector
{
public:
	enum
	{
		STATE_UNKOWN = 0,
		STATE_RESIDENTIAL,
		STATE_WARNING,
		STATE_BAD,
		STATE_ERROR,
		NUM_STATES,
	};

	struct CVpnRequest
	{
		int m_ClientID;// class write
		char m_aAddress[256];// class write
		int64 m_RemoveTime;// class write
		int m_ResultState;//thread write
		char m_aResultCountry[256];//thread write
		bool m_TimeLimitExceeded;//both write
	};

private:
	CServer *m_pServer;
	VpnDetectorResult *m_pResultFunction;
	int m_DetectState[MAX_CLIENTS];
	array<CVpnRequest *> m_lRequestList;

	static void VpnCheckThread(void *pData);

	void WorkStack();
	void UpdateList();
	void ResetState(int ClientID);
	void NewClient(int ClientID, char *pAddress);
	bool CheckList(int ClientID);

public:
	CVpnDetector();

	void Tick();
	void Init(CServer *pServer, VpnDetectorResult *pResultFunc);

	const char *GetVpnState(int ClientID);

	CServer *Server() const { return m_pServer; }
};