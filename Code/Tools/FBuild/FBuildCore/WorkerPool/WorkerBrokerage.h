// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class WorkerConnectionPool;
class ConnectionInfo;

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
    WorkerBrokerage();
    ~WorkerBrokerage();

    const AString & GetBrokerageRootPaths() const { return m_BrokerageRootPaths; }
    const AString & GetHostName() const { return m_HostName; }

	bool ConnectToCoordinator();
	void DisconnectFromCoordinator();

protected:
    void InitBrokerage();

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootPaths;
    bool                m_BrokerageInitialized;
	AString             m_CoordinatorAddress;
    AString             m_HostName;
    WorkerConnectionPool * m_ConnectionPool;
    const ConnectionInfo * m_Connection;
};

//------------------------------------------------------------------------------
