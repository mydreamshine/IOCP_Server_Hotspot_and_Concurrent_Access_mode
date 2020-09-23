// PROG14_1_16b.CPP - DirectInput keyboard demo

// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  
#define INITGUID
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <windows.h>   // include important windows stuff
#include <windowsx.h>
#include <stdio.h>

#include <d3d9.h>     // directX includes
#include "d3dx9tex.h"     // directX includes
#include "gpdumb1.h"
#include "..\..\2019_TT\2019_TT\protocol.h"

#pragma comment (lib, "ws2_32.lib")

// DEFINES ////////////////////////////////////////////////

#define MAX(a,b)	((a)>(b))?(a):(b)
#define	MIN(a,b)	((a)<(b))?(a):(b)

// defines for windows 
#define WINDOW_CLASS_NAME L"WINXCLASS"  // class name

#define WINDOW_WIDTH    680   // size of window
#define WINDOW_HEIGHT   730

#define	BUF_SIZE				1024
#define	WM_SOCKET				WM_USER + 1

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms = NULL);
int Game_Shutdown(void *parms = NULL);
int Game_Main(void *parms = NULL);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle = NULL; // save the window handle
HINSTANCE main_instance = NULL; // save the instance
char buffer[80];                // used to print text

								// demo globals
BOB			player;				// 플레이어 Unit
BOB			npc[NUM_NPC];      // NPC Unit
BOB         skelaton[MAX_USER];     // the other player skelaton

BITMAP_IMAGE reactor;      // the background   

BITMAP_IMAGE black_tile;
BITMAP_IMAGE white_tile;
#define TILE_WIDTH 65

#define UNIT_TEXTURE  0

SOCKET g_mysocket;
WSABUF	send_wsabuf;
char 	send_buffer[BUF_SIZE];
WSABUF	recv_wsabuf;
char	recv_buffer[BUF_SIZE];
char	packet_buffer[BUF_SIZE];
DWORD		in_packet_size = 0;
int		saved_packet_size = 0;
int		g_myid;

int		g_left_x = 0;
int     g_top_y = 0;


// FUNCTIONS //////////////////////////////////////////////
void ProcessPacket(char *ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_OK:
	{
		sc_packet_login_ok *packet = 
			reinterpret_cast<sc_packet_login_ok *>(ptr);
		g_myid = packet->id;
	}

	case SC_PUT_PLAYER:
	{
		sc_packet_put_player *my_packet = reinterpret_cast<sc_packet_put_player *>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			player.x = my_packet->x;
			player.y = my_packet->y;
			player.attr |= BOB_ATTR_VISIBLE;
		}
		else if (id < MAX_USER) {
			skelaton[id].x = my_packet->x;
			skelaton[id].y = my_packet->y;
			skelaton[id].attr |= BOB_ATTR_VISIBLE;
		}
		else {
			npc[id - NPC_ID_START].x = my_packet->x;
			npc[id - NPC_ID_START].y = my_packet->y;
			npc[id - NPC_ID_START].attr |= BOB_ATTR_VISIBLE;
		}
		break;
	}
	case SC_POS:
	{
		sc_packet_pos *my_packet = reinterpret_cast<sc_packet_pos *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			g_left_x = my_packet->x - 4;
			g_top_y = my_packet->y - 4;
			player.x = my_packet->x;
			player.y = my_packet->y;
		}
		else if (other_id < MAX_USER) {
			skelaton[other_id].x = my_packet->x;
			skelaton[other_id].y = my_packet->y;
		}
		else {
			npc[other_id - NPC_ID_START].x = my_packet->x;
			npc[other_id - NPC_ID_START].y = my_packet->y;
		}
		break;
	}

	case SC_REMOVE_PLAYER:
	{
		sc_packet_remove_player *my_packet = reinterpret_cast<sc_packet_remove_player *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			player.attr &= ~BOB_ATTR_VISIBLE;
		}
		else if (other_id < MAX_USER) {
			skelaton[other_id].attr &= ~BOB_ATTR_VISIBLE;
		}
		else {
			npc[other_id - NPC_ID_START].attr &= ~BOB_ATTR_VISIBLE;
		}
		break;
	}
	case SC_CHAT:
	{
		sc_packet_chat *my_packet = reinterpret_cast<sc_packet_chat *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			wcsncpy_s(player.message, my_packet->message, 256);
			player.message_time = GetTickCount();
		}
		else if (other_id < MAX_USER) {
			wcsncpy_s(skelaton[other_id].message, my_packet->message, 256);
			skelaton[other_id].message_time = GetTickCount();
		}
		else {
			wcsncpy_s(npc[other_id - NPC_ID_START].message, my_packet->message, 256);
			npc[other_id - NPC_ID_START].message_time = GetTickCount();
		}
		break;

	} 
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void ReadPacket(SOCKET sock)
{
	DWORD iobyte, ioflag = 0;

	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
		printf("Recv Error [%d]\n", err_code);
	}

	BYTE *ptr = reinterpret_cast<BYTE *>(recv_buffer);

	while (0 != iobyte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (iobyte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
		}
	}
}

void clienterror()
{
	exit(-1);
}

