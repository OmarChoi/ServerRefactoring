#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <fstream>
#include <string>
#include <deque>
using namespace std;

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

#include"../../Server_TermProject/Server_TermProject/protocol_2023.h"

sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

deque<sf::Text> m_world_chat;
chrono::system_clock::time_point m_chatmess_end_time;
int g_MapData[W_WIDTH][W_HEIGHT] = { 1, };

void ReadMap()
{
	std::cout << "Map intialize begin.\n";
	ifstream mapFile("content/map.txt");

	int x = 0, y = 0;
	char temp = ' ';
	while (!mapFile.eof())
	{
		mapFile >> temp;
		g_MapData[x++][y] = (int)(temp - 48);
		if (x == W_WIDTH)
		{
			y += 1;
			x = 0;
		}
	}
	std::cout << "Map intialize end.\n";
}

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;
public:
	int id;
	int m_x, m_y;
	int m_hp, m_maxHP;
	int m_level, m_exp;
	char name[NAME_SIZE];
	int m_chatLine{ -1 };
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw();
	void set_name(const char str[]) {
		strcpy_s(name, str);
		m_name.setFont(g_font);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}

	void set_battle_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 0, 0));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(1);
	}

	void set_chat(const char str[]) {
		sf::Text tempText;
		tempText.setFont(g_font);
		tempText.setString(str);
		tempText.setFillColor(sf::Color(255, 255, 255));
		tempText.setStyle(sf::Text::Regular);
		m_world_chat.push_back(tempText);
		if (m_world_chat.size() > 5)
		{
			m_world_chat.pop_front();
		}
		m_chatmess_end_time = chrono::system_clock::now() + chrono::seconds(5);
	}

};

OBJECT avatar;
OBJECT obst;
unordered_map <int, OBJECT> players;

OBJECT a_tile;
OBJECT b_tile;
OBJECT monst;

sf::Texture* player;
sf::Texture* monster;
sf::Texture* obstacle;
sf::Texture* tile1;
sf::Texture* tile2;

void OBJECT::draw() {
	if (false == m_showing) return;
	float rx = (m_x - g_left_x) * 65.0f + 1;
	float ry = (m_y - g_top_y) * 65.0f + 1;
	m_sprite.setPosition(rx, ry);
	g_window->draw(m_sprite);
	auto size = m_name.getGlobalBounds();
	if (m_mess_end_time < chrono::system_clock::now()) {
		m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
		g_window->draw(m_name);
	}
	else {
		m_chat.setPosition(rx, ry - 10);
		g_window->draw(m_chat);
	}

	if (m_chatmess_end_time > chrono::system_clock::now()) {
		int i = 0;
		for (auto& t : m_world_chat)
		{
			t.setPosition((avatar.m_x - g_left_x - SCREEN_WIDTH / 2) * 65.0f + 1,
				((avatar.m_y - g_top_y) * 65.0f + 1) - 20 * i++);
			g_window->draw(t);
		}
	}
	else
	{
		m_world_chat.clear();
	}

}

