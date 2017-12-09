#ifndef UPDATEGUI_H
#define UPDATEGUI_H 1

#include <string>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/richtext/richtextctrl.h>

#include "UpdateFactory.hpp"
#include "JobQueue.hpp"


enum LogType { RTL_DEFAULT, RTL_GREEN, RTL_RED };
enum {
	/* UpdateGUIFrame */
	wxID_EDITOR = wxID_HIGHEST + 1,
	wxID_IP, wxID_PW, wxID_IMG,
	wxID_UPDATEFILE, wxID_IMPORTCSV, wxID_DOUPDATE,
	wxID_LICENSE,
	/* UpdateGUILicense */
	wxID_ACCEPT, wxID_DECLINE
};

class UpdateGUI : public wxApp
{
public:
	virtual bool OnInit();
};

class UpdateGUIFrame: public wxFrame
{
public:
	UpdateGUIFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~UpdateGUIFrame() {}
protected:
	UpdateFactory uf;
private:
	/* log to GUI */
	void tLog(enum LogType type, const char *text, const char *ident=nullptr);
	void tLog(enum LogType type, std::string& text, const char *ident=nullptr);

	/* wxWidgets GUI/Thread events */
	void OnClose(wxCloseEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnLicense(wxCommandEvent& event);
	void OnEditor(wxCommandEvent& event);
	void OnUpdateFile(wxCommandEvent& event);
	void OnImportCSV(wxCommandEvent& event);
	void OnUpdate(wxCommandEvent& event);
	void OnNavigationKey(wxNavigationKeyEvent& event);
	void OnThread(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();

	/* GUI elements */
	wxBoxSizer *mainVSizer;
	wxStaticBoxSizer *ipBox, *pwBox, *imgBox, *subBox, *logBox;
	wxButton *imgButton, *subButton, *csvButton;
	wxTextCtrl *ipEntry, *pwEntry, *imgEntry, *logText;

	/** JobQueue */
	Queue *jobs;
	/** Thread list IDs */
	std::list<int> threads;
};

bool isLicenseAlreadyAccepted();

class UpdateGUILicense: public wxFrame
{
public:
	UpdateGUILicense(wxWindow *parent, const wxString& title,
	                 const wxPoint& pos, const wxSize& size,
	                 bool showButtons = true);
	~UpdateGUILicense() {}
private:
	void OnClose(wxCloseEvent& event);
	void OnAccept(wxCommandEvent& event);
	void OnDecline(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();

	wxBoxSizer *mainVSizer;
	wxStaticBoxSizer *licBox, *btnBox;
	wxButton *acceptButton, *declineButton;
	wxTextCtrl *licText;
};

#endif
