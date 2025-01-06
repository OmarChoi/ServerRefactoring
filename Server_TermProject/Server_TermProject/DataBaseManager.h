#pragma once
class PlayerSession;
class DataBaseManager
{
private:
	SQLHENV m_hEnv;
	SQLHDBC m_hDbc;
	SQLHSTMT m_hStmt;

public:
	DataBaseManager() { Init(); }
	~DataBaseManager();

private:
	void Init();
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

public:
	bool GetUserData(WCHAR* userID, PlayerSession* pPlayer);
	bool UpdatePlayerData(WCHAR* userID, int objID);
	void SetHandle();
};

