#include "stdafx.h"
#include "Manager.h"
#include "GameManager.h"
#include "PlayerSession.h"
#include "DataBaseManager.h"

DataBaseManager::~DataBaseManager()
{
    if (m_hDbc != nullptr)
    {
        SQLDisconnect(m_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
        m_hDbc = nullptr;
    }
    if (m_hEnv != nullptr)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = nullptr;
    }
}

void DataBaseManager::Init()
{
    cout << "Initiate DataBase initialization.\n";
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
	SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

    SetHandle();
    // 핸들 타임아웃을 고려하여 7시간 정도의 시간 뒤에 다시 핸들을 설정해주는 명령어 입력
    // 멀티쓰레드 환경에서 Handle이 중복되어 사용되면 문제가 발생할 수 있다.
    cout << "DataBase initialization complete.\n";
}

void DataBaseManager::HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
    // Microsoft Sample
    SQLSMALLINT iRec = 0;
    SQLINTEGER  iError;
    WCHAR       wszMessage[1000];
    WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];


    if (RetCode == SQL_INVALID_HANDLE)
    {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }

    while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
        (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)),
        (SQLSMALLINT*)NULL) == SQL_SUCCESS)
    {
        // Hide data truncated..
        // 01004(데이터 잘림 경고)가 아닌 경우에만 메시지를 출력
        if (wcsncmp(wszState, L"01004", 5))
        {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}

bool DataBaseManager::GetUserData(WCHAR* userID, PlayerSession* pPlayer)
{
    SQLRETURN retcode;

    SQLWCHAR szName[20];
    SQLINTEGER hp, level, exp, xPos, yPos;
    SQLLEN cbHp, cbName, cbXPos, cbYPos, cbLevel, cbExp;

    wstring sQuery = L"EXEC GetUserData ";
    sQuery += userID;

    const wchar_t* cpQuery = sQuery.c_str();

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &m_hStmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        HandleDiagnosticRecord(m_hStmt, SQL_HANDLE_STMT, retcode);
        return false;
    }

    retcode = SQLExecDirect(m_hStmt, (SQLWCHAR*)cpQuery, SQL_NTS);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        SQLBindCol(m_hStmt, 1, SQL_C_WCHAR, szName, sizeof(szName), &cbName);
        SQLBindCol(m_hStmt, 2, SQL_C_LONG, &hp, 0, &cbHp);
        SQLBindCol(m_hStmt, 3, SQL_C_LONG, &level, 0, &cbLevel);
        SQLBindCol(m_hStmt, 4, SQL_C_LONG, &exp, 0, &cbExp);
        SQLBindCol(m_hStmt, 5, SQL_C_LONG, &xPos, 0, &cbXPos);
        SQLBindCol(m_hStmt, 6, SQL_C_LONG, &yPos, 0, &cbYPos);

        retcode = SQLFetch(m_hStmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            // PlayerSession에 데이터 추가
            pPlayer->SetLevel(level);
            pPlayer->SetExp(exp);
            pPlayer->SetHp(hp);
            pPlayer->SetMaxHp(hp);
            pPlayer->SetPos(yPos, xPos);
        }
        else
        {
            HandleDiagnosticRecord(m_hStmt, SQL_HANDLE_STMT, retcode);
            return false;
        }
    }
    else
    {
        HandleDiagnosticRecord(m_hStmt, SQL_HANDLE_STMT, retcode);
        return false;
    }
    SQLCloseCursor(m_hStmt);
    return true;
}

bool DataBaseManager::UpdatePlayerData(WCHAR* userID, int objID)
{
    Manager& manager = Manager::GetInstance();
    PlayerSession* player = manager.GetGameManager()->GetPlayerSession(objID);
    SQLRETURN retcode;
    SQLWCHAR szName[20];
    SQLINTEGER hp, level, exp, xPos, yPos;
    SQLLEN cbName = SQL_NTS, cbHp = 0, cbExp = 0, cbLevel = 0, cbXPos = 0, cbYPos = 0;

    wcsncpy_s(szName, userID, 20);
    szName[19] = L'\0';
    hp = std::max(0.0f, player->GetHp());
    exp = std::max(0, player->GetExp());
    level = std::max(1, player->GetLevel());
    xPos = std::clamp(player->GetPos().xPos, 0, W_WIDTH);
    yPos = std::clamp(player->GetPos().yPos, 0, W_HEIGHT);

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &m_hStmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        HandleDiagnosticRecord(m_hDbc, SQL_HANDLE_DBC, retcode);
        return false;
    }
    const wchar_t* query = L"{CALL UpdateUserInfo(?, ?, ?, ?, ?, ?)}";
    retcode = SQLPrepare(m_hStmt, (SQLWCHAR*)query, SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        HandleDiagnosticRecord(m_hStmt, SQL_HANDLE_STMT, retcode);
        return false;
    }

    SQLBindParameter(m_hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 20, 0, szName, 20 * sizeof(SQLWCHAR), &cbName);
    SQLBindParameter(m_hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &hp, 0, &cbHp);
    SQLBindParameter(m_hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &exp, 0, &cbExp);
    SQLBindParameter(m_hStmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &level, 0, &cbLevel);
    SQLBindParameter(m_hStmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &xPos, 0, &cbXPos);
    SQLBindParameter(m_hStmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &yPos, 0, &cbYPos);

    retcode = SQLExecute(m_hStmt);

    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
    {
        std::cout << "DataBase UserInfo Update Fail.\n";
        HandleDiagnosticRecord(m_hStmt, SQL_HANDLE_STMT, retcode);
        SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
        return false;
    }
    SQLCloseCursor(m_hStmt);
    return true;
}

void DataBaseManager::SetHandle()
{
    if (m_hDbc != nullptr)
    {
        SQLDisconnect(m_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
    }

    SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDbc);
    SQLSetConnectAttr(m_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
    SQLRETURN rev = SQLConnect(m_hDbc, (SQLWCHAR*)L"2018184036_Server_DB", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
    if (rev != SQL_SUCCESS && rev != SQL_SUCCESS_WITH_INFO)
    {
        cout << "DataBase Connect Error.\n";
        HandleDiagnosticRecord(m_hDbc, SQL_HANDLE_DBC, rev);
    }
}
