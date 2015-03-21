// JobQueueRemote - list of pending jobs
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_WORKERPOOL_JOBQUEUEREMOTE_H
#define FBUILD_WORKERPOOL_JOBQUEUEREMOTE_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"

#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Core/Process/Mutex.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Node;
class Job;
class WorkerThread;

// JobQueueRemote
//------------------------------------------------------------------------------
class JobQueueRemote : public Singleton< JobQueueRemote >
{
public:
	JobQueueRemote( uint32_t numWorkerThreads );
	~JobQueueRemote();

	// main thread calls these
	void QueueJob( Job * job );
	Job * GetCompletedJob();
	void CancelJobsWithUserData( void * userData );

	// handle shutting down
	void SignalStopWorkers();
	bool HaveWorkersStopped() const;

	inline size_t GetNumWorkers() const { return m_Workers.GetSize(); }
	void		  GetWorkerStatus( size_t index, AString & hostName, AString & status, bool & isIdle ) const;
private:
	// worker threads call these
	friend class WorkerThread;
	friend class WorkerThreadRemote;
	Job *		GetJobToProcess();
	static Node::BuildResult DoBuild( Job * job, bool racingRemoteJob );
	void		FinishedProcessingJob( Job * job, bool result );

	// internal helpers
	static bool	ReadResults( Job * job );

	mutable Mutex		m_PendingJobsMutex;
	Array< Job * >		m_PendingJobs;
	mutable Mutex		m_InFlightJobsMutex;
	Array< Job * >		m_InFlightJobs;
	Mutex				m_CompletedJobsMutex;
	Array< Job * >		m_CompletedJobs;
	Array< Job * >		m_CompletedJobsFailed;

	Array< WorkerThread * > m_Workers;
};

//------------------------------------------------------------------------------
#endif // FBUILD_WORKERPOOL_JOBQUEUEREMOTE_H 