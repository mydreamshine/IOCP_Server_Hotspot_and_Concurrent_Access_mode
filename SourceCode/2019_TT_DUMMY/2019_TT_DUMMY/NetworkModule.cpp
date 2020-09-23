#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <winsock.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <queue>
#include <array>
#include <unordered_map>

using namespace std;
using namespace chrono;

extern HWND		hWnd;

const static int INVALID_ID = -1;
const static int MAX_PACKET_SIZE = 255;
const static int MAX_BUFF_SIZE = 255;

#pragma comment (lib, "ws2_32.lib")

#include "NetworkModule.h"

HANDLE g_hiocp;

enum OPTYPE { OP_SEND, OP_RECV, OP_DO_MOVE };

high_resolution_clock::time_point last_connect_time;

struct OverlappedEx {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	unsigned char IOCP_buf[MAX_BUFF_SIZE];
	OPTYPE event_type;
	int event_target;
};

struct CLIENT {
	int id;
	int x;
	int y;
	high_resolution_clock::time_point last_move_time;
	bool connect;

	SOCKET client_socket;
	OverlappedEx recv_over;
	unsigned char packet_buf[MAX_PACKET_SIZE];
	int prev_packet_data;
	int curr_packet_size;
};

array<CLIENT, MAX_USER> g_clients;
atomic_int num_connections;

vector <thread *> worker_threads;
thread test_thread;

float point_cloud[MAX_USER * 2];

extern atomic_bool keys[256];
extern atomic_bool is_test_mode_cuncurrent_connect = ATOMIC_VAR_INIT(false);
extern BOOL done;	// Bool Variable To Exit Loop

// 나중에 NPC까지 추가 확장 용
struct ALIEN {
	int id;
	int x, y;
	int visible_count;
};

void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"에러" << lpMsgBuf << std::endl;

	MessageBox(hWnd, lpMsgBuf, L"ERROR", 0);
	LocalFree(lpMsgBuf);
	while (true);
}

void DisconnectClient(int ci)
{
	closesocket(g_clients[ci].client_socket);
	g_clients[ci].connect = false;
	cout << "Client [" << ci << "] Disconnected!\n";
}

void ProcessPacket(int ci, unsigned char packet[])
{
	switch (packet[1]) {
	case SC_POS: {
		sc_packet_pos *pos_packet = reinterpret_cast<sc_packet_pos *>(packet);
		if (INVALID_ID == g_clients[ci].id) g_clients[ci].id = ci;
		if (ci == pos_packet->id) {
			g_clients[ci].x = pos_packet->x;
			g_clients[ci].y = pos_packet->y;
		}
		} break;
	case SC_PUT_PLAYER: {
		sc_packet_put_player* put_player_packet = reinterpret_cast<sc_packet_put_player*>(packet);
		if (INVALID_ID == g_clients[ci].id) g_clients[ci].id = ci;
		if (ci == put_player_packet->id) {
			g_clients[ci].x = put_player_packet->x;
			g_clients[ci].y = put_player_packet->y;
		}
		} break;
	case SC_REMOVE_PLAYER: break;
	case SC_CHAT: break;
	case SC_LOGIN_OK: break;
	case SC_TEST_MODE_CHANGE_OK: {
		sc_packet_test_mode_change_ok* test_mode_packet = reinterpret_cast<sc_packet_test_mode_change_ok*>(packet);
		if (test_mode_packet->test_mode == TEST_MODE::CONCURRENT_CONNECT) is_test_mode_cuncurrent_connect = TRUE;
		else is_test_mode_cuncurrent_connect = FALSE;
		} break;
	default:
		MessageBox(hWnd, L"Unknown Packet Type", L"ERROR", 0);
		while (true);
	}
}

