#include "JobQueue.hpp"
#include "UpdateFactory.hpp"


void Queue::AddJob(const Job& job, const JobPriority& priority)
{
	wxMutexLocker lock(m_MutexQueue);
	m_Jobs.insert(std::make_pair(priority, job));
	m_QueueCount.Post();
}

Job Queue::Pop()
{
	Job element;
	m_QueueCount.Wait();
	m_MutexQueue.Lock();
	element = (m_Jobs.begin())->second;
	m_Jobs.erase(m_Jobs.begin());
	m_MutexQueue.Unlock();
	return element;
}

void Queue::Report(const Job::JobEvents& cmd, const wxString& sArg, int iArg)
{
	wxCommandEvent evt(wxEVT_THREAD, cmd);
	evt.SetString(sArg);
	evt.SetInt(iArg);
	m_pParent->AddPendingEvent(evt);
}

wxThread::ExitCode WorkerThread::doWork()
{
	Sleep(1000);
	Job::JobEvents iErr;
	m_pQueue->Report(Job::eID_THREAD_STARTED, wxEmptyString, m_ID);
	try { while(true) OnJob(); }
	catch (Job::JobEvents& i) { m_pQueue->Report(iErr=i, wxEmptyString, m_ID); }
	return (wxThread::ExitCode) iErr;
}

void WorkerThread::doJob()
{
	int rv;
	std::string err;
	UpdateFactory uf;

	Job job = m_pQueue->Pop();
	m_pQueue->incBusyWorker();

	/* process the job which was started by the GUI */
	switch(job.m_cmd)
	{
		case Job::eID_THREAD_EXIT:
			m_pQueue->decBusyWorker();
			throw Job::eID_THREAD_EXIT;
		case Job::eID_THREAD_JOB:
			Sleep(1000); /* give the UI some time to handle a lot of new Jobs */

			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Connecting to %s:%i"),
			        job.m_Arg.jobid, job.m_Arg.hostname, job.m_Arg.port), m_ID);
			uf.setDest(job.m_Arg.hostname, job.m_Arg.port);
			uf.setPass(job.m_Arg.password);

			rv = uf.doAuth();
			if (uf.getEmcVersion() != EMC_UNKNOWN) {
				m_pQueue->Report(Job::eID_THREAD_MSG,
				    wxString::Format(wxT("Job #%d: Current EnergyManager version: %s"),
				        job.m_Arg.jobid, mapEmcVersion(uf.getEmcVersion())), m_ID);
			}

			if (rv != UPDATE_OK) {
				mapEmcError(rv, err);
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.m_Arg.jobid, err.c_str()), m_ID);
				break;
			}

			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Loading file \"%s\""),
			        job.m_Arg.jobid, job.m_Arg.update_file), m_ID);
			uf.setUpdateFile(job.m_Arg.update_file.c_str());
			rv = uf.loadUpdateFile();

			if (uf.getFwVersion() == EMC_UNKNOWN) {
				m_pQueue->Report(Job::eID_THREAD_MSG,
				    wxString::Format(wxT("Job #%d: Invalid firmware update file"),
				        job.m_Arg.jobid), m_ID);
				break;
			}
			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Firmware image version: %s"),
			        job.m_Arg.jobid, mapEmcVersion(uf.getFwVersion())), m_ID);
			if (!isEmcVersionLowerThen(uf.getEmcVersion(), uf.getFwVersion())) {
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
					wxString::Format(wxT("Job #%d: Version mismatch (%s >= %s)"),
						job.m_Arg.jobid, mapEmcVersion(uf.getEmcVersion()),
						mapEmcVersion(uf.getFwVersion())), m_ID);
				break;
			}

			if (rv != UPDATE_OK) {
				mapEmcError(rv, err);
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.m_Arg.jobid, err.c_str()), m_ID);
				break;
			}

			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Uploading file \"%s\""),
			        job.m_Arg.jobid, job.m_Arg.update_file), m_ID);
			rv = uf.doUpdate();
			mapEmcError(rv, err);
			if (rv != UPDATE_OK) {
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.m_Arg.jobid, err.c_str()), m_ID);
				break;
			}

			m_pQueue->Report(Job::eID_THREAD_MSGOK,
			    wxString::Format(wxT("Job #%d: %s."),
			        job.m_Arg.jobid, err), m_ID);
			break;
		case Job::eID_THREAD_NULL:
		default:
			break;
	}
	m_pQueue->Report(Job::eID_THREAD_JOB_DONE,
	    wxString::Format(wxT("Job #%d: finished."),
	        job.m_Arg.jobid), m_ID);
	m_pQueue->decBusyWorker();
}