void client_initialize()
{
	obstacle = new sf::Texture;
	tile1 = new sf::Texture;
	tile2 = new sf::Texture;
	player = new sf::Texture;
	monster = new sf::Texture;

	obstacle->loadFromFile("content/obstacle.png");
	player->loadFromFile("content/player.png");
	monster->loadFromFile("content/monster.png");
	tile1->loadFromFile("content/tile1.png");
	tile2->loadFromFile("content/tile2.png");
	if (false == g_font.loadFromFile("content/cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	a_tile = OBJECT{ *tile1, 5, 5, TILE_WIDTH, TILE_WIDTH };
	b_tile = OBJECT{ *tile2, 5, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *player, 0, 0, 64, 64 };
	monst = OBJECT{ *monster, 0, 0, 64, 64 };
	obst = OBJECT{ *obstacle, 0, 0, 128, 128 };
	avatar.move(0, 0);
}

void client_finish()
{
	players.clear();
	delete player;
	delete monster;
	delete tile1;
	delete tile2;
	delete obstacle;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[2])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.move(packet->x, packet->y);
		avatar.m_maxHP = packet->maxHp;
		avatar.m_exp = packet->exp;
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
		
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.show();
	}
	break;

	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = OBJECT{ *player, 0, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		else {
			players[id] = OBJECT{ *monster, 0, 0, 64, 64 };
			players[id].id = id;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		int type = (int)my_packet->chatType;
		int other_id = my_packet->id;
		string str = "";
		if (other_id == g_myid) {
			str += avatar.name;
		}
		else {
			str += players[other_id].name;
		}
		str += " : ";
		str += my_packet->mess;
		const char* message = str.c_str();

		switch (type)
		{
		case 0:
		{
			if (other_id == g_myid) {
				avatar.set_battle_chat(my_packet->mess);
			}
			else {
				players[other_id].set_battle_chat(my_packet->mess);
			}
		}
		break;
		case 1:
		{
			if (other_id == g_myid) {
				avatar.set_chat(message);
			}
			else {
				players[other_id].set_chat(message);
			}
		}
		break;
		}
		break;
	}
	case SC_LOGIN_OK:
		break;
	case SC_LOGIN_FAIL:
	{
		if (g_window)
			g_window->close();
		break;
	}
	case SC_STAT_CHANGE:
	{
		SC_STAT_CHANGE_PACKET* packet = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);

		avatar.m_maxHP = packet->max_hp;
		avatar.m_exp = packet->exp;
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
	}
		break;
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) 
			in_packet_size = *(reinterpret_cast<short*>(ptr));
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (0 == (tile_x / 3 + tile_y / 3) % 2) {
				a_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				a_tile.a_draw();
			}
			else
			{
				b_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				b_tile.a_draw();
			}
			if (g_MapData[tile_x][tile_y] == 0) {
				obst.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				obst.a_draw();
			}
		}
	avatar.draw();
	for (auto& pl : players) pl.second.draw();

	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	g_window->draw(text);


	sf::Text hp;
	hp.setFont(g_font);
	char hpBuf[100];
	sprintf_s(hpBuf, "HP : %d / %d", avatar.m_hp, avatar.m_maxHP);
	hp.setString(hpBuf);
	hp.setPosition(10, 30);
	g_window->draw(hp);

	sf::Text level;
	level.setFont(g_font);
	char lvBuf[100];
	sprintf_s(lvBuf, "Level : %d", avatar.m_level);
	level.setString(lvBuf);
	level.setPosition(10, 60);
	g_window->draw(level);


	sf::Text exp;
	exp.setFont(g_font);
	char expBuf[100];
	sprintf_s(expBuf, "exp : %d", avatar.m_exp);
	exp.setString(expBuf);
	exp.setPosition(10, 90);
	g_window->draw(exp);
}

void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	unsigned short packetSize = *reinterpret_cast<unsigned short*>(packet);
	size_t sent = 0;
	s_socket.send(packet, packetSize, sent);
}

int main()
{
	string userName;
	cout << "ID를 입력하세요 : ";
	cin >> userName;
	ReadMap();
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	client_initialize();
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;

	strcpy_s(p.name, userName.c_str());
	send_packet(&p);
	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				int a = 0;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					break;
				case sf::Keyboard::Right:
					direction = 3;
					break;
				case sf::Keyboard::Up:
					direction = 0;
					break;
				case sf::Keyboard::Down:
					direction = 1;
					break;
				case sf::Keyboard::A:
				{
					CS_ATTACK_PACKET p;
					p.size = sizeof(CS_ATTACK_PACKET);
					p.type = CS_ATTACK;
					send_packet(&p);
					break;
				}
				case sf::Keyboard::Escape:
				{
					CS_LOGOUT_PACKET p;
					p.size = sizeof(CS_LOGOUT_PACKET);
					p.type = CS_LOGOUT;
					send_packet(&p);
					window.close();
					break;
				}
				case sf::Keyboard::F1:
				{
					CS_TELEPORT_PACKET p;
					p.size = sizeof(CS_TELEPORT_PACKET);
					p.type = CS_TELEPORT;
					send_packet(&p);
					break;
				}
				case sf::Keyboard::F2:
				{
					CS_CHAT_PACKET p;
					p.size = sizeof(CS_CHAT_PACKET);
					p.type = CS_CHAT;
					strcpy_s(p.mess, "HELLO");
					send_packet(&p);
					break;
				}
				case sf::Keyboard::F3:
				{
					CS_CHAT_PACKET p;
					p.size = sizeof(CS_CHAT_PACKET);
					p.type = CS_CHAT;
					strcpy_s(p.mess, "BYE");
					send_packet(&p);
					break;
				}
				}
				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
				}
			}
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}