// WorkerBrokerageClient - Client-side worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerageClient
//------------------------------------------------------------------------------
class WorkerBrokerageClient : public WorkerBrokerage
{
public:
    WorkerBrokerageClient();
    ~WorkerBrokerageClient();

    void FindWorkers( Array< AString > & outWorkerList );
    void UpdateWorkerList( Array< uint32_t > &workerListUpdate );

protected:
    Array< uint32_t >   m_WorkerListUpdate;
    bool                m_WorkerListUpdateReady;
};

//------------------------------------------------------------------------------
