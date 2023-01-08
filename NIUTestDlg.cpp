#include "stdafx.h"

HANDLE hIcoRed, hIcoAmber, hIcoGreen, hIcoBlue;
SOCKET Socket = NULL;
static u_short nPort = 33110;
sockaddr sockAddrClient;

using namespace std;

HINSTANCE hInst = 0;
static char appAboutName[] = "About NIUTest";

int NewHandler(size_t size)
{
	UNREFERENCED_PARAMETER(size);
	throw WinException("Out of memory");
}

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	hInst = hInstance;
	_set_new_handler(&NewHandler);

	HWND hDialog = 0;

	hDialog = CreateDialog(hInst, MAKEINTRESOURCE(DLG_MAIN), 0, static_cast<DLGPROC>(DialogProc));
	if (!hDialog)
	{
		char buf[100];
		wsprintf(buf, "Error x%x", GetLastError());
		MessageBox(0, buf, "CreateDialog", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	// Attach icon to main dialog
	const HANDLE hbicon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MAINFRAME),
		IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	// load big icon
	if (hbicon)
		SendMessage(hDialog, WM_SETICON, ICON_BIG, LPARAM(hbicon));

	// load small icon
	const HANDLE hsicon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MAINFRAME),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	if (hsicon)
		SendMessage(hDialog, WM_SETICON, ICON_SMALL, LPARAM(hsicon));
	// load dialog icons
	hIcoRed = LoadImage(hInst, MAKEINTRESOURCE(IDB_LIGHT_RED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
	hIcoAmber = LoadImage(hInst, MAKEINTRESOURCE(IDB_LIGHT_AMBER), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
	hIcoGreen = LoadImage(hInst, MAKEINTRESOURCE(IDB_LIGHT_GREEN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);
	hIcoBlue = LoadImage(hInst, MAKEINTRESOURCE(IDB_LIGHT_BLUE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_SHARED);

	MSG  msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		// only handle non-dialog messages here
		if (!IsDialogMessage(hDialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

Controller::Controller(HWND hwnd)
	: m_seRxData(hwnd, 0, FALSE),
	m_eTxData(hwnd, IDC_TRANSMIT, FALSE),
	m_ePort(hwnd, IDC_EDIT_PORT),
	m_bClear(hwnd, IDC_CLEAR, TRUE),
	m_bSend(hwnd, IDC_SEND, FALSE),
	m_bSOH(hwnd, IDC_SOH, FALSE),
	m_bSTX(hwnd, IDC_STX, FALSE),
	m_bCR(hwnd, IDC_CR, FALSE),
	m_bLF(hwnd, IDC_LF, FALSE),
	m_cRespond(hwnd, IDC_RESPOND),
	m_bStart(hwnd, IDC_START),
	m_bPause(hwnd, IDC_PAUSE, FALSE),
	m_bExit(hwnd, IDOK)
{
	// Other initializations...
	m_cRespond.Check();
	ServerOn = false;
	ServerPause = false;
	AutoRespond = true;
	// use OEM font as it displays codes below ascii 32 (smiley faces for stx and soh)
	HGDIOBJ hfSysFixed = GetStockObject(OEM_FIXED_FONT);
	m_ePort.SetFont(hfSysFixed);
	m_eTxData.SetFont(hfSysFixed);
	m_eTxData.SetString("");
}

void Controller::Command(HWND hwnd, int controlID, int command)
{
	switch (controlID)
	{
	case IDC_CLEAR:
		ClearDisplay();
		break;
	case IDC_RESPOND:
		if (command == BN_CLICKED)
		{
			AutoRespond = !AutoRespond;
			AutoRespond ? m_cRespond.Check() : m_cRespond.UnCheck();
		}
		break;
	case IDC_START:
		if (command == BN_CLICKED)
			if (ServerOn)
			{
				ServerControl(hwnd, SVR_STOP);
				StopServer(hwnd);
			}
			else
			{
				//TODO: test for valid port number
				BOOL bSuccess;
				int nTimes = GetDlgItemInt(hwnd, IDC_EDIT_PORT, &bSuccess, FALSE);
				u_short portno = nPort;
				if (nTimes && nTimes < 65536)
					portno = static_cast<u_short>(nTimes);
				SetDlgItemInt(hwnd, IDC_EDIT_PORT, portno, FALSE);
				if (StartServer(hwnd, portno))
				{
					ServerControl(hwnd, SVR_START);
				}
			}
		break;
	case IDC_PAUSE:
		if (command == BN_CLICKED)
			if (ServerOn)
			{
				if (ServerPause)
				{
					ServerControl(hwnd, SVR_RESUME);
				}
				else
				{
					ServerControl(hwnd, SVR_PAUSE);
				}
			}
		break;
	case IDC_SOH:
		m_eTxData.AddString("\x1");
		break;
	case IDC_STX:
		m_eTxData.AddString("\x2");
		break;
	case IDC_CR:
		m_eTxData.AddString("\x0d");
		break;
	case IDC_LF:
		m_eTxData.AddString("\x0a");
		break;
	case IDC_SEND:
		char szBuffer[1024];
		int len;
		ZeroMemory(szBuffer, sizeof(szBuffer));
		len = m_eTxData.GetLength();
		if (len)
		{
			m_eTxData.GetString(szBuffer, sizeof(szBuffer));
			string ostr = string(szBuffer, len) + string("\r\n");
			send(Socket, ostr.c_str(), (int)ostr.length(), 0);
			// prepare string for data display
			m_eTxData.SetString("");
			if (!ServerPause)
			{
				ostr.insert(0, "Out -> ");
				UpdateDisplay(&ostr);
			}
		}
		break;
	case IDOK:
		if (command == BN_CLICKED)
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		break;
	}
}

bool Controller::SocketState(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_READ:
	{
		char szIncoming[1024];
		ZeroMemory(szIncoming, sizeof(szIncoming));

		int inDataLength = recv(Socket, szIncoming, 1024, 0);

		// create the output string
		if (!ServerPause)
		{
			string ostr(szIncoming, inDataLength);
			ostr.insert(0, "In  <- ");
			UpdateDisplay(&ostr);
			ostr.clear();  // clear the string
		}
		if (AutoRespond)
		{
			if (atoi(&szIncoming[5]) != 93)      // don't acknowledge an acknowledge!
			{
				// Send SOH<tag number>STX 93 CR LF to acknowledge
				int nLength = 5;
				string ostr = string(szIncoming, nLength) + string("93\r\n");
				send(Socket, ostr.c_str(), (int)ostr.length(), 0);
				if (!ServerPause)
				{
					ostr.insert(0, "Out -> ");
					UpdateDisplay(&ostr);
				}
			}
		}
	}
	break;
	case FD_CLOSE:
	{
		HWND Hwnd = GetDlgItem(hwnd, IDC_STATUS);
		SendMessage(Hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcoAmber);
		m_bPause.SetName("Pause");
		ServerPause = false;
		m_bPause.Enable(FALSE);
		closesocket(Socket);
		DisableSend();
	}
	break;
	case FD_ACCEPT:
	{
		int size = sizeof(sockaddr);
		Socket = accept(wParam, &sockAddrClient, &size);
		if (Socket == INVALID_SOCKET)
		{
			//			int nret = WSAGetLastError();
			WSACleanup();
		}
		HWND Hwnd = GetDlgItem(hwnd, IDC_STATUS);
		SendMessage(Hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcoGreen);
		m_bPause.Enable(TRUE);
		EnableSend();
	}
	break;
	}
	return TRUE;
}

bool Controller::StopServer(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	if (Socket)
	{
		shutdown(Socket, SD_BOTH);
		closesocket(Socket);
	}
	WSACleanup();
	return TRUE;
}

bool Controller::StartServer(HWND hwnd, u_short nSPort)
{
	WSADATA WsaDat;
	int nResult = WSAStartup(MAKEWORD(2, 2), &WsaDat);
	if (nResult != 0)
	{
		MessageBox(hwnd, "Winsock initialization failed", "Critical Error", MB_ICONERROR);
		SendMessage(hwnd, WM_DESTROY, NULL, NULL);
		return false;
	}
	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Socket == INVALID_SOCKET)
	{
		MessageBox(hwnd, "Socket creation failed", "Critical Error", MB_ICONERROR);
		SendMessage(hwnd, WM_DESTROY, NULL, NULL);
		return false;
	}
	SOCKADDR_IN SockAddr{ 0 };
	SockAddr.sin_port = htons(nSPort);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(Socket, (LPSOCKADDR)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR)
	{
		MessageBox(hwnd, "Unable to bind socket", "Error", MB_OK);
		SendMessage(hwnd, WM_DESTROY, NULL, NULL);
		return false;
	}
	nResult = WSAAsyncSelect(Socket, hwnd, WM_SOCKET, (FD_CLOSE | FD_ACCEPT | FD_READ));
	if (nResult)
	{
		MessageBox(hwnd, "WSAAsyncSelect failed", "Critical Error", MB_ICONERROR);
		SendMessage(hwnd, WM_DESTROY, NULL, NULL);
		return false;
	}
	if (listen(Socket, (1)) == SOCKET_ERROR)
	{
		MessageBox(hwnd, "Unable to listen!", "Error", MB_OK);
		SendMessage(hwnd, WM_DESTROY, NULL, NULL);
		return false;
	}
	return true;
}

void Controller::EnableSend()
{
	m_eTxData.Enable(TRUE);
	m_bSend.Enable(TRUE);
	m_bSTX.Enable(TRUE);
	m_bSOH.Enable(TRUE);
	m_bCR.Enable(TRUE);
	m_bLF.Enable(TRUE);
}

void Controller::DisableSend()
{
	m_eTxData.Enable(FALSE);
	m_bSend.Enable(FALSE);
	m_bSTX.Enable(FALSE);
	m_bSOH.Enable(FALSE);
	m_bCR.Enable(FALSE);
	m_bLF.Enable(FALSE);
}

void Controller::ServerControl(HWND hwnd, int action)
{
	HWND Hwnd = GetDlgItem(hwnd, IDC_STATUS);
	LPARAM icon = 0;
	switch (action)
	{
	case SVR_START:
		icon = (LPARAM)hIcoAmber;
		m_ePort.Enable(FALSE);
		m_bStart.SetName("Stop");
		ServerOn = true;
		break;
	case SVR_PAUSE:
		icon = (LPARAM)hIcoBlue;
		m_bPause.SetName("Resume");
		ServerPause = true;
		DisableSend();
		break;
	case SVR_RESUME:
		icon = (LPARAM)hIcoGreen;
		m_bPause.SetName("Pause");
		ServerPause = false;
		EnableSend();
		break;
	case SVR_STOP:
		icon = (LPARAM)hIcoRed;
		m_ePort.Enable(TRUE);
		DisableSend();
		m_bStart.SetName("Start");
		ServerOn = false;
		break;
	}
	SendMessage(Hwnd, STM_SETIMAGE, IMAGE_ICON, icon);
}

void Controller::ClearDisplay()
{
	m_seRxData.SendEditor(SCI_CLEARALL);
	m_seRxData.Enable(FALSE);
}

void Controller::UpdateDisplay(std::string* ostr)
{
	m_seRxData.SendEditor(SCI_APPENDTEXT, ostr->length(), (LPARAM)ostr->c_str());
	m_seRxData.SendEditor(SCI_SCROLLCARET);
	m_seRxData.Enable(TRUE);
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static Controller* control = 0;
	switch (message)
	{
	case WM_INITDIALOG:
		try
		{
			control = new Controller(hwnd);
			HMENU hSysMenu;
			hSysMenu = GetSystemMenu(hwnd, FALSE);
			// add a system menu separator
			AppendMenu(hSysMenu, MF_SEPARATOR, NULL, NULL);
			// add a system menu item
			AppendMenu(hSysMenu, MF_STRING, IDS_ABOUTBOX, appAboutName);

			UpdateWindow(hwnd);
		}
		catch (WinException e)
		{
			MessageBox(0, e.GetMessage(), "Exception", MB_ICONEXCLAMATION | MB_OK);
		}
		catch (...)
		{
			MessageBox(0, "Unknown", "Exception", MB_ICONEXCLAMATION | MB_OK);
			return -1;
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		// control class handles all dialog controls
		if (control)
			control->Command(hwnd, LOWORD(wParam), HIWORD(wParam));
		return (INT_PTR)TRUE;
		// control class handles all socket messages
	case WM_SOCKET:
		if (control)
			control->SocketState(hwnd, wParam, lParam);
		return (INT_PTR)TRUE;
	case WM_DESTROY:
		if (Socket)
		{
			shutdown(Socket, SD_BOTH);
			closesocket(Socket);
		}
		WSACleanup();
		PostQuitMessage(0);
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		if (control)
			delete control;
		DestroyWindow(hwnd);
		return (INT_PTR)TRUE;
	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case IDS_ABOUTBOX:
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
			break;
		}
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
