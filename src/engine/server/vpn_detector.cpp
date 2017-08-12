
#include <new>

#include <base/system.h>
#include <engine/server/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>

#include "vpn_detector.h"

#define XKEY "MzExOk5xeE9mdmVvZzEwNGRJVnNZdW01d0dla2ZSMW5Ec2xR"

static NETSOCKET invalid_socket = { NETTYPE_INVALID, -1, -1 };
static const char *s_aVpnStateNames[CVpnDetector::NUM_STATES] = { "Unknown", "Residential", "Warning", "Bad", "Error" };

struct CThreadFeed
{
	int m_ClientID;
	char m_aAddress[256];
	int m_Result;
};

static bool HttpRequest(char *pMethod, const char *pHost, const char *pPath, const char *pKey, char *pBuffer, int BufferSize)
{
	NETSOCKET Socket = invalid_socket;
	NETADDR HostAddress;
	char aNetBuff[1024];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&HostAddress, aAddrStr, sizeof(aAddrStr), 80);

	if (net_host_lookup(pHost, &HostAddress, NETTYPE_IPV4) != 0)
	{
		dbg_msg("VPN-Detector", "Error running host lookup");
		return false;
	}

	//Connect
	int socketID = create_http_socket();
	if (socketID < 0)
	{
		dbg_msg("VPN-Detector", "Error creating socket");
		return false;
	}

	Socket.type = NETTYPE_IPV4;
	Socket.ipv4sock = socketID;
	HostAddress.port = 80;

	if (net_tcp_connect(Socket, &HostAddress) != 0)
	{
		net_tcp_close(Socket);
		dbg_msg("VPN-Detector", "Error connecting to host");
		return false;
	}

	//net_set_non_blocking(Socket);

	str_format(aNetBuff, sizeof(aNetBuff), "%s %s HTTP/1.0\nHost: %s\nX-Key: %s\n\n", pMethod, pPath, pHost, pKey);
	net_tcp_send(Socket, aNetBuff, str_length(aNetBuff));

	int Received = net_tcp_recv(Socket, pBuffer, BufferSize - 1);
	pBuffer[Received + 1] = 0;

	return true;
}

void CVpnDetector::VpnCheckThread(void *pData)
{
	CVpnRequest *pRequest = (CVpnRequest *)pData;
	char AddressBuf[256];
	char aBuf[1024];

	str_format(AddressBuf, sizeof(AddressBuf), "/ip/%s", pRequest->m_aAddress);
	if (HttpRequest("GET", "v2.api.iphub.info", AddressBuf, XKEY, aBuf, sizeof(aBuf)) == false)
		return;
	
	if (str_find(aBuf, "RequestThrottled") != 0x0)
		pRequest->m_TimeLimitExceeded = true;
	else
	{
		pRequest->m_ResultState = STATE_ERROR;
		const char *pBlock = str_find(aBuf, "\"block\":");
		if (pBlock != 0x0)
		{
			pBlock += str_length("\"block\":");
			if (pBlock[0] == '0')
				pRequest->m_ResultState = STATE_RESIDENTIAL;
			else if (pBlock[0] == '1')
				pRequest->m_ResultState = STATE_BAD;
			else if (pBlock[0] == '2')
				pRequest->m_ResultState = STATE_WARNING;
		}
		
	}
}

CVpnDetector::CVpnDetector()
{
	mem_zero(&m_DetectState, sizeof(m_DetectState));
	m_pServer = 0x0;
}

void CVpnDetector::WorkStack()
{
	static int64 s_Time = time_get();
	if (m_lRequestList.size() <= 0)
		return;

	CVpnRequest *pRequest = m_lRequestList[0];

	if (pRequest->m_TimeLimitExceeded == true)
	{
		if (pRequest->m_RemoveTime < Server()->Tick())//new request in 5 seconds
		{
			pRequest->m_RemoveTime = Server()->Tick() + Server()->TickSpeed() * 5.0f;
			pRequest->m_TimeLimitExceeded = false;
			thread_init(VpnCheckThread, pRequest);
		}
	}
	else
	{
		bool Done = false;

		if (pRequest->m_ResultState != STATE_UNKOWN)
		{
			m_DetectState[pRequest->m_ClientID] = pRequest->m_ResultState;
			Done = true;
		}

		if (pRequest->m_RemoveTime < Server()->Tick() || Done == true)
		{
			m_lRequestList.remove_index(0);
			delete pRequest;
		}
	}
}

void CVpnDetector::UpdateList()
{
	static bool s_Online[MAX_CLIENTS] = { };

	if (g_Config.m_SvVpnDetectorActive == 0)
		return;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (Server()->ClientIngame(i) == false)
		{
			if (s_Online[i] == true)
				ResetState(i);
		}
		else
		{
			if (CheckList(i) == false)
			{
				char aAddress[128];
				Server()->GetClientAddr(i, aAddress, sizeof(aAddress));
				NewClient(i, aAddress);
			}
		}

		s_Online[i] = Server()->ClientIngame(i);
	}
}

void CVpnDetector::ResetState(int ClientID)
{
	for (int i = 0; i < m_lRequestList.size(); i++)
	{
		if (m_lRequestList[i]->m_ClientID != i)
			continue;

		CVpnRequest *pRequest = m_lRequestList[i];
		m_lRequestList.remove_index(i);
		delete pRequest;
		i--;
	}

	m_DetectState[ClientID] = STATE_UNKOWN;
}

void CVpnDetector::NewClient(int ClientID, char *pAddress)
{
	CVpnRequest *pRequest = new CVpnRequest();
	pRequest->m_ClientID = ClientID;
	str_copy(pRequest->m_aAddress, pAddress, sizeof(pRequest->m_aAddress));
	pRequest->m_ResultState = STATE_UNKOWN;
	//mem_zero(pRequest->m_aResultCountry, sizeof(pRequest->m_aResultCountry));
	pRequest->m_TimeLimitExceeded = false;
	pRequest->m_RemoveTime = Server()->Tick() + Server()->TickSpeed() * 5.0f;

	m_lRequestList.add(pRequest);
	thread_init(VpnCheckThread, pRequest);
}

bool CVpnDetector::CheckList(int ClientID)
{
	for (int i = 0; i < m_lRequestList.size(); i++)
		if (m_lRequestList[i]->m_ClientID == ClientID)
			return true;

	if (Server()->ClientIngame(ClientID) == false)
		return true;

	if(m_DetectState[ClientID] == STATE_UNKOWN)
		return false;

	return true;
}

void CVpnDetector::Tick()
{
	WorkStack();
	UpdateList();
}

void CVpnDetector::Init(CServer *pServer)
{
	m_pServer = pServer;
}

const char *CVpnDetector::VpnState(int ClientID)
{
	return s_aVpnStateNames[m_DetectState[ClientID]];
}