LRESULT CALLBACK WindowProc(HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	// this is the main message handler of the system
	PAINTSTRUCT	ps;		   // used in WM_PAINT
	HDC			hdc;	   // handle to a device context

						   // what is the message 
	switch (msg)
	{
	case WM_KEYDOWN: {
		int x = 0, y = 0;
		if (wparam == VK_RIGHT)	x += 1;
		if (wparam == VK_LEFT)	x -= 1;
		if (wparam == VK_UP)	y -= 1;
		if (wparam == VK_DOWN)	y += 1;
		cs_packet_up *my_packet = reinterpret_cast<cs_packet_up *>(send_buffer);
		my_packet->size = sizeof(my_packet);
		send_wsabuf.len = sizeof(my_packet);
		DWORD iobyte;
		if (0 != x) {
			if (1 == x) my_packet->type = CS_RIGHT;
			else my_packet->type = CS_LEFT;
			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
				printf("Error while sending packet [%d]", error_code);
			}
		}
		if (0 != y) {
			if (1 == y) my_packet->type = CS_DOWN;
			else my_packet->type = CS_UP;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		}


	}
					 break;
	case WM_CREATE:
	{
		// do initialization stuff here
		return(0);
	} break;

	case WM_PAINT:
	{
		// start painting
		hdc = BeginPaint(hwnd, &ps);

		// end painting
		EndPaint(hwnd, &ps);
		return(0);
	} break;

	case WM_DESTROY:
	{
		// kill the application			
		PostQuitMessage(0);
		return(0);
	} break;
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lparam)) {
			closesocket((SOCKET)wparam);
			clienterror();
			break;
		}
		switch (WSAGETSELECTEVENT(lparam)) {
		case FD_READ:
			ReadPacket((SOCKET)wparam);
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wparam);
			clienterror();
			break;
		}
	}

	default:break;

	} // end switch

	  // process any messages that we didn't take care of 
	return (DefWindowProc(hwnd, msg, wparam, lparam));

} // end WinProc

  // WINMAIN ////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE hprevinstance,
	LPSTR lpcmdline,
	int ncmdshow)
{
	// this is the winmain function

	WNDCLASS winclass;	// this will hold the class we create
	HWND	 hwnd;		// generic window handle
	MSG		 msg;		// generic message


						// first fill in the window class stucture
	winclass.style = CS_DBLCLKS | CS_OWNDC |
		CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = hinstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;

	// register the window class
	if (!RegisterClass(&winclass))
		return(0);

	// create the window, note the use of WS_POPUP
	if (!(hwnd = CreateWindow(WINDOW_CLASS_NAME, // class
		L"Chess Client",	 // title
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0,	   // x,y
		WINDOW_WIDTH,  // width
		WINDOW_HEIGHT, // height
		NULL,	   // handle to parent 
		NULL,	   // handle to menu
		hinstance,// instance
		NULL)))	// creation parms
		return(0);

	// save the window handle and instance in a global
	main_window_handle = hwnd;
	main_instance = hinstance;

	// perform all game console specific initialization
	Game_Init();

	// enter main event loop
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// test if this is a quit
			if (msg.message == WM_QUIT)
				break;

			// translate any accelerator keys
			TranslateMessage(&msg);

			// send the message to the window proc
			DispatchMessage(&msg);
		} // end if

		  // main game processing goes here
		Game_Main();

	} // end while

	  // shutdown game and release all resources
	Game_Shutdown();

	// return to Windows like this
	return(static_cast<int>(msg.wParam));

} // end WinMain

  ///////////////////////////////////////////////////////////

  // WINX GAME PROGRAMMING CONSOLE FUNCTIONS ////////////////

