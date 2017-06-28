/*	2017�� 1�б� ��Ʈ��ũ���α׷��� ���� 2��
     ����: ������
     �й�: 122021 	*/

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <ws2tcpip.h>
#include <time.h>

#define BUFSIZE    512
#define MULTICASTIP "235.4.3.2"
#define REMOTEPORT  9000

//����� �����Ǵ� 224.0.0.0 �� 239.255.255.255 ������ �����Ǹ� �����մϴ�

IN_ADDR GetDefaultMyIP();
// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags);
// ���� ��� ������ �Լ�
DWORD WINAPI Sender(LPVOID arg);
DWORD WINAPI Receiver(LPVOID arg);
bool ip_check(char *ip);
bool port_check(int port);
char mulip[15];
char port[15];
static u_short mPort; // ���� ��Ʈ ��ȣ

SOCKET sock; // ����
char buf[BUFSIZE+1]; // ������ �ۼ��� ����
char name[BUFSIZE+1];	//	nickname
char tempname[BUFSIZE+1]="NONE";
HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HWND hNameButton, hSendButton, hNickButton, hConnectButton; // ������ ��ư
HWND hEdit1, hEdit2, hEdit3, hEdit4, hEdit5; // ���� ��Ʈ��

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// ��ȭ���� ���ν���
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char tbuffer[20];
	HANDLE hThread;

	switch(uMsg){
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hEdit3 = GetDlgItem(hDlg, IDC_EDIT3);
		hEdit4 = GetDlgItem(hDlg, IDC_ADDR);
		hEdit5 = GetDlgItem(hDlg, IDC_PORT);
		SetDlgItemText(hDlg, IDC_ADDR, MULTICASTIP);//ip�ּ��ʱⰪ ����
		SetDlgItemInt(hDlg, IDC_PORT, REMOTEPORT,FALSE);//��Ʈ��ȣ �ʱⰪ����
		hConnectButton = GetDlgItem(hDlg, IDC_CONNECT);
		hSendButton = GetDlgItem(hDlg, IDOK);
		hNickButton = GetDlgItem(hDlg, IDC_NICK);
		SendMessage(hEdit3, EM_SETLIMITTEXT, BUFSIZE, 0);
		DisplayText("���� �ּҿ� ��Ʈ��ȣ, ���̵� �����ϼ���.\r\n");
		EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
		EnableWindow(hNickButton, FALSE); // �г��� ��Ȱ��ȭ
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam)){

		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_ADDR, mulip, 16);
			GetDlgItemText(hDlg, IDC_PORT, port, 16);
			mPort = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);

			if(ip_check(mulip) == false)
			{
				DisplayText("�߸��� �ּ� �Է�.\r\n");
				EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			}
			else if(port_check(atoi(port)) == false)
			{
				DisplayText("�߸��� ��Ʈ��ȣ �Է�.\r\n");
				EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			}
			else
			{
				DisplayText("���� ��Ʈ��ȣ : %d\r\n",mPort);
				MessageBox(hDlg, "���ӵǾ����ϴ�.", "����", MB_ICONINFORMATION);
				EnableWindow(hConnectButton, FALSE); // ���� ��ư ��Ȱ��ȭ
				EnableWindow(hEdit4, FALSE); // �����ּ� ��Ȱ��ȭ
				EnableWindow(hEdit5, FALSE); // ��Ʈ��ȣ ��Ȱ��ȭ
				EnableWindow(hNickButton, TRUE);	//	�г��� Ȱ��ȭ

				//���� ������ ����
				hThread = CreateThread(NULL, 0, Receiver, (LPVOID)sock, 0, NULL);

				//�۽� ������ ����
				hThread = CreateThread(NULL, 0, Sender, (LPVOID)sock, 0, NULL);

				if(hThread == NULL) { closesocket(sock); }
				else { CloseHandle(hThread); }

			}
			return TRUE;


		case IDC_NICK:
			GetDlgItemText(hDlg, IDC_EDIT1, name, BUFSIZE+1);
			_strtime( tbuffer );
			if(strcmp(tempname,"NONE") == 0)
			{
			DisplayText("���̵� %s���� �����Ǿ����ϴ�.\r\n������ �ð�:%s\r\n", name, tbuffer);
			strcpy(tempname,name);

			}

			else if(strcmp(tempname, name) != 0)
			{
				_strtime( tbuffer );
				DisplayText("���̵� %s���� %s�� ����Ǿ����ϴ�.\r\n������ �ð�:%s\r\n", tempname, name, tbuffer);
				strcpy(tempname,name);
			}
			
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			return TRUE;
			

		case IDOK:
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT1, name, BUFSIZE+1);

			if(strcmp(tempname,name) != 0)
			{
				_strtime( tbuffer );
				DisplayText("���̵� %s���� %s�� ����Ǿ����ϴ�.\r\n������ �ð�:%s\r\n", tempname, name, tbuffer);
				strcpy(tempname,name);
			}
			GetDlgItemText(hDlg, IDC_EDIT3, buf, BUFSIZE+1);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
			SetFocus(hEdit3);
			SendMessage(hEdit3, EM_SETSEL, 0, -1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI Receiver(LPVOID arg)
{
	int retval;
	char tbuffer[9]; //	�ð�
	char addr[20];
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");	

	// SO_REUSEADDR �ɼ� ����
	BOOL optval = TRUE;
	retval = setsockopt(sock, SOL_SOCKET,
		SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	// bind()
	SOCKADDR_IN localaddr;
	ZeroMemory(&localaddr, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(mPort);
	retval = bind(sock, (SOCKADDR *)&localaddr, sizeof(localaddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");
	
	// ��Ƽĳ��Ʈ �׷� ����
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(mulip);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	retval = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char *)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");	//	Ư�� ��Ƽĳ��Ʈ�ּҿ� ���Խ�Ŵ

	// ������ ��ſ� ����� ����
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE+1];

	// ��Ƽĳ��Ʈ ������ �ޱ�
	while(1){
	// ������ �ޱ�
		addrlen = sizeof(clientaddr);

		recvfrom(sock, name, BUFSIZE, 0, 
			(SOCKADDR *)&clientaddr, &addrlen);
		if(retval == SOCKET_ERROR){
			err_display("recvfrom()");
			continue;
		}

		recvfrom(sock, buf, BUFSIZE, 0, 
			(SOCKADDR *)&clientaddr, &addrlen);
		if(retval == SOCKET_ERROR){
			err_display("recvfrom()");
			continue;
		}

		recvfrom(sock, addr, 16, 0, 
			(SOCKADDR *)&clientaddr, &addrlen);
		if(retval == SOCKET_ERROR){
			err_display("recvfrom()");
			continue;
		}

		//_strtime( tbuffer );
		recvfrom(sock, tbuffer, 9, 0, 
			(SOCKADDR *)&clientaddr, &addrlen);
		if(retval == SOCKET_ERROR){
			err_display("recvfrom()");
			continue;
		}

		DisplayText("[%s] %s\r\n�۽��� IP : %s  ,  ���� �ð� : %s\r\n", name, buf, addr, tbuffer);

		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
}




	// ��Ƽĳ��Ʈ �׷� Ż��
	retval = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(char *)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}


DWORD WINAPI Sender(LPVOID arg)
{
	
	int retval;
	char tbuffer[9]; //	�ð�


	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	IN_ADDR sender_addr = GetDefaultMyIP();//����Ʈ IPv4 �ּ� ������
	char addr[20];
	strcpy(addr, inet_ntoa(sender_addr));

	// ��Ƽĳ��Ʈ TTL ����
	int ttl = 2;
	retval = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
		(char *)&ttl, sizeof(ttl));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	// ���� �ּ� ����ü �ʱ�ȭ
	SOCKADDR_IN remoteaddr;
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(mulip);
	remoteaddr.sin_port = htons(mPort);

	// ������ ��ſ� ����� ����
	int addrlen;
	char sendbuf[BUFSIZE+1];
	int len;
	HANDLE hThread;

	

	// ��Ƽĳ��Ʈ ������ ������
	while(1){
		//DisplayText("���� ��� : %s\r\n", name);
		WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
			continue;
		}

		// �̸� ������
		retval = sendto(sock, name, BUFSIZE, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// ������ ������
		retval = sendto(sock, buf, BUFSIZE, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// ���� IP
		retval = sendto(sock, addr, 16, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// �ð�
		_strtime( tbuffer );
		retval = sendto(sock, tbuffer, 9, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
		
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���

	}

	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}


// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

IN_ADDR GetDefaultMyIP() 
{
    char localhostname[MAX_PATH];
    IN_ADDR addr={0,};
 
    if(gethostname(localhostname, MAX_PATH) == SOCKET_ERROR)//ȣ��Ʈ �̸� ������
    {
        return addr; 
    }
    HOSTENT *ptr = gethostbyname(localhostname);//ȣ��Ʈ ��Ʈ�� ������
    while(ptr && ptr->h_name)
    {
        if(ptr->h_addrtype == PF_INET)//IPv4 �ּ� Ÿ���� ��
        {
            memcpy(&addr, ptr->h_addr_list[0], ptr->h_length);//�޸� ����
            break;//�ݺ��� Ż��
        }
        ptr++;
    }
    return addr;
}

bool ip_check(char *ip)
{
	//����� �����Ǵ� 224.0.0.0 �� 239.255.255.255 ������ �����Ǹ� �����մϴ�

    if(strcmp(ip, " ") == 0)
    {
        return false;
    }

    int len = strlen(ip);

    if( len > 15 || len < 9 )
        return false;

    int nNumCount = 0;
    int nDotCount = 0;
    int i = 0;

	if(ip[0] != '2' || (ip[1] != '3' && ip[1] != '2')) {
		return false;
	}


	if(ip[1] == '2') {
		if(ip[2] < '4')	
			return false;
	}

	for(i=0;i<len;i++)
	{
	

		if(ip[i] < '0' || ip[i] > '9')
        {
            if(ip[i] == '.')
            {
                ++nDotCount;
                nNumCount = 0;
            }
            else
                return false;
        }
        else
        {
            if(++nNumCount > 3)
                return false;
        }

	}

    if(nDotCount != 3)
        return false;

    return true;
}

bool port_check(int port)
{
	//1~1024 ����, 65535���� ����


	if(1 > port || (0 < port && port < 1025) )
		return false;

	else if(port > 65535)
		return false;

    return true;
}