void Worker_Thread()
{
	while (true) {
		DWORD io_size = 0;
		unsigned long long ci = 0;
		OverlappedEx *over = nullptr;
		if (done) break;
		BOOL ret = GetQueuedCompletionStatus(g_hiocp, &io_size, &ci,
			reinterpret_cast<LPWSAOVERLAPPED *>(&over), INFINITE);
		if (done) break;
		// std::cout << "GQCS :";
		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			if (64 == err_no) DisconnectClient(static_cast<int>(ci));
			else if (err_no != WSA_IO_PENDING && err_no != WSAECONNRESET && err_no != WSAENOTSOCK && err_no != WSAECONNABORTED) error_display("GQCS : ", WSAGetLastError());
		}
		if (0 == io_size) {
			DisconnectClient(static_cast<int>(ci));
			continue;
		}
		if (OP_RECV == over->event_type) {
			std::cout << "RECV from Client :" << ci;
			std::cout << "  IO_SIZE : " << io_size << std::endl;
			unsigned char *buf = g_clients[ci].recv_over.IOCP_buf;
			unsigned psize = g_clients[ci].curr_packet_size;
			unsigned pr_size = g_clients[ci].prev_packet_data;
			while (io_size > 0) {
				if (0 == psize) psize = buf[0];
				if (io_size + pr_size >= psize) {
					// 지금 패킷 완성 가능
					unsigned char packet[MAX_PACKET_SIZE];
					memcpy(packet, g_clients[ci].packet_buf, pr_size);
					memcpy(packet + pr_size, buf, psize - pr_size);
					ProcessPacket(static_cast<int>(ci), packet);
					io_size -= psize - pr_size;
					buf += psize - pr_size;
					psize = 0; pr_size = 0;
				}
				else {
					memcpy(g_clients[ci].packet_buf + pr_size, buf, io_size);
					pr_size += io_size;
					io_size = 0;
				}
			}
			g_clients[ci].curr_packet_size = psize;
			g_clients[ci].prev_packet_data = pr_size;
			DWORD recv_flag = 0;
			int ret = WSARecv(g_clients[ci].client_socket,
				&g_clients[ci].recv_over.wsabuf, 1,
				NULL, &recv_flag, &g_clients[ci].recv_over.over, NULL);
			if (SOCKET_ERROR == ret) {
				int err_no = WSAGetLastError();
				if (err_no != WSA_IO_PENDING && err_no != WSAECONNRESET && err_no != WSAENOTSOCK && err_no != WSAECONNABORTED)
				{
					error_display("RECV ERROR", err_no);
				}
			}
		}
		else if (OP_SEND == over->event_type) {
			if (io_size != over->wsabuf.len) {
				std::cout << "Send Incomplete Error!\n";
				closesocket(g_clients[ci].client_socket);
				g_clients[ci].connect = false;
			}
			delete over;
		}
		else if (OP_DO_MOVE == over->event_type) {
			// Not Implemented Yet
			delete over;
		}
		else {
			std::cout << "Unknown GQCS event!\n";
			while (true);
		}
	}

	for (int i = 0; i < num_connections; ++i)
		DisconnectClient(i);
}

void Adjust_Number_Of_Client()
{
	if (num_connections >= MAX_TEST) return;
	if (high_resolution_clock::now() < last_connect_time + 100ms) return;

	g_clients[num_connections].client_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");


	int Result = WSAConnect(g_clients[num_connections].client_socket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);
	if (0 != Result) {
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING && err_no != WSAECONNRESET && err_no != WSAENOTSOCK && err_no != WSAECONNABORTED)
		{
			error_display("WSAConnect", err_no);
		}
	}

	g_clients[num_connections].curr_packet_size = 0;
	g_clients[num_connections].prev_packet_data = 0;
	ZeroMemory(&g_clients[num_connections].recv_over, sizeof(g_clients[num_connections].recv_over));
	g_clients[num_connections].recv_over.event_type = OP_RECV;
	g_clients[num_connections].recv_over.wsabuf.buf =
		reinterpret_cast<CHAR *>(g_clients[num_connections].recv_over.IOCP_buf);
	g_clients[num_connections].recv_over.wsabuf.len = sizeof(g_clients[num_connections].recv_over.IOCP_buf);

	DWORD recv_flag = 0;
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_clients[num_connections].client_socket), g_hiocp, num_connections, 0);
	int ret = WSARecv(g_clients[num_connections].client_socket, &g_clients[num_connections].recv_over.wsabuf, 1,
		NULL, &recv_flag, &g_clients[num_connections].recv_over.over, NULL);
	if (SOCKET_ERROR == ret) {
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING && err_no != WSAECONNRESET && err_no != WSAENOTSOCK && err_no != WSAECONNABORTED)
		{
			error_display("RECV ERROR", err_no);
		}
	}
	g_clients[num_connections].connect = true;
	num_connections++;
}

