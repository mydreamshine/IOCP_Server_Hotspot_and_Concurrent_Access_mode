#pragma once

constexpr int g_client_controller_id = 0; // ù��° Ŭ���̾�Ʈ�� �ٸ� Ŭ���̾�Ʈ�� ��Ʈ�� �� �� �ִ� ������ �ź��� Ŭ���̾�Ʈ
constexpr int MAX_TEST = 1000;
constexpr int MAX_USER = 1000;
constexpr int MAX_STR_LEN = 50;

constexpr int NPC_ID_START = 1000;
constexpr int NUM_NPC = 20000;

#define WORLD_WIDTH		800
#define WORLD_HEIGHT	800

#define SERVER_PORT		3500

// HOTSPOT: �������� �ʱ���ġ���� ���� Ŭ���̾�Ʈ���� �����̵�(cs_pos) ��Ŷ ����
// CONCURRENT: �������� �ʱ���ġ���� ���� Ŭ���̾�Ʈ���� ���� �����̵�(cs_teleport) ��Ŷ ����
enum TEST_MODE
{
	NON, HOTSPOT, CONCURRENT_CONNECT // Concurrent : ���ü� (Not Parallel)
};

#define CS_UP		     1
#define CS_DOWN		     2
#define CS_LEFT		     3
#define CS_RIGHT	     4
#define CS_TELEPORT      5
#define CS_SET_TEST_MODE 6

#define SC_LOGIN_OK			   1
#define SC_PUT_PLAYER		   2
#define SC_REMOVE_PLAYER	   3
#define SC_POS				   4
#define SC_CHAT				   5
#define SC_TEST_MODE_CHANGE_OK 6

#pragma pack(push ,1)

struct sc_packet_pos {
	char size;
	char type;
	int id;
	short x, y;
};

struct sc_packet_remove_player {
	char size;
	char type;
	int id;
};

struct sc_packet_login_ok {
	char size;
	char type;
	int id;
};

struct sc_packet_put_player {
	char size;
	char type;
	int id;
	short x, y;
};

struct sc_packet_chat {
	char size;
	char type;
	int	id;
	wchar_t	message[MAX_STR_LEN];
};

struct sc_packet_test_mode_change_ok {
	char size;
	char type;
	int	id;
	TEST_MODE test_mode;
};

struct cs_packet_up {
	char	size;
	char	type;
};

struct cs_packet_down {
	char	size;
	char	type;
};

struct cs_packet_left {
	char	size;
	char	type;
};

struct cs_packet_right {
	char	size;
	char	type;
};

struct cs_packet_test_mode_chnage{
	char size;
	char type;
	TEST_MODE test_mode;
};

#pragma pack (pop)