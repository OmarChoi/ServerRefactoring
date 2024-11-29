#pragma once

class DataBase
{
public:
	DataBase() {};
	~DataBase() {};
private:
	SQLHENV m_hEnv;
	SQLHDBC m_hDbc;
	SQLHSTMT m_hStmt;

public:
	void InitalizeDB();
	bool GetUserData(WCHAR* userID, int objID);
	bool UpdatePlayerData(WCHAR* userID, int objID);
};