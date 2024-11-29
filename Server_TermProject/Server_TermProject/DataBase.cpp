#include "stdafx.h"
#include "DataBase.h"
#include "GameObject.h"

extern array<Player*, MAX_USER> Players;

void PrintDataBaseError(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER iError;
    WCHAR wszMessage[1000];
    WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
    if (RetCode == SQL_INVALID_HANDLE) {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }
    while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
        (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
        // Hide data truncated..
        if (wcsncmp(wszState, L"01004", 5)) {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}

void DataBase::InitalizeDB()
{
    cout << "Init DataBase\n";
    // Allocate environment handle  
    SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(m_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

                // Connect to data source  
                retcode = SQLConnect(m_hDbc, (SQLWCHAR*)L"2018184036_Server_DB", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
            }
            else
            {
                PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
                return;
            }
        }
        else
        {
            PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
            return;
        }
    }
    else
    {
        PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
        return;
    }
    cout << "Success DataBase\n";
}

bool DataBase::GetUserData(WCHAR* userID, int objID)
{
    SQLRETURN retcode;

    SQLWCHAR szName[20];
    SQLINTEGER hp, level, exp, xPos, yPos;
    SQLLEN cbHp, cbName, cbXPos, cbYPos, cbLevel, cbExp;
    
    wstring sQuery = L"EXEC GetUserData ";
    sQuery += userID;
    const wchar_t* cpQuery = sQuery.c_str();

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &m_hStmt);
    retcode = SQLExecDirect(m_hStmt, (SQLWCHAR*)cpQuery, SQL_NTS);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        // userName, userHp, userLevel, userEXP
        retcode = SQLBindCol(m_hStmt, 1, SQL_C_WCHAR, szName, 22, &cbName);
        retcode = SQLBindCol(m_hStmt, 2, SQL_C_LONG, &hp, 10, &cbHp);
        retcode = SQLBindCol(m_hStmt, 3, SQL_C_LONG, &level, 10, &cbLevel);
        retcode = SQLBindCol(m_hStmt, 4, SQL_C_LONG, &exp, 10, &cbExp);
        retcode = SQLBindCol(m_hStmt, 5, SQL_C_LONG, &xPos, 10, &cbXPos);
        retcode = SQLBindCol(m_hStmt, 6, SQL_C_LONG, &yPos, 10, &cbYPos);

        retcode = SQLFetch(m_hStmt);  // Fetch를 통해서 레코드를 한 줄씩 출력 , Fetch 전에 field를 어느 변수에 받을지 지정해줘야한다.
        if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
            PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        { 
            Players[objID]->m_level = level;
            Players[objID]->m_hp = hp;
            Players[objID]->m_exp = exp;
            Players[objID]->m_xPos = xPos;
            Players[objID]->m_yPos = yPos;
            printf("%ls : %d\n", szName, objID);

            SQLCancel(m_hStmt);
            SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
        }
        else 
        {
            PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
            return false;
        } 
    }
    else
    {
        PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
        return false;
    }
    return true;
}

bool DataBase::UpdatePlayerData(WCHAR* userID, int objID)
{
    SQLRETURN retcode;

    SQLWCHAR szName[20];
    SQLINTEGER hp, level, exp, xPos, yPos;
    SQLLEN cbHp, cbName, cbXPos, cbYPos, cbLevel, cbExp;

    //@name nchar(20),
    //@hp INT,
    //@exp INT,
    //@level INT,
    //@xPos INT,
    //@yPos INT

    wstring sQuery = L"EXEC UpdateUserInfo ";
    sQuery += userID;
    sQuery += L", ";
    sQuery += to_wstring(Players[objID]->m_hp);
    sQuery += L", ";
    sQuery += to_wstring(Players[objID]->m_exp);
    sQuery += L", ";
    sQuery += to_wstring(Players[objID]->m_level);
    sQuery += L", ";
    sQuery += to_wstring(Players[objID]->m_xPos);
    sQuery += L", ";
    sQuery += to_wstring(Players[objID]->m_yPos);

    const wchar_t* cpQuery = sQuery.c_str();

    retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &m_hStmt);
    retcode = SQLExecDirect(m_hStmt, (SQLWCHAR*)cpQuery, SQL_NTS);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
        cout << "Update Succes\n";
    else
    {
        cout << "Update Fail\n";
        PrintDataBaseError(m_hStmt, SQL_HANDLE_STMT, retcode);
        return false;
    }
    return true;
}