void SendPacket(int cl, void *packet)
{
	int psize = reinterpret_cast<unsigned char *>(packet)[0];
	int ptype = reinterpret_cast<unsigned char *>(packet)[1];
	OverlappedEx *over = new OverlappedEx;
	over->event_type = OP_SEND;
	memcpy(over->IOCP_buf, packet, psize);
	ZeroMemory(&over->over, sizeof(over->over));
	over->wsabuf.buf = reinterpret_cast<CHAR *>(over->IOCP_buf);
	over->wsabuf.len = psize;
	int ret = WSASend(g_clients[cl].client_socket, &over->wsabuf, 1, NULL, 0,
		&over->over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING && err_no != WSAECONNRESET && err_no != WSAENOTSOCK && err_no != WSAECONNABORTED)
			error_display("Error in SendPacket:", err_no);
	}
	std::cout << "Send Packet [" << ptype << "] To Client : " << cl << std::endl;
}

void set_test_mode_with_key()
{
	if (keys[0x31] == TRUE) // 키 '1'
	{
		cs_packet_test_mode_chnage test_mode_packet;
		test_mode_packet.size = sizeof(test_mode_packet);
		test_mode_packet.type = CS_SET_TEST_MODE;
		test_mode_packet.test_mode = TEST_MODE::HOTSPOT;
		SendPacket(g_client_controller_id, &test_mode_packet);
		keys[0x31] = FALSE;
	}
	else if (keys[0x32] == TRUE) // 키 '2'
	{
		cs_packet_test_mode_chnage test_mode_packet;
		test_mode_packet.size = sizeof(test_mode_packet);
		test_mode_packet.type = CS_SET_TEST_MODE;
		test_mode_packet.test_mode = TEST_MODE::CONCURRENT_CONNECT;
		SendPacket(g_client_controller_id, &test_mode_packet);
		keys[0x32] = FALSE;
	}
}

void Test_Thread()
{
	while (true) {
		if (done) break;
		Sleep(100);
		Adjust_Number_Of_Client();

		set_test_mode_with_key();

		for (int i = g_client_controller_id + 1; i < num_connections; ++i)
		{
			if (false == g_clients[i].connect) continue;
			if (g_clients[i].last_move_time + 1s > high_resolution_clock::now()) continue;
			g_clients[i].last_move_time = high_resolution_clock::now();
			cs_packet_up my_packet;
			my_packet.size = sizeof(my_packet);
			switch (rand() % 4)
			{
			case 0: my_packet.type = CS_UP; break;
			case 1: my_packet.type = CS_DOWN; break;
			case 2: my_packet.type = CS_LEFT; break;
			case 3: my_packet.type = CS_RIGHT; break;
			}
			SendPacket(i, &my_packet);
		}
	}
}

void InitializeNetwork()
{
	for (int i = 0; i < MAX_USER; ++i) {
		g_clients[i].connect = false;
		g_clients[i].id = INVALID_ID;
	}
	num_connections = 0;
	last_connect_time = high_resolution_clock::now();

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);

	for (int i = 0; i < 6; ++i)
		worker_threads.push_back(new std::thread{ Worker_Thread });

	test_thread = thread{ Test_Thread };
}

void ShutdownNetwork()
{
	test_thread.join();
	for (auto pth : worker_threads) {
		pth->join();
		delete pth;
	}

	WSACleanup();

	CloseHandle(g_hiocp);
}

void Do_Network()
{
	return;
}

void GetPointCloud(int *size, float **points)
{
	for (int i = g_client_controller_id + 1; i < num_connections; ++i) {
		point_cloud[i * 2] = static_cast<float>(g_clients[i].x);
		point_cloud[i * 2 + 1] = static_cast<float>(g_clients[i].y);
	}
	*size = num_connections;
	*points = point_cloud;
}

