#include <cstdio>
#include <iostream>

#include "UpdateFactory.hpp"

#ifdef USE_GUI
#include "UpdateGUI.hpp"

wxIMPLEMENT_APP(UpdateGUI);

#if defined(_UNICODE) && defined(WIN32)
int wmain(int argc, wchar_t* wargv[])
{
	wxEntryStart(argc, wargv);
	wxTheApp->CallOnInit();
	wxTheApp->OnRun();
	return 0;
}
#endif

#else

int main(int argc, char **argv)
{
	int rv;
	std::vector<UpdateFactory*> uf;
	std::string errstr;

	if (argc == 0)
		return 1;
	if (argc == 3) {
		uf.clear();
		rv = loadUpdateFactoriesFromCSV(argv[1], argv[2], uf);
		if (rv != UPDATE_OK) {
			std::cerr << "CSV file read \"" << argv[1] << "\" failed with: " << rv << std::endl;
			return 1;
		}
	} else
	if (argc == 4) {
		uf.push_back(new UpdateFactory());
		uf[0]->setDest(argv[1], 80);
		uf[0]->setPass(argv[2]);
		uf[0]->setUpdateFile(argv[3]);
	} else {
		std::cerr << "Missing CLI arguments, using defaults .." << std::endl
		          << "usage: " << argv[0]
		          << " [update-csv-file]|[[hostname] [password]] [update-file]"
		          << std::endl << std::endl;
		return 1;
	}

	for (auto *u : uf) {
		rv = u->doAuth();
		mapEmcError(rv, errstr);
		std::cerr << "doAuth returned " << rv << ": " << errstr << std::endl;
		if (rv == UPDATE_OK) {
			std::cerr << "uploading file " << u->getUpdateFile() << std::endl;
			rv = u->loadUpdateFile();
			mapEmcError(rv, errstr);
			std::cerr << "load file returned " << rv << ": " << errstr << std::endl;
			rv = u->doUpdate();
			mapEmcError(rv, errstr);
			std::cerr << "doUpdate returned " << rv << ": " << errstr << std::endl;
		}
	}

	for (auto *u : uf) {
		delete u;
	}
	return 0;
}

#if defined(_UNICODE) && defined(WIN32)
int wmain(int argc, wchar_t* wargv[])
{
	size_t len;
	static char **argv = new char*[argc];

	/* convert wide character argvector to ASCII */
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
