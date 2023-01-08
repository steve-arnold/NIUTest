#pragma once

#include <windows.h>
#include "NIUTest_add.h"
#include <string>

#define WM_SOCKET           104
#define IDS_ABOUTBOX        101

class WinException
{
public:
	WinException (char *msg)
		: _err (GetLastError()), _msg(msg)	{}
	DWORD GetError() const { return _err; }
	char *GetMessage () const { return _msg; }
private:
	DWORD _err;
	char *_msg;
};

// out of memory handler that throws WinException
int NewHandler (size_t size);

class Controller
{
public:
	Controller(HWND hwnd);
	~Controller () {}
	void Command (HWND hwnd, int controlID, int command);
	bool SocketState(HWND hwnd, WPARAM wParam, LPARAM lParam);

private:
	bool StartServer(HWND hWnd, u_short nPort);
	bool StopServer(HWND hWnd);
	SciEdit     m_seRxData;
	Edit        m_eTxData;
	Edit        m_ePort;
	CheckBox    m_cRespond;
	Button      m_bClear;
	Button      m_bSOH;
	Button      m_bSTX;
	Button      m_bCR;
	Button      m_bLF;
	Button      m_bSend;
	Button      m_bStart;
	Button      m_bPause;
	Button      m_bExit;

	bool ServerOn;
	bool ServerPause;
	bool AutoRespond;
	void ClearDisplay();
	void UpdateDisplay(std::string *ostr);
	void EnableSend();
	void DisableSend();
	void ServerControl(HWND hwnd, int action);
};
enum {SVR_STOP, SVR_START, SVR_RESUME, SVR_PAUSE};