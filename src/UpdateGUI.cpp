#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "UpdateGUI.hpp"
#include "JobQueue.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <wx/aboutdlg.h>


wxBEGIN_EVENT_TABLE(UpdateGUIFrame, wxFrame)
	EVT_CLOSE(UpdateGUIFrame::OnClose)

	EVT_MENU(wxID_EXIT,       UpdateGUIFrame::OnExit)
	EVT_MENU(wxID_ABOUT,      UpdateGUIFrame::OnAbout)
	EVT_MENU(wxID_EDITOR,     UpdateGUIFrame::OnEditor)
	EVT_MENU(wxID_UPDATEFILE, UpdateGUIFrame::OnUpdateFile)
	EVT_MENU(wxID_DOUPDATE,   UpdateGUIFrame::OnUpdate)

	EVT_BUTTON(wxID_UPDATEFILE, UpdateGUIFrame::OnUpdateFile)
	EVT_BUTTON(wxID_IMPORTCSV,  UpdateGUIFrame::OnImportCSV)
	EVT_BUTTON(wxID_DOUPDATE,   UpdateGUIFrame::OnUpdate)

	EVT_COMMAND(wxID_ANY, wxEVT_THREAD, UpdateGUIFrame::OnThread)
wxEND_EVENT_TABLE()


bool UpdateGUI::OnInit()
{
	UpdateGUIFrame *frame = new UpdateGUIFrame("UpdateTool",
	                                           wxPoint(50, 50), wxSize(450, 340));
	frame->Show(true);
	return true;
}

UpdateGUIFrame::UpdateGUIFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
          : wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_UPDATEFILE,
	                 "&Select Image ...\tCtrl-F",
	                 "Select a firmware image.");
	menuFile->Append(wxID_DOUPDATE,
	                 "&Submit\tCtrl-U",
	                 "Start the firmware update process.");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

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
	    wxDefaultSize, wxTE_PROCESS_TAB);
	ipBox->Add(ipEntry, 1, wxEXPAND|wxALL, 5);
	pwEntry   = new wxTextCtrl(this, wxID_PW, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_PASSWORD | wxTE_PROCESS_TAB);
	pwBox->Add(pwEntry, 1, wxEXPAND|wxALL, 5);
	imgEntry  = new wxTextCtrl(this, wxID_IMG, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_READONLY
	);
	imgBox->Add(imgEntry, 1, wxALIGN_CENTER|wxALL, 5);
	logText   = new wxTextCtrl(
	    this, wxID_EDITOR, wxEmptyString, wxDefaultPosition,
	    wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2
	);
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
		for (int tid = 1; tid <= 2; ++tid) {
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
	for (unsigned i = 0; i < threads.size(); ++i) {
		jobs->AddJob(Job(Job::eID_THREAD_EXIT, JobArgs()), Queue::eHIGHEST);
	}
	if (!threads.empty()) return;

	for (auto *p : {ipEntry,pwEntry,imgEntry,logText}) { p->Destroy(); }
	imgButton->Destroy();
	subButton->Destroy();
	for (auto *p : {ipBox,pwBox,imgBox,subBox,logBox}) { p->Clear(); }
	mainVSizer->Clear();
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
#ifndef PACKAGE_URL
#define PACKAGE_URL "https://some_download_website.tld"
#endif

void UpdateGUIFrame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(PACKAGE_NAME);
	aboutInfo.SetVersion(PACKAGE_VERSION);
	aboutInfo.SetDescription("A simple firmware update tool.");
	aboutInfo.SetCopyright("(C) 2017");
	aboutInfo.SetWebSite(PACKAGE_URL);
	aboutInfo.AddDeveloper("Toni Uhlig");
	aboutInfo.AddDeveloper("Valeri Budjko");
	wxAboutBox(aboutInfo, this);
}

void UpdateGUIFrame::OnEditor(wxCommandEvent& event)
{
}

void UpdateGUIFrame::OnUpdateFile(wxCommandEvent& event)
{
	wxString log;
	wxFileDialog openFileDialog(this, _("Select Update File"), "", "",
	                            "image files (*.image)|*.image", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		openFileDialog.Destroy();
		return;
	}

	imgEntry->Clear();
	imgEntry->AppendText(openFileDialog.GetPath());
	log = wxString::Format(wxT("Update File: \"%s\""), openFileDialog.GetPath());
	tLog(RTL_DEFAULT, log);

	openFileDialog.Destroy();
}

void UpdateGUIFrame::OnImportCSV(wxCommandEvent& event)
{
	int rv;
	std::vector<UpdateFactory*> uf;
	std::string err;
	wxString log;
	wxFileDialog openFileDialog(this, _("Select Update CSV"), "", "",
	                            "image files (*.csv)|*.csv", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		openFileDialog.Destroy();
		return;
	}

	log = wxString::Format(wxT("CSV File: \"%s\""), openFileDialog.GetPath());
	tLog(RTL_DEFAULT, log);
	rv = loadUpdateFactoriesFromCSV(openFileDialog.GetPath(), imgEntry->GetValue(), uf);
	if (rv != UPDATE_OK) {
		mapEmcError(rv, err);
		tLog(RTL_RED, wxString::Format(wxT("CSV parse failed: \"%s\""), err));
	}
	for (auto *u : uf) {
		int jobid = rand();
		jobs->AddJob(Job(Job::eID_THREAD_JOB, JobArgs(jobid, *u)));
		delete u;
	}
	SetStatusText(wxString::Format(wxT("CSV Import %s"), openFileDialog.GetPath()));
	openFileDialog.Destroy();
}

void UpdateGUIFrame::OnUpdate(wxCommandEvent& event)
{
	int jobid = rand();
	std::vector<UpdateFactory*> uf;
	std::string str;

	str = ipEntry->GetValue();
	loadUpdateFactoriesFromStr(str, imgEntry->GetValue(), pwEntry->GetValue(), uf);
	for (auto *u : uf) {
		std::ostringstream log;
		log << "Update started ... (Job: #" << jobid << ")";
		tLog(RTL_DEFAULT, log.str().c_str());

		jobs->AddJob(Job(Job::eID_THREAD_JOB, JobArgs(jobid, *u)));
		delete u;
	}
	SetStatusText(wxT("Jobs started."));
}

void UpdateGUIFrame::OnThread(wxCommandEvent& event)
{
	wxString wxs;
	LogType tp = RTL_DEFAULT;

	switch (event.GetId()) {
		case Job::eID_THREAD_JOB:
		case Job::eID_THREAD_MSG:
		case Job::eID_THREAD_MSGOK:
		case Job::eID_THREAD_MSGERR:
			wxs = wxString::Format(wxT("Thread [%i]: \"%s\""), event.GetInt(), event.GetString().c_str());

			switch (event.GetId()) {
				case Job::eID_THREAD_JOB: SetStatusText(wxs); break;
				case Job::eID_THREAD_MSGOK: tp = RTL_GREEN; break;
				case Job::eID_THREAD_MSGERR: tp = RTL_RED; break;
			}
			tLog(tp, wxs);
			break;
		case Job::eID_THREAD_EXIT:
			SetStatusText(wxString::Format(wxT("Thread [%i]: Stopped."), event.GetInt()));
			threads.remove(event.GetInt());
			if (threads.empty()) { this->OnExit(event); }
			break;
		case Job::eID_THREAD_STARTED:
			SetStatusText(wxString::Format(wxT("Thread [%i]: Ready."), event.GetInt()));
			break;
		default: event.Skip();
	}
}
