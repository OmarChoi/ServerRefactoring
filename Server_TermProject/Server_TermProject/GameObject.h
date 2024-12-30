#pragma once
class GameObject
{
public:
	int								m_xPos, m_yPos;
	int								m_spawnXPos, m_spawnYPos;
	int								m_objectID;
	char							m_name[NAME_SIZE];
	int								m_lastMoveTime;
	chrono::system_clock::time_point m_tpLastMoveTime;
	chrono::system_clock::time_point m_tpLastAttackTime;

	unordered_set<int>				m_viewList;
	mutex							m_viewListLock;
public:
	int								m_hp;
	int								m_maxHp;
	int								m_level;
	OBJ_TYPE						m_type;
public:
	GameObject();
	~GameObject();
	bool canSee(const GameObject* otherPlayer);
	bool CanGo(int x, int y);
	void AddViewList(int objID);
	void RemoveViewList(int objID);
	bool CanHit(int objID, OBJ_TYPE ot);
};

class Player : public GameObject
{
	OVER_EXP						m_recvOver;
public:
	SOCKET							m_socket;
	mutex							m_socketLock;
	C_STATE							m_playerState;
	mutex							m_stateLock;
	int								m_prevRemain;
	int								m_lastActionTime;
	WCHAR							m_nameForDB[NAME_SIZE];

	unordered_set<int>				m_viewNpcList;
	mutex							m_viewNpcListLock;
public:
	int								m_exp;
public:
	Player(int objID = -1);
	~Player() {}

	void ProcessPacket(char* packet);
	void UpdateViewList();
	void AddViewNPCList(int objID);
	void RemoveViewNPCList(int objID);

public:
	void callRecv();
	void callSend(void* packet);
	void send_add_object_packet(int objId);
	void send_add_npc_packet(int objId);
	void send_remove_object_packet(int objId);
	void send_move_object_packet(int objId);
	void send_npc_move_object_packet(int objId);
	void send_chat_packet(int objId, const char* mess, char chatType);

public:
	void send_login_info_packet();
	void send_login_ok_packet();
	void send_login_fail_packet();
	void send_stat_change_packet();
};

class NPC : public GameObject
{
public:
	bool			m_isActive;
	mutex			m_activeLock;

	int				m_targetID;
	mutex			m_targetLock;
	MONSTER_TYPE	m_monserType;
	lua_State*		m_luaState;
	mutex			m_luaLock;

	bool			m_isDead;
public:
	pair<int, int>	m_targetPos{ INT_MAX, INT_MAX };
	stack<pair<int, int>> m_path;
	chrono::system_clock::time_point m_changeTime;
public:
	NPC();
	~NPC();

	void UpdateViewList();
	void WakeUp(int objID);
	void ReleaseTarget(int objID);
	void Move();
	void SimplyMove();
	bool GetAgro(int x, int y);
	void Attack();
	void TrackingPath(pair<int, int> start);
	void GetHit(int objID);
	int GetDistance(pair<int, int> start, pair<int, int> dest)
	{
		int a = abs(start.first - dest.first);
		int b = abs(start.second - dest.second);
		return a + b;
	}
};