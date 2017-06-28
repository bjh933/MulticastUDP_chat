/*	2017년 1학기 네트워크프로그래밍 숙제 2번
     성명: 변지훈
     학번: 122021 	*/

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

//사용할 아이피는 224.0.0.0 와 239.255.255.255 사이의 아이피를 선택합니다

IN_ADDR GetDefaultMyIP();
// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags);
// 소켓 통신 스레드 함수
DWORD WINAPI Sender(LPVOID arg);
DWORD WINAPI Receiver(LPVOID arg);
bool ip_check(char *ip);
bool port_check(int port);
char mulip[15];
char port[15];
static u_short mPort; // 서버 포트 번호

SOCKET sock; // 소켓
char buf[BUFSIZE+1]; // 데이터 송수신 버퍼
char name[BUFSIZE+1];	//	nickname
char tempname[BUFSIZE+1]="NONE";
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hNameButton, hSendButton, hNickButton, hConnectButton; // 보내기 버튼
HWND hEdit1, hEdit2, hEdit3, hEdit4, hEdit5; // 편집 컨트롤

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// 대화상자 프로시저
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
		SetDlgItemText(hDlg, IDC_ADDR, MULTICASTIP);//ip주소초기값 설정
		SetDlgItemInt(hDlg, IDC_PORT, REMOTEPORT,FALSE);//포트번호 초기값설정
		hConnectButton = GetDlgItem(hDlg, IDC_CONNECT);
		hSendButton = GetDlgItem(hDlg, IDOK);
		hNickButton = GetDlgItem(hDlg, IDC_NICK);
		SendMessage(hEdit3, EM_SETLIMITTEXT, BUFSIZE, 0);
		DisplayText("서버 주소와 포트번호, 아이디를 설정하세요.\r\n");
		EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
		EnableWindow(hNickButton, FALSE); // 닉네임 비활성화
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam)){

		case IDC_CONNECT:
			GetDlgItemText(hDlg, IDC_ADDR, mulip, 16);
			GetDlgItemText(hDlg, IDC_PORT, port, 16);
			mPort = GetDlgItemInt(hDlg, IDC_PORT, NULL, FALSE);

			if(ip_check(mulip) == false)
			{
				DisplayText("잘못된 주소 입력.\r\n");
				EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			}
			else if(port_check(atoi(port)) == false)
			{
				DisplayText("잘못된 포트번호 입력.\r\n");
				EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			}
			else
			{
				DisplayText("연결 포트번호 : %d\r\n",mPort);
				MessageBox(hDlg, "접속되었습니다.", "접속", MB_ICONINFORMATION);
				EnableWindow(hConnectButton, FALSE); // 접속 버튼 비활성화
				EnableWindow(hEdit4, FALSE); // 서버주소 비활성화
				EnableWindow(hEdit5, FALSE); // 포트번호 비활성화
				EnableWindow(hNickButton, TRUE);	//	닉네임 활성화

				//수신 스레드 생성
				hThread = CreateThread(NULL, 0, Receiver, (LPVOID)sock, 0, NULL);

				//송신 스레드 생성
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
			DisplayText("아이디가 %s으로 설정되었습니다.\r\n설정한 시간:%s\r\n", name, tbuffer);
			strcpy(tempname,name);

			}

			else if(strcmp(tempname, name) != 0)
			{
				_strtime( tbuffer );
				DisplayText("아이디가 %s에서 %s로 변경되었습니다.\r\n변경한 시간:%s\r\n", tempname, name, tbuffer);
				strcpy(tempname,name);
			}
			
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			return TRUE;
			

		case IDOK:
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT1, name, BUFSIZE+1);

			if(strcmp(tempname,name) != 0)
			{
				_strtime( tbuffer );
				DisplayText("아이디가 %s에서 %s로 변경되었습니다.\r\n변경한 시간:%s\r\n", tempname, name, tbuffer);
				strcpy(tempname,name);
			}
			GetDlgItemText(hDlg, IDC_EDIT3, buf, BUFSIZE+1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
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

// 편집 컨트롤 출력 함수
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

// 클라이언트와 데이터 통신
DWORD WINAPI Receiver(LPVOID arg)
{
	int retval;
	char tbuffer[9]; //	시간
	char addr[20];
	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");	

	// SO_REUSEADDR 옵션 설정
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
	
	// 멀티캐스트 그룹 가입
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(mulip);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	retval = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char *)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");	//	특정 멀티캐스트주소에 가입시킴

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE+1];

	// 멀티캐스트 데이터 받기
	while(1){
	// 데이터 받기
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

		DisplayText("[%s] %s\r\n송신자 IP : %s  ,  보낸 시간 : %s\r\n", name, buf, addr, tbuffer);

		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기
}




	// 멀티캐스트 그룹 탈퇴
	retval = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(char *)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}


DWORD WINAPI Sender(LPVOID arg)
{
	
	int retval;
	char tbuffer[9]; //	시간


	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	IN_ADDR sender_addr = GetDefaultMyIP();//디폴트 IPv4 주소 얻어오기
	char addr[20];
	strcpy(addr, inet_ntoa(sender_addr));

	// 멀티캐스트 TTL 설정
	int ttl = 2;
	retval = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
		(char *)&ttl, sizeof(ttl));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	// 소켓 주소 구조체 초기화
	SOCKADDR_IN remoteaddr;
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(mulip);
	remoteaddr.sin_port = htons(mPort);

	// 데이터 통신에 사용할 변수
	int addrlen;
	char sendbuf[BUFSIZE+1];
	int len;
	HANDLE hThread;

	

	// 멀티캐스트 데이터 보내기
	while(1){
		//DisplayText("보낸 사람 : %s\r\n", name);
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

		// 문자열 길이가 0이면 보내지 않음
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
			continue;
		}

		// 이름 보내기
		retval = sendto(sock, name, BUFSIZE, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// 데이터 보내기
		retval = sendto(sock, buf, BUFSIZE, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// 보낸 IP
		retval = sendto(sock, addr, 16, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}

		// 시간
		_strtime( tbuffer );
		retval = sendto(sock, tbuffer, 9, 0,(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
		if(retval == SOCKET_ERROR){
			err_display("send()");
			break;
		}
		
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기

	}

	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}


// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

// 사용자 정의 데이터 수신 함수
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
 
    if(gethostname(localhostname, MAX_PATH) == SOCKET_ERROR)//호스트 이름 얻어오기
    {
        return addr; 
    }
    HOSTENT *ptr = gethostbyname(localhostname);//호스트 엔트리 얻어오기
    while(ptr && ptr->h_name)
    {
        if(ptr->h_addrtype == PF_INET)//IPv4 주소 타입일 때
        {
            memcpy(&addr, ptr->h_addr_list[0], ptr->h_length);//메모리 복사
            break;//반복문 탈출
        }
        ptr++;
    }
    return addr;
}

bool ip_check(char *ip)
{
	//사용할 아이피는 224.0.0.0 와 239.255.255.255 사이의 아이피를 선택합니다

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
	//1~1024 금지, 65535까지 가능


	if(1 > port || (0 < port && port < 1025) )
		return false;

	else if(port > 65535)
		return false;

    return true;
}