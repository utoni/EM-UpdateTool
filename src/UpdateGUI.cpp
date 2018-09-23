#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "UpdateGUI.hpp"
#include "JobQueue.hpp"
#include "license.h"

#include <iostream>
#include <iomanip>
#include <tuple>
#include <chrono>
#include <ctime>

#include <wx/aboutdlg.h>
#include <wx/msgdlg.h>


wxBEGIN_EVENT_TABLE(UpdateGUIFrame, wxFrame)
	EVT_CLOSE(UpdateGUIFrame::OnClose)

	EVT_MENU(wxID_EXIT,       UpdateGUIFrame::OnExit)
	EVT_MENU(wxID_ABOUT,      UpdateGUIFrame::OnAbout)
	EVT_MENU(wxID_LICENSE,    UpdateGUIFrame::OnLicense)
	EVT_MENU(wxID_EDITOR,     UpdateGUIFrame::OnEditor)
	EVT_MENU(wxID_UPDATEFILE, UpdateGUIFrame::OnUpdateFile)
	EVT_MENU(wxID_IMPORTCSV,  UpdateGUIFrame::OnImportCSV)
	EVT_MENU(wxID_DOUPDATE,   UpdateGUIFrame::OnUpdate)

	EVT_BUTTON(wxID_UPDATEFILE, UpdateGUIFrame::OnUpdateFile)
	EVT_BUTTON(wxID_IMPORTCSV,  UpdateGUIFrame::OnImportCSV)
	EVT_BUTTON(wxID_DOUPDATE,   UpdateGUIFrame::OnUpdate)

	EVT_NAVIGATION_KEY(UpdateGUIFrame::OnNavigationKey)

	EVT_COMMAND(wxID_ANY, wxEVT_THREAD, UpdateGUIFrame::OnThread)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(UpdateGUILicense, wxFrame)
	EVT_CLOSE(UpdateGUILicense::OnClose)

	EVT_BUTTON(wxID_ACCEPT,   UpdateGUILicense::OnAccept)
	EVT_BUTTON(wxID_DECLINE,  UpdateGUILicense::OnDecline)
wxEND_EVENT_TABLE()


bool UpdateGUI::OnInit()
{
	if (isLicenseAlreadyAccepted()) {
		UpdateGUIFrame *frame = new UpdateGUIFrame("UpdateTool",
		                                wxPoint(50, 50), wxSize(450, 340));
		frame->Show(true);
	} else {
		UpdateGUILicense *frame = new UpdateGUILicense(nullptr, "UpdateTool - LICENSE",
		                                wxPoint(50, 50), wxSize(450, 340), true);
		frame->Show(true);
	}
	return true;
}

UpdateGUIFrame::UpdateGUIFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
          : wxFrame(NULL, wxID_ANY, title, pos, size)
{
	SetBackgroundColour(wxColour(220, 220, 220));
#ifdef WIN32
	/* Set a pretty GUI icon (left-top window bar).
	 * This requires the Icon to be compiled in a
	 * resources section with the windres compiler!
	 */
	SetIcon(wxICON(APPICON));
#endif

	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_UPDATEFILE,
	                 "&Select Image ...\tCtrl-F",
	                 "Select a firmware image.");
	menuFile->Append(wxID_DOUPDATE,
	                 "&Submit\tCtrl-U",
	                 "Start the firmware update process.");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_IMPORTCSV,
	                 "&Import CSV ...\tCtrl-I",
	                 "Import a CSV table from file.");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	menuHelp->Append(wxID_LICENSE,
	                 "&License ...\tCtrl-L",
	                 "Show the corresponding Software License");

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar(menuBar);
	CreateStatusBar();
	SetStatusText("Initialised");

	mainVSizer = new wxBoxSizer(wxVERTICAL);
	ipBox  = new wxStaticBoxSizer(wxHORIZONTAL, this, "IP address (or FQDN)");
	pwBox  = new wxStaticBoxSizer(wxHORIZONTAL, this, "Device Password");
	imgBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Firmware Image");
	subBox = new wxStaticBoxSizer(wxHORIZONTAL, this);
	logBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Status Log");

	imgButton = new wxButton(this, wxID_UPDATEFILE, wxT("Select ..."));
	imgBox->Add(imgButton, 0, wxALIGN_LEFT|wxALL, 5);
	subButton = new wxButton(this, wxID_DOUPDATE, wxT("Submit"));
	csvButton = new wxButton(this, wxID_IMPORTCSV, wxT("Import CSV ..."));
	subBox->AddStretchSpacer();
	subBox->Add(csvButton, 0, wxALL, 5);
	subBox->Add(subButton, 0, wxALL, 5);
	subBox->AddStretchSpacer();
	ipEntry   = new wxTextCtrl(this, wxID_IP, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, 0);
	ipEntry->SetHint(wxT("domain.local,domain.de:8080,192.168.0.1,192.168.0.1:8080"));
	ipBox->Add(ipEntry, 1, wxEXPAND|wxALL, 5);
	pwEntry   = new wxTextCtrl(this, wxID_PW, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_PASSWORD);
	pwBox->Add(pwEntry, 1, wxEXPAND|wxALL, 5);
	imgEntry  = new wxTextCtrl(this, wxID_IMG, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_READONLY
	);
	imgBox->Add(imgEntry, 1, wxALIGN_CENTER|wxALL, 5);
	logText   = new wxTextCtrl(
	    this, wxID_EDITOR, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2
	);
