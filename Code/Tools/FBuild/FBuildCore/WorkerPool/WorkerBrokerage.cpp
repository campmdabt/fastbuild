// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/Env/Env.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
	: m_BrokerageInitialized(false)
{
}

// InitBrokerage
//------------------------------------------------------------------------------
void WorkerBrokerage::InitBrokerage()
{
	PROFILE_FUNCTION;

	if (m_BrokerageInitialized)
	{
		return;
	}

	if (m_CoordinatorAddress.IsEmpty())
	{
		AStackString<> coordinator;
		if (Env::GetEnvVariable("FASTBUILD_COORDINATOR", coordinator))
		{
			m_CoordinatorAddress = coordinator;
		}
	}

	if (m_CoordinatorAddress.IsEmpty())
	{
		OUTPUT("Using brokerage folder\n");

		// brokerage path includes version to reduce unnecessary comms attempts
		const uint32_t protocolVersion = Protocol::PROTOCOL_VERSION_MAJOR;

		// root folder
		AStackString<> brokeragePath;
		if (Env::GetEnvVariable("FASTBUILD_BROKERAGE_PATH", brokeragePath))
		{
			// FASTBUILD_BROKERAGE_PATH can contain multiple paths separated by semi-colon. The worker will register itself into the first path only but
			// the additional paths are paths to additional broker roots allowed for finding remote workers (in order of priority)
			const char *start = brokeragePath.Get();
			const char *end = brokeragePath.GetEnd();
			AStackString<> pathSeparator(";");
			while (true)
			{
				AStackString<> root;
				AStackString<> brokerageRoot;

				const char *separator = brokeragePath.Find(pathSeparator, start, end);
				if (separator != nullptr)
				{
					root.Append(start, (size_t)(separator - start));
				}
				else
				{
					root.Append(start, (size_t)(end - start));
				}
				root.TrimStart(' ');
				root.TrimEnd(' ');
// <path>/<group>/<version>/
#if defined(__WINDOWS__)
				brokerageRoot.Format("%s\\main\\%u.windows\\", root.Get(), protocolVersion);
#elif defined(__OSX__)
				brokerageRoot.Format("%s/main/%u.osx/", root.Get(), protocolVersion);
#else
				brokerageRoot.Format("%s/main/%u.linux/", root.Get(), protocolVersion);
#endif

				m_BrokerageRoots.Append(brokerageRoot);
				if (!m_BrokerageRootPaths.IsEmpty())
				{
					m_BrokerageRootPaths.Append(pathSeparator);
				}

				m_BrokerageRootPaths.Append(brokerageRoot);

				if (separator != nullptr)
				{
					start = separator + 1;
				}
				else
				{
					break;
				}
			}
		}
	}
	else
	{
		OUTPUT("Using coordinator\n");
	}

	m_BrokerageInitialized = true;
}

// ConnectToCoordinator
//------------------------------------------------------------------------------
bool WorkerBrokerage::ConnectToCoordinator()
{
    if ( m_CoordinatorAddress.IsEmpty() == false )
    {
        m_ConnectionPool = FNEW( WorkerConnectionPool );
        m_Connection = m_ConnectionPool->Connect( m_CoordinatorAddress, Protocol::COORDINATOR_PORT, 2000, this ); // 2000ms connection timeout
        if ( m_Connection == nullptr )
        {
            FLOG_OUTPUT( "Failed to connect to the coordinator at %s\n", m_CoordinatorAddress.Get() );
            FDELETE m_ConnectionPool;
            m_ConnectionPool = nullptr;
            // m_CoordinatorAddress.Clear();
            return false;
        }

        FLOG_OUTPUT( "Connected to the coordinator\n" );
        return true;
    }

    return false;
}

// DisconnectFromCoordinator
//------------------------------------------------------------------------------
void WorkerBrokerage::DisconnectFromCoordinator()
{
    if ( m_ConnectionPool )
    {
        FDELETE m_ConnectionPool;
        m_ConnectionPool = nullptr;
        m_Connection = nullptr;

        FLOG_OUTPUT( "Disconnected from the coordinator\n" );
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage() = default;

//------------------------------------------------------------------------------
