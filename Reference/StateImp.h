#pragma once
#include <vector>

class CContext;

class CState
{
public:
    virtual ~CState() = default;

public:
    virtual int Handle(CContext* pContext, WPARAM = NULL, LPARAM = NULL) = 0;
};

class CContext
{
public:
    CContext() = default;
    CContext(CState* pState)
    {
        m_pState = pState;
    }
    virtual ~CContext() = default;

    virtual BOOL Exec(WPARAM wParam = NULL, LPARAM lParam = NULL)
    {
        for (size_t i = 0; i < m_StateAry.size(); i++)
        {
            if (m_StateAry[i] != nullptr)
            {
                if (m_StateAry[i]->Handle(this, wParam, lParam))
                {
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    void AddState(CState* pState)
    {
        m_StateAry.push_back(pState);
    }

    void StateChange(CState* pState)
    {
        m_pState = pState;
    }

protected:
    CState* m_pState = nullptr;
    std::vector<CState*> m_StateAry;
};