int Game_Init(void *parms)
{
	// this function is where you do all the initialization 
	// for your game

	// set up screen dimensions
	screen_width = WINDOW_WIDTH;
	screen_height = WINDOW_HEIGHT;
	screen_bpp = 32;

	// initialize directdraw
	DD_Init(screen_width, screen_height, screen_bpp);


	// create and load the reactor bitmap image
	Create_Bitmap32(&reactor, 0, 0, 531, 532);
	Create_Bitmap32(&black_tile, 0, 0, 531, 532);
	Create_Bitmap32(&white_tile, 0, 0, 531, 532);
	Load_Image_Bitmap32(&reactor, L"CHESSMAP.BMP", 0, 0, BITMAP_EXTRACT_MODE_ABS);
	Load_Image_Bitmap32(&black_tile, L"CHESSMAP.BMP", 0, 0, BITMAP_EXTRACT_MODE_ABS);
	black_tile.x = 69;
	black_tile.y = 5;
	black_tile.height = TILE_WIDTH;
	black_tile.width = TILE_WIDTH;
	Load_Image_Bitmap32(&white_tile, L"CHESSMAP.BMP", 0, 0, BITMAP_EXTRACT_MODE_ABS);
	white_tile.x = 5;
	white_tile.y = 5;
	white_tile.height = TILE_WIDTH;
	white_tile.width = TILE_WIDTH;

	// now let's load in all the frames for the skelaton!!!

	Load_Texture(L"CHESS2.PNG", UNIT_TEXTURE, 384, 64);

	if (!Create_BOB32(&player, 0, 0, 64, 64, 1, BOB_ATTR_SINGLE_FRAME)) return(0);
	Load_Frame_BOB32(&player, UNIT_TEXTURE, 0, 2, 0, BITMAP_EXTRACT_MODE_CELL);

	// set up stating state of skelaton
	Set_Animation_BOB32(&player, 0);
	Set_Anim_Speed_BOB32(&player, 4);
	Set_Vel_BOB32(&player, 0, 0);
	Set_Pos_BOB32(&player, 0, 0);


	// create skelaton bob
	for (int i = 0; i < MAX_USER; ++i) {
		if (!Create_BOB32(&skelaton[i], 0, 0, 64, 64, 1, BOB_ATTR_SINGLE_FRAME))
			return(0);
		Load_Frame_BOB32(&skelaton[i], UNIT_TEXTURE, 0, 0, 0, BITMAP_EXTRACT_MODE_CELL);

		// set up stating state of skelaton
		Set_Animation_BOB32(&skelaton[i], 0);
		Set_Anim_Speed_BOB32(&skelaton[i], 4);
		Set_Vel_BOB32(&skelaton[i], 0, 0);
		Set_Pos_BOB32(&skelaton[i], 0, 0);
	}

	 // create skelaton bob
	for (int i = 0; i < NUM_NPC; ++i) {
		if (!Create_BOB32(&npc[i], 0, 0, 64, 64, 1, BOB_ATTR_SINGLE_FRAME))
			return(0);
		Load_Frame_BOB32(&npc[i], UNIT_TEXTURE, 0, 4, 0, BITMAP_EXTRACT_MODE_CELL);

		// set up stating state of skelaton
		Set_Animation_BOB32(&npc[i], 0);
		Set_Anim_Speed_BOB32(&npc[i], 4);
		Set_Vel_BOB32(&npc[i], 0, 0);
		Set_Pos_BOB32(&npc[i], 0, 0);
		// Set_ID(&npc[i], i);
	}



	// set clipping rectangle to screen extents so mouse cursor
	// doens't mess up at edges
	//RECT screen_rect = {0,0,screen_width,screen_height};
	//lpddclipper = DD_Attach_Clipper(lpddsback,1,&screen_rect);

	// hide the mouse
	//ShowCursor(FALSE);


	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	WSAAsyncSelect(g_mysocket, main_window_handle, WM_SOCKET, FD_CLOSE | FD_READ);

	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;


	// return success
	return(1);

} // end Game_Init

  ///////////////////////////////////////////////////////////

int Game_Shutdown(void *parms)
{
	// this function is where you shutdown your game and
	// release all resources that you allocated

	// kill the reactor
	Destroy_Bitmap32(&black_tile);
	Destroy_Bitmap32(&white_tile);
	Destroy_Bitmap32(&reactor);

	// kill skelaton
	for (int i = 0; i < MAX_USER; ++i) Destroy_BOB32(&skelaton[i]);
	//for (int i = 0; i < MAX_NPC; ++i)
	//	Destroy_BOB32(&npc[i]);

	// shutdonw directdraw
	DD_Shutdown();

	WSACleanup();

	// return success
	return(1);
} // end Game_Shutdown

  ///////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
	// this is the workhorse of your game it will be called
	// continuously in real-time this is like main() in C
	// all the calls for you game go here!
	// check of user is trying to exit
	if (KEY_DOWN(VK_ESCAPE) || KEY_DOWN(VK_SPACE))
		PostMessage(main_window_handle, WM_DESTROY, 0, 0);

	// start the timing clock
	Start_Clock();

	// clear the drawing surface
	DD_Fill_Surface(D3DCOLOR_ARGB(255, 0, 0, 0));

	// get player input

	g_pd3dDevice->BeginScene();
	g_pSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);

	// draw the background reactor image
	for (int i = 0; i<10; ++i)
		for (int j = 0; j<10; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x <0) || (tile_y<0)) continue;
			if (((tile_x >> 2) % 2) == ((tile_y >> 2) % 2))
				Draw_Bitmap32(&white_tile, TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
			else
				Draw_Bitmap32(&black_tile, TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
		}
	//	Draw_Bitmap32(&reactor);

	g_pSprite->End();
	g_pSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);


	// draw the skelaton
	Draw_BOB32(&player);
	for (int i = 0; i<MAX_USER; ++i) Draw_BOB32(&skelaton[i]);
	for (int i = 0; i<NUM_NPC; ++i) Draw_BOB32(&npc[i]);

	// draw some text
	wchar_t text[300];
	wsprintf(text, L"MY POSITION (%3d, %3d)", player.x, player.y);
	Draw_Text_D3D(text, 10, screen_height - 64, D3DCOLOR_ARGB(255, 255, 255, 255));

	g_pSprite->End();
	g_pd3dDevice->EndScene();

	// flip the surfaces
	DD_Flip();

	// sync to 3o fps
	//Wait_Clock(30);


	// return success
	return(1);

} // end Game_Main

  //////////////////////////////////////////////////////////