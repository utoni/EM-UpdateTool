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

void Queue::ReportDone(const JobReport& jobReport, const wxString& sArg, int iArg)
{
	wxCommandEvent evt(wxEVT_THREAD, Job::eID_THREAD_JOB_DONE);
	evt.SetString(sArg);
	evt.SetInt(iArg);
	evt.SetClientObject(new JobReport(jobReport));
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
	std::time_t jobStart, jobEnd;
	unsigned diffTime;
	std::string err;
	UpdateFactory uf;

	Job job = m_pQueue->Pop();
	m_pQueue->incBusyWorker();
	jobStart = std::time(nullptr);

	/* process the job which was started by the GUI */
	switch (job.cmdEvent)
	{
		case Job::eID_THREAD_EXIT:
			m_pQueue->decBusyWorker();
			throw Job::eID_THREAD_EXIT;
		case Job::eID_THREAD_JOB:
			Sleep(1000); /* give the UI some time to handle a lot of new Jobs */

			/* try to CONNECT to an EnergyManager */
			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Connecting to %s:%i"),
			        job.cmdArgs.jobid, job.cmdArgs.hostname, job.cmdArgs.port), m_ID);
			uf.setDest(job.cmdArgs.hostname, job.cmdArgs.port);
			uf.setPass(job.cmdArgs.password);

			/* try to AUTHENTICATE against an EnergyManager */
			rv = uf.doAuth();
			/* print REMOTE EnergyManager firmware version (if valid) */
			if (uf.getEmcVersion() != EMC_UNKNOWN) {
				m_pQueue->Report(Job::eID_THREAD_MSG,
				    wxString::Format(wxT("Job #%d: Current EnergyManager version: %s"),
				        job.cmdArgs.jobid, mapEmcVersion(uf.getEmcVersion())), m_ID);
			}

			/* check if AUTHENTICATE failed */
			if (rv != UPDATE_OK) {
				mapEmcError(rv, err);
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.cmdArgs.jobid, err.c_str()), m_ID);
				break;
			}

			/* load UPDATE IMAGE into memory */
			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Loading file \"%s\""),
			        job.cmdArgs.jobid, job.cmdArgs.update_file), m_ID);
			uf.setUpdateFile(job.cmdArgs.update_file.c_str());
			rv = uf.loadUpdateFile();

			/* validate and check for UPDATE IMAGE firmware version */
			if (uf.getFwVersion() == EMC_UNKNOWN) {
				m_pQueue->Report(Job::eID_THREAD_MSG,
				    wxString::Format(wxT("Job #%d: Invalid firmware update file"),
				        job.cmdArgs.jobid), m_ID);
				break;
			}
			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Firmware image version: %s"),
			        job.cmdArgs.jobid, mapEmcVersion(uf.getFwVersion())), m_ID);

			/* compare REMOTE with UPDATE IMAGE firmware version */
			if (!isEmcVersionLowerThen(uf.getEmcVersion(), uf.getFwVersion())) {
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
					wxString::Format(wxT("Job #%d: Version mismatch (%s >= %s)"),
						job.cmdArgs.jobid, mapEmcVersion(uf.getEmcVersion()),
						mapEmcVersion(uf.getFwVersion())), m_ID);
				break;
			}

			/* compare succeeded */
			if (rv != UPDATE_OK) {
				mapEmcError(rv, err);
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.cmdArgs.jobid, err.c_str()), m_ID);
				break;
			}

			/* upload UPDATE IMAGE to the REMOTE EnergyManager */
			m_pQueue->Report(Job::eID_THREAD_MSG,
			    wxString::Format(wxT("Job #%d: Uploading file \"%s\""),
			        job.cmdArgs.jobid, job.cmdArgs.update_file), m_ID);
			rv = uf.doUpdate();
			/* report any errors during upload */
			mapEmcError(rv, err);
			if (rv != UPDATE_OK) {
				m_pQueue->Report(Job::eID_THREAD_MSGERR,
				    wxString::Format(wxT("Job #%d: %s."),
				        job.cmdArgs.jobid, err.c_str()), m_ID);
				break;
			}

			/* done */
			m_pQueue->Report(Job::eID_THREAD_MSGOK,
			    wxString::Format(wxT("Job #%d: %s."),
			        job.cmdArgs.jobid, err), m_ID);
			break;
		case Job::eID_THREAD_NULL:
		default:
			break;
	}

	/* report consumed time */
	jobEnd = std::time(nullptr);
	diffTime = std::difftime(jobEnd, jobStart);
	m_pQueue->ReportDone(JobReport(diffTime),
	    wxString::Format(wxT("Job #%d: finished. Time consumed: %umin %usec"),
	        job.cmdArgs.jobid, (diffTime / 60), (diffTime % 60)), m_ID);

	m_pQueue->decBusyWorker();
}
