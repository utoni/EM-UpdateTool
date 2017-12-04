#ifndef JOBQUEUE_H
#define JOBQUEUE_H 1

#include <string>
#include <map>
#include <list>
#include <wx/frame.h>
#include <wx/thread.h>
#include <wx/menu.h>
#include <wx/app.h>

#include "UpdateFactory.hpp"


class JobArgs
{
public:
	JobArgs()
	    : jobid(-1), hostname(""), port(0),
	      update_file(""), password("") {}
	JobArgs(int jobid, UpdateFactory& uf)
	    : jobid(jobid), hostname(uf.getHostname()),
	      port(uf.getPort()), update_file(uf.getUpdateFile()),
	      password(uf.getPassword()) {}
	JobArgs(int jobid, const char *hostname, int port,
	        const char *update_file, const char *password)
	    : jobid(jobid), hostname(hostname), port(port),
	      update_file(update_file), password(password) {}
	JobArgs(int jobid, std::string& hostname, int port,
	        std::string& update_file, std::string& password)
	    : jobid(jobid), hostname(hostname), port(port),
	      update_file(update_file), password(password) {}

	/** randomized (not unique) Job ID */
	int jobid;
	/* minimum required data for our UpdateFactory */
	std::string hostname;
	int port;
	std::string update_file;
	std::string password;
};

class Job
{
public:
	enum JobEvents
	{
		/* thread should exit or wants to exit */
		eID_THREAD_EXIT = wxID_HIGHEST + 1000,
		/* dummy command */
 		eID_THREAD_NULL,
		/* worker thread has started OK */
		eID_THREAD_STARTED,
		/* process normal job */
		eID_THREAD_JOB,
		/* job completed */
		eID_THREAD_JOB_DONE,
		/* process different messages in the frontend */
		eID_THREAD_MSG,
		eID_THREAD_MSGOK,
		eID_THREAD_MSGERR
	};

	Job() : m_cmd(eID_THREAD_NULL) {}
	Job(JobEvents cmd, JobArgs arg) : m_cmd(cmd), m_Arg(arg) {}
	Job(JobEvents cmd, int jobid, UpdateFactory& uf) : m_cmd(cmd), m_Arg(jobid, uf) {}
	JobEvents m_cmd;
	JobArgs m_Arg;
};

class Queue
{
public:
	enum JobPriority { eHIGHEST, eHIGHER, eNORMAL, eBELOW_NORMAL, eLOW, eIDLE };
	Queue(wxEvtHandler *pParent) : m_pParent(pParent), m_busyWorker(0), m_totalJobsDone(0) {}
	/* push a job with given priority class onto the FIFO */
	void AddJob(const Job& job, const JobPriority& priority = eNORMAL);
	Job Pop();
	/* report back to parent */
	void Report(const Job::JobEvents& cmd, const wxString& sArg = wxEmptyString, int iArg = 0);
	size_t Stacksize()
	{
		wxMutexLocker lock(m_MutexQueue);
		return m_Jobs.size();
	}
	size_t getBusyWorker() {
		wxMutexLocker lock(m_MutexBusyWorker);
		return m_busyWorker;
	}
	size_t getTotalJobsDone() {
		wxMutexLocker lock(m_MutexBusyWorker);
		return m_totalJobsDone;
	}
	void resetTotalJobsDone() {
		wxMutexLocker lock(m_MutexBusyWorker);
		m_totalJobsDone = 0;
	}
	void incBusyWorker() {
		wxMutexLocker lock(m_MutexBusyWorker);
		m_busyWorker++;
	}
	void decBusyWorker() {
		wxMutexLocker lock(m_MutexBusyWorker);
		if (m_busyWorker > 0) m_busyWorker--;
		m_totalJobsDone++;
	}
private:
	/** main GUI EventHandler */
	wxEvtHandler *m_pParent;
	/** a priority Queue using std::multimap */
	std::multimap<JobPriority, Job> m_Jobs;
	wxMutex m_MutexQueue, m_MutexBusyWorker;
	wxSemaphore m_QueueCount;
	size_t m_busyWorker, m_totalJobsDone;
};
 
class WorkerThread : public wxThread
{
public:
	WorkerThread(Queue *pQueue, int id = 0) : m_pQueue(pQueue), m_ID(id) { assert(pQueue); wxThread::Create(); }
private:
	Queue *m_pQueue;
	int m_ID;

	wxThread::ExitCode doWork();
	virtual wxThread::ExitCode Entry() { return this->doWork(); }
	void doJob();
	virtual void OnJob() { return this->doJob(); }
};

#endif
