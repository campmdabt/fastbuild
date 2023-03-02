// WorkerBrokerageClient - Client-side worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerageClient.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::WorkerBrokerageClient()
	: m_WorkerListUpdateReady(false)
{
	
};

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::~WorkerBrokerageClient() = default;

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerageClient::FindWorkers(Array<AString> &outWorkerList)
{
	PROFILE_FUNCTION;

	// Check for workers for the FASTBUILD_WORKERS environment variable
	// which is a list of worker addresses separated by a semi-colon.
	AStackString<> workersEnv;
	if (Env::GetEnvVariable("FASTBUILD_WORKERS", workersEnv))
	{
		// If we find a valid list of workers, we'll use that
		workersEnv.Tokenize(outWorkerList, ';');
		if (outWorkerList.IsEmpty() == false)
		{
			return;
		}
	}

	// check for workers through brokerage

	// Init the brokerage
	InitBrokerage();
	if (m_BrokerageRoots.IsEmpty() && m_CoordinatorAddress.IsEmpty())
	{
		FLOG_WARN("No brokerage root and no coordinator available; did you set FASTBUILD_BROKERAGE_PATH or launched with -coordinator param?");
		return;
	}

	if (ConnectToCoordinator())
	{
		m_WorkerListUpdateReady = false;

		FLOG_OUTPUT("Requesting worker list\n");

		const Protocol::MsgRequestWorkerList msg;
		msg.Send(m_Connection);

		while (m_WorkerListUpdateReady == false)
		{
			Thread::Sleep(1);
		}

		DisconnectFromCoordinator();

		FLOG_OUTPUT("Worker list received: %u workers\n", (uint32_t)m_WorkerListUpdate.GetSize());
		if (m_WorkerListUpdate.GetSize() == 0)
		{
			FLOG_WARN("No workers received from coordinator");
			return; // no files found
		}

		// presize
		if ((outWorkerList.GetSize() + m_WorkerListUpdate.GetSize()) > outWorkerList.GetCapacity())
		{
			outWorkerList.SetCapacity(outWorkerList.GetSize() + m_WorkerListUpdate.GetSize());
		}

		// Get addresses for the local host
		StackArray<AString> localAddresses;
		Network::GetIPv4Addresses(localAddresses);

		// convert worker strings
		const uint32_t *const end = m_WorkerListUpdate.End();
		for (uint32_t *it = m_WorkerListUpdate.Begin(); it != end; ++it)
		{
			AStackString<> workerName;
			TCPConnectionPool::GetAddressAsString(*it, workerName);

			// Filter out local addresses
			if (localAddresses.Find(workerName))
			{
				FLOG_OUTPUT("Skipping woker %s\n", workerName.Get());
				continue;
			}

			outWorkerList.Append(workerName);
		}

		m_WorkerListUpdate.Clear();
	}
	else if (!m_BrokerageRoots.IsEmpty())
	{
		Array<AString> results(256, true);
		for (AString &root : m_BrokerageRoots)
		{
			const size_t filesBeforeSearch = results.GetSize();
			if (!FileIO::GetFiles(root,
								  AStackString<>("*"),
								  false,
								  &results))
			{
				FLOG_WARN("No workers found in '%s'", root.Get());
			}
			else
			{
				FLOG_WARN("%zu workers found in '%s'", results.GetSize() - filesBeforeSearch, root.Get());
			}
		}

		// presize
		if ((outWorkerList.GetSize() + results.GetSize()) > outWorkerList.GetCapacity())
		{
			outWorkerList.SetCapacity(outWorkerList.GetSize() + results.GetSize());
		}

		// Get addresses for the local host
		StackArray<AString> localAddresses;
		Network::GetIPv4Addresses(localAddresses);

		// convert worker strings
		for (const AString &fileName : results)
		{
			const char *lastSlash = fileName.FindLast(NATIVE_SLASH);
			AStackString<> workerName(lastSlash + 1);

			// Filter out local addresses
			if (localAddresses.Find(workerName))
			{
				continue;
			}

			outWorkerList.Append(workerName);
		}
	}
}

// UpdateWorkerList
//------------------------------------------------------------------------------
void WorkerBrokerageClient::UpdateWorkerList( Array< uint32_t > &workerListUpdate )
{
    m_WorkerListUpdate.Swap( workerListUpdate );
    m_WorkerListUpdateReady = true;
}

//------------------------------------------------------------------------------
