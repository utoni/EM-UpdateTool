#include <cstdio>
#include <iostream>

#include "UpdateFactory.hpp"

#ifdef USE_GUI
#include "UpdateGUI.hpp"

wxIMPLEMENT_APP(UpdateGUI);

#ifdef WIN32
/* Windoze uses UTF-16 as preferred encoding, but is not limited to .. */
#ifdef _UNICODE
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	wxEntryStart(argc, argv);
	wxTheApp->CallOnInit();
	wxTheApp->OnRun();
	return 0;
}
#endif

#else

/* Command Line Interface (CLI) for UNIX/WIN */
int main(int argc, char *argv[])
{
	size_t totalUpdates = 0, successUpdates = 0;
	int rv;
	std::vector<UpdateFactory*> uf;
	std::string errstr, hostPorts;

	if (argc == 0)
		return 1;
	if (argc == 3) {
		std::vector<std::string> error_list;
		rv = loadUpdateFactoriesFromCSV(argv[1], argv[2], uf, error_list);
		if (rv != UPDATE_OK) {
			std::cerr << "CSV file read \"" << argv[1] << "\" failed with: " << rv << std::endl;
			return 1;
		}
		for (auto& errstr : error_list) {
			std::cerr << "CSV read error: " << errstr << std::endl;
		}
	} else
	if (argc == 4) {
		hostPorts = std::string(argv[1]);
		loadUpdateFactoriesFromStr(hostPorts, argv[3], argv[2], uf);
	} else {
		std::cerr << "usage: " << argv[0]
		          << " [update-csv-file]|[[hostname] [password]] [update-file]"
		          << std::endl << std::endl;
		return 1;
	}

	for (auto *u : uf) {
		totalUpdates++;
		std::cerr << "Connecting to '" << u->getHostname() << ":"
		          << u->getPort() << "' with password '"
		          << u->getPassword() << "'" << std::endl;
		rv = u->doAuth();
		mapEmcError(rv, errstr);
		if (rv == UPDATE_OK) {
			std::cerr << "loading file " << u->getUpdateFile() << std::endl;
			rv = u->loadUpdateFile();
			std::cerr << "firmware version: " << mapEmcVersion(u->getFwVersion()) << std::endl;
			if (rv == UPDATE_OK) {
				if (!isEmcVersionLowerThen(u->getEmcVersion(), u->getFwVersion())) {
					std::cerr << "version mismatch (" << mapEmcVersion(u->getEmcVersion())
					    << " >= " << mapEmcVersion(u->getFwVersion()) << ")" << std::endl;
					continue;
				}
				std::cerr << "uploading file " << u->getUpdateFile() << std::endl;
				rv = u->doUpdate();
				if (rv == UPDATE_OK) {
					successUpdates++;
					std::cerr << "Update succeeded!" << std::endl;
				} else {
					mapEmcError(rv, errstr);
					std::cerr << "doUpdate returned " << rv << ": " << errstr << std::endl;
				}
			} else {
				mapEmcError(rv, errstr);
				std::cerr << "load file returned " << rv << ": " << errstr << std::endl;
			}
		} else std::cerr << "doAuth returned " << rv << ": " << errstr << std::endl;
	}

	for (auto *u : uf) {
		delete u;
	}

	std::cout << "-----------------------" << std::endl
	          << "updates: " << totalUpdates << std::endl
	          << "success: " << successUpdates << std::endl
	          << "failed.: " << (totalUpdates - successUpdates) << std::endl
	          << std::endl;

	return 0;
}

/* Our Command Line Interface (CLI) wants UTF-16 support if target platform is Windoze .. */
#if defined(_UNICODE) && defined(WIN32)
int wmain(int argc, wchar_t* wargv[])
{
	size_t len;
	static char **argv = new char*[argc];

	/* convert wide character argvector to ASCII, dirty .. */
	for (int i = 0; i < argc; ++i) {
		len = wcslen(wargv[i]) * sizeof(wchar_t);
		argv[i] = (char *) calloc(len+1, sizeof(char));
		wcstombs(argv[i], wargv[i], len);
		fprintf(stderr, "arg[%d]: %s\n", i, argv[i]);
	}
	return main(argc, argv);
}
#endif
#endif