#ifdef WIN32
	logText->HideNativeCaret();
#endif
	logBox->Add(logText, 1, wxEXPAND|wxALL, 5);

	mainVSizer->Add(ipBox,  0, wxEXPAND|wxALL|wxTOP, 5);
	mainVSizer->Add(pwBox,  0, wxEXPAND|wxALL, 5);
	mainVSizer->Add(imgBox, 0, wxEXPAND|wxALL, 5);
	mainVSizer->Add(subBox, 0, wxEXPAND|wxALL, 5);
	mainVSizer->Add(logBox, 1, wxEXPAND|wxALL|wxBOTTOM, 5);

	SetSizerAndFit(mainVSizer);
	SetInitialSize(wxSize(600, 800));
	Centre();
	jobs = new Queue(this);
	{
		for (int tid = 1; tid <= 3; ++tid) {
			threads.push_back(tid);
			WorkerThread* thread = new WorkerThread(jobs, tid);
			if (thread) thread->Run();
		}
	}
	tLog(RTL_DEFAULT, "UpdateTool started (wxGUI).");
}

void UpdateGUIFrame::tLog(enum LogType type, const char *text, const char *ident)
{
	switch (type) {
		case RTL_DEFAULT:
			logText->SetDefaultStyle(wxTextAttr(*wxBLACK));
			break;
		case RTL_GREEN:
			logText->SetDefaultStyle(wxTextAttr(*wxGREEN));
			break;
		case RTL_RED:
			logText->SetDefaultStyle(wxTextAttr(*wxRED));
			break;
	}
	std::ostringstream out;
	auto timestamp = std::time(nullptr);
	out << "[" << std::put_time(std::localtime(&timestamp), "%H:%M:%S") << "] ";
	if (ident)
		out << "[" << ident << "] ";
	out << text << std::endl;
	logText->AppendText(out.str());
}

void UpdateGUIFrame::tLog(enum LogType type, std::string& text, const char *ident)
{
	this->tLog(type, text.c_str(), ident);
}

void UpdateGUIFrame::OnClose(wxCloseEvent& event)
{
	if (!threads.empty() && (jobs->Stacksize() > 0 || jobs->getBusyWorker() > 0)) {
		std::ostringstream log;
		log << "You have " << jobs->Stacksize() << " pending and "
		    << jobs->getBusyWorker() << " running job(s). Quit?";

		wxMessageDialog dlg(this, log.str(), wxMessageBoxCaptionStr,
		    wxYES_NO | wxCENTRE | wxICON_QUESTION);
		dlg.SetYesNoLabels("&Quit", "&Don't quit");
		switch (dlg.ShowModal()) {
			case wxID_YES: Destroy(); _exit(0);
			case wxID_NO:
			default: return;
		}
	}

	for (unsigned i = 0; i < threads.size(); ++i) {
		jobs->AddJob(Job(Job::eID_THREAD_EXIT, JobArgs()), Queue::eHIGHEST);
	}
	if (!threads.empty()) return;

	Destroy();
}

void UpdateGUIFrame::OnExit(wxCommandEvent& event)
{
	Close(true);
}

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "UpdateTool"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

void UpdateGUIFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(PACKAGE_NAME);
	aboutInfo.SetVersion(PACKAGE_VERSION);
	aboutInfo.SetDescription("A simple firmware update tool for Energy Manager / Datamanager firmware.");
	aboutInfo.SetCopyright("(C) TQ-Systems GmbH, Toni Uhlig 2017");
	wxAboutBox(aboutInfo, this);
}

void UpdateGUIFrame::OnLicense(wxCommandEvent& event)
{
	UpdateGUILicense *frame = new UpdateGUILicense(this, "UpdateTool - LICENSE",
	                                               wxPoint(50, 50), wxSize(450, 340),
	                                               false);
	frame->Show(true);
}

void UpdateGUIFrame::OnEditor(wxCommandEvent& event)
{
	/* StatusLog Events: not used atm */
}

void UpdateGUIFrame::OnUpdateFile(wxCommandEvent& event)
{
	wxString log;
	wxFileDialog openFileDialog(this, _("Select Update File"), "", "",
	                            "image files (*.image)|*.image", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	imgEntry->Clear();
	imgEntry->AppendText(openFileDialog.GetPath());
	log = wxString::Format(wxT("Update File: \"%s\""), openFileDialog.GetPath());
	tLog(RTL_DEFAULT, log);
}

void UpdateGUIFrame::OnImportCSV(wxCommandEvent& event)
{
	size_t jobid, jobcount = 0;
	std::vector<UpdateFactory*> uf;
	wxString log;
	std::vector<std::string> error_list;
	wxFileDialog openFileDialog(this, _("Select Update CSV"), "", "",
	                            "image files (*.csv)|*.csv", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (imgEntry->GetValue().empty()) {
		tLog(RTL_RED, "A firmware image is required!");
		return;
	}

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	tLog(RTL_DEFAULT, wxString::Format(wxT("CSV File: \"%s\""), openFileDialog.GetPath()));
	loadUpdateFactoriesFromCSV(openFileDialog.GetPath(), imgEntry->GetValue(), uf, error_list);
	/* check for CSV read errors */
	for (auto& errstr : error_list) {
		tLog(RTL_RED, wxString::Format(wxT("CSV read error: \"%s\""), errstr));
	}

	/* add all csv jobs to the job queue */
	jobid = rand();
	for (auto *u : uf) {
		totalUpdates++;
		jobs->AddJob(Job(Job::eID_THREAD_JOB, JobArgs(jobid, *u)));
		jobid++;
		delete u;
		jobcount++;
	}

	log = wxString::Format(wxT("CSV Import \"%s\": got %u Job(s)"), openFileDialog.GetPath(), jobcount);
	tLog(RTL_GREEN, log);
	SetStatusText(log);
}

void UpdateGUIFrame::OnUpdate(wxCommandEvent& event)
{
	std::vector<UpdateFactory*> uf;
	std::string str;

	if (ipEntry->GetValue().empty()) {
		tLog(RTL_RED, "A hostname is required!");
		return;
	}
	if (imgEntry->GetValue().empty()) {
		tLog(RTL_RED, "A firmware image is required!");
		return;
	}

	str = ipEntry->GetValue();
	/* parse multiple hostname:port combinations */
	loadUpdateFactoriesFromStr(str, imgEntry->GetValue(), pwEntry->GetValue(), uf);
	/* add all jobs to the job queue */
	int jobid = rand();
	for (auto *u : uf) {
		totalUpdates++;
		std::ostringstream log;
		log << "Update started ... (Job: #" << jobid << ")";
		tLog(RTL_DEFAULT, log.str().c_str());

		jobs->AddJob(Job(Job::eID_THREAD_JOB, JobArgs(jobid, *u)));
		delete u; /* do not leak heap memory */
		jobid++;
	}
}

void UpdateGUIFrame::OnNavigationKey(wxNavigationKeyEvent& event)
{
	/* Try to use a wxPanel OR make wxButtons work! */
	auto focused_window = FindFocus()->GetId();
	switch (focused_window) {
		case wxID_IP: pwEntry->SetFocus(); break;
		default: ipEntry->SetFocus(); break;
	}
}

/* return a tuple(avgJobTime[sec], totJobTime[min], estJobTime[min]) */
static std::tuple<unsigned, unsigned, unsigned>
calcJobTimingsAvgTotEst(Queue& jobs, size_t parallelJobCount, JobReport& jobReport, std::vector<unsigned>& jobTimings)
{
	unsigned avg = 0, tot = 0, est;
	unsigned jobTime = jobReport.jobTime;

	jobTimings.push_back(jobTime);
	/* calculate an average/total */
	for (auto& time : jobTimings) {
		avg += time;
		tot += time;
	}
	avg /= jobTimings.size();
	/* Not the best solution since we dont know exactly
	 * how much jobs are running in parallel.
	 */
	tot = (tot / parallelJobCount) / 60;
	est = (avg * (jobs.Stacksize() + jobs.getBusyWorker())) / 60;

	return std::make_tuple(avg, tot, est);
}

void UpdateGUIFrame::OnThread(wxCommandEvent& event)
{
	wxString wxs;
	LogType tp = RTL_DEFAULT;
	static size_t counter = 0;
	static std::vector<unsigned> jobTimings;
	unsigned avgJobTime = 0, totJobTime = 0, estTotalJobTime = 0;
	bool allJobsDone = false;
	JobReport *jobReport = nullptr;

	/* some periodic informational output */
	if ((++counter % 30) == 0) {
		wxs = wxString::Format(wxT("%u jobs done, %u jobs pending, %u jobs running"), jobs->getTotalJobsDone(), jobs->Stacksize(), jobs->getBusyWorker());
		tLog(RTL_DEFAULT, wxs);
	}

	/* process the wx event itself */
	wxs = wxString::Format(wxT("Thread [%i]: %s"), event.GetInt(), event.GetString().c_str());
	switch (event.GetId()) {
		/* case Job::eID_THREAD_JOB: SetStatusText(wxs); break; */
		case Job::eID_THREAD_MSG: break;
		case Job::eID_THREAD_MSGOK: SetStatusText(wxs); tp = RTL_GREEN; break;
		case Job::eID_THREAD_MSGERR: successUpdates--; tp = RTL_RED; break;
		case Job::eID_THREAD_JOB_DONE:
			successUpdates++;
			/* calculate job execution times (total/average/estimated) */
			jobReport = dynamic_cast<JobReport *>(event.GetClientObject());
			std::tie (avgJobTime, totJobTime, estTotalJobTime) = calcJobTimingsAvgTotEst(*jobs, threads.size(), *jobReport, jobTimings);
			/* If more then one job was in the queue
			 * inform the user about job completion,
			 * otherwise reset completed job counter.
			 */
			if (jobs->Stacksize() == 0 && jobs->getBusyWorker() == 0) {
				if (jobs->getTotalJobsDone() > 1)
					allJobsDone = true;
				jobs->resetTotalJobsDone();
			} else {
				SetStatusText(wxString::Format(wxT("Jobs remaining: %u, estimated remaining time: %umin"),
				jobs->Stacksize() + jobs->getBusyWorker(), estTotalJobTime));
			}
			break;
		case Job::eID_THREAD_EXIT:
			/* should only be run once per thread and only if the app exits */
			SetStatusText(wxString::Format(wxT("Thread [%i]: Stopped."), event.GetInt()));
			threads.remove(event.GetInt());
			if (threads.empty()) { this->OnExit(event); }
			break;
		case Job::eID_THREAD_STARTED:
			/* should only be run once per thread and only if the app starts */
			SetStatusText(wxString::Format(wxT("Thread [%i]: Ready."), event.GetInt()));
			break;
		default: event.Skip();
	}

	/* output the wxString in the status log */
	switch (event.GetId()) {
		case Job::eID_THREAD_JOB:
		case Job::eID_THREAD_JOB_DONE:
		case Job::eID_THREAD_MSG:
		case Job::eID_THREAD_MSGOK:
		case Job::eID_THREAD_MSGERR:
			tLog(tp, wxs);
			break;
		default:
			break;
	}

	/* job queue is now empty and had more than one job */
	if (allJobsDone) {
		counter = 0;
		/* we presume that threads.size() jobs ran at the same time */
		wxs = wxString::Format(wxT("All jobs finished. Time consumption: %umin "
		    "(average per device: %umin %usec)"),
		    (unsigned)((float)totJobTime + 0.5f) /* roundup */,
		    avgJobTime / 60, avgJobTime % 60);
		SetStatusText(wxs);
		tLog(RTL_GREEN, wxs);
		/*
		 * The correct size_t format specifier %zu might not
		 * be available on older mingw xcompiler, so use %lu.
		 */
		wxs = wxString::Format(wxT("Total updates: %lu, success: %lu, failed: %lu"),
		    (unsigned long) getTotalUpdates(), (unsigned long) getSuccessUpdates(),
		    (unsigned long) (getTotalUpdates() - getSuccessUpdates()));
		tLog(RTL_DEFAULT, wxs);
		jobTimings.clear();
		totalUpdates = 0;
		successUpdates = 0;
	}
}

#define LICENSE_FILENAME "utool_license_accepted"
static const std::wstring licenses(ALL_LICENSES);

#ifdef WIN32
static DWORD buildLicensePathWin(TCHAR *pathBuf, DWORD pathLen)
{
	DWORD len;

	memset(pathBuf, 0, sizeof(TCHAR) * pathLen);
	len = GetTempPath(pathLen, pathBuf);
	if (len > 0) {
		wcsncat(pathBuf, _T(LICENSE_FILENAME), pathLen - len);
	}
	return len;
}
#endif

bool isLicenseAlreadyAccepted()
{
#ifdef WIN32
	TCHAR path[MAX_PATH];

	if (buildLicensePathWin(&path[0], MAX_PATH) > 0) {
		if (_waccess(path, R_OK) == 0) {
			std::wcerr << std::wstring(&path[0])
			           << ": License already accepted!" << std::endl;
			return true;
		} else {
			std::wcerr << std::wstring(&path[0])
			           << ": License NOT accepted!" << std::endl;
		}
	}
	return false;
#else
	return false;
#endif
}

static void setLicenseAccepted()
{
#ifdef WIN32
	FILE *lf;
	TCHAR path[MAX_PATH];

	if (buildLicensePathWin(&path[0], MAX_PATH) > 0) {
		lf = _wfopen(path, _T("w+t"));
		if (lf)
			fclose(lf);
	}
#endif
}

UpdateGUILicense::UpdateGUILicense(wxWindow *parent, const wxString& title,
                                   const wxPoint& pos, const wxSize& size,
                                   bool showButtons)
	      : wxFrame(parent, wxID_ANY, title, pos, size)
{
	SetBackgroundColour(wxColour(220, 220, 220));
#ifdef WIN32
	SetIcon(wxICON(APPICON));
#endif
	mainVSizer = new wxBoxSizer(wxVERTICAL);
	licBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "License");
	if (showButtons)
		btnBox = new wxStaticBoxSizer(wxHORIZONTAL, this);

	licText       = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
	                               wxDefaultSize,
	                               wxEXPAND | wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
	if (showButtons) {
		acceptButton  = new wxButton(this, wxID_ACCEPT, wxT("Accept"));
		declineButton = new wxButton(this, wxID_DECLINE, wxT("Decline"));
	}

	licBox->Add(licText, 1, wxEXPAND|wxALL, 5);
	if (showButtons) {
		btnBox->AddStretchSpacer();
		btnBox->Add(acceptButton, 0, wxALL, 5);
		btnBox->Add(declineButton, 0, wxALL, 5);
		btnBox->AddStretchSpacer();
	}

	mainVSizer->Add(licBox, 1, wxEXPAND|wxALL, 5);
	if (showButtons)
		mainVSizer->Add(btnBox, 0, wxEXPAND|wxALL|wxBOTTOM, 5);

	SetSizerAndFit(mainVSizer);
	SetInitialSize(wxSize(600, 800));
	Centre();

	licText->AppendText(licenses);
	licText->ShowPosition(0);
	licText->SetSelection(0, 0);
#ifdef WIN32
	licText->HideNativeCaret();
#endif
}

void UpdateGUILicense::OnClose(wxCloseEvent& event)
{
	Destroy();
}

void UpdateGUILicense::OnAccept(wxCommandEvent& event)
{
	setLicenseAccepted();
	UpdateGUIFrame *frame = new UpdateGUIFrame("UpdateTool",
	                                           wxPoint(50, 50), wxSize(450, 340));
	frame->Show(true);
	Destroy();
}

void UpdateGUILicense::OnDecline(wxCommandEvent& event)
{
	_exit(0);
}
