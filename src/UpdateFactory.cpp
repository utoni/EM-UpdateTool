#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>

#include "UpdateFactory.hpp"
#include "Csv.hpp"


/* debug only */
#if 0
static std::string toHex(const std::string& s, bool upper_case)
{
	std::ostringstream ret;

	for (std::string::size_type i = 0; i < s.length(); ++i)
		ret << std::hex << std::setfill('0') << std::setw(2) << (upper_case ? std::uppercase : std::nouppercase) << (int)s[i];

	return ret.str();
}
#endif

/* Do not use `const char *` as key type. */
static const std::map<const std::string, const enum EMCVersion> version_map = {
	{ "1.50", EMC_150 },     { "2.04", EMC_204RC6 },
	{ "2.04~rc5", EMC_150 }, { "2.04~rc6", EMC_204RC6 }
};

enum EMCVersion mapEmcVersion(std::string& emc_version)
{
	try {
		return version_map.at(emc_version);
	} catch (std::out_of_range) {
		return EMC_UNKNOWN;
	}
}

static const std::map<const int, const std::string> error_map = {
	{ UPDATE_OK,         "Update succeeded" },
	{ UPDATE_HTTP_ERROR, "HTTP connection failed" },
	{ UPDATE_HTTP_NOT200,"HTTP Error (not an EnergyManager)" },
	{ UPDATE_HTTP_SID,   "No Session ID found (not an EnergyManager)" },
	{ UPDATE_JSON_ERROR, "Invalid JSON HTTP response (not an EnergyManager)" },
	{ UPDATE_AUTH_ERROR, "Authentication failed" },
	{ UPDATE_VERSION,    "Invalid EnergyManager version" },
	{ UPDATE_FILE,       "Could not open update file" }
};

void mapEmcError(int error, std::string& out)
{
	try {
		out = error_map.at(error);
	} catch (std::out_of_range) {
		out = "Unknown";
	}
}

static inline void dump_request(httplib::Request& req)
{
	std::cerr << "http-cli: " << req.method << " "
	          << req.path
	          << " with MIME " << req.get_header_value("Content-Type")
	          << " and SIZE " << req.body.length()
	          << std::endl;
}

static inline void dump_json(json11::Json& json)
{
	std::string str;
	json.dump(str);
	std::cerr << "json: " << (!str.empty() ? str : "empty") << std::endl;
}

inline void dump_class(UpdateFactory *uf)
{
	if (!uf->phpsessid.empty())
		std::cerr << "Session: " << uf->phpsessid << std::endl;
	if (!uf->hostname.empty())
		std::cerr << "Host...: " << uf->hostname << ":" << uf->port << std::endl;
	if (!uf->emc_serial.empty())
		std::cerr << "Serial.: " << uf->emc_serial << std::endl;
	if (!uf->emc_version.empty())
		std::cerr << "Version: " << uf->emc_version << std::endl;
}

void UpdateFactory::setDest(const char *hostname, int port)
{
	cleanup();
	this->hostname = hostname;
	this->port     = port;
	http_client = new httplib::Client(hostname, port);
	phpsessid   = "";
	emc_serial  = "";
	emc_version = "";
	authenticated = false;
	mapped_emc_version = EMC_UNKNOWN;
}

void UpdateFactory::setDest(std::string& hostname, int port)
{
	this->setDest(hostname.c_str(), port);
}

void UpdateFactory::setDest(std::string& hostname, std::string& port)
{
	this->setDest(hostname.c_str(), std::stoi(port));
}

void UpdateFactory::setUpdateFile(const char *update_file)
{
	this->update_file = std::string(update_file);
}

void UpdateFactory::setPass(const char *passwd)
{
	this->passwd = std::string(passwd);
}

void UpdateFactory::setPass(std::string& passwd)
{
	this->setPass(passwd.c_str());
}

static bool grepCookie(const std::string& setcookie, const char *name, std::string& out)
{
	std::string::size_type index;
	std::string prefix = "=";

	if (name) {
		prefix = name;
	}
	index = setcookie.find(prefix, 0);
	if (index == std::string::npos)
		return false;
	index = setcookie.find(';', 0);
	if (index == std::string::npos)
		index = setcookie.length()-1;

	out = setcookie.substr(0, index);
	return true;
}

int UpdateFactory::doAuth()
{
	httplib::Request req;
	httplib::Response res1, res2;
	json11::Json json;
	std::string errmsg;

	if (!http_client)
		return UPDATE_HTTP_ERROR;

	genRequest(req, "/start.php", nullptr);
	if (!doGet(req, res1))
		return UPDATE_HTTP_ERROR;
	if (res1.status != 200)
		return UPDATE_HTTP_NOT200;
	if (!grepCookie(res1.get_header_value("Set-Cookie"), "PHPSESSID", phpsessid))
		return UPDATE_HTTP_SID;
	if (!parseJsonResult(res1, json, errmsg)) {
		return UPDATE_JSON_ERROR;
	}

	dump_json(json);
	emc_serial = json["serial"].string_value();
	emc_version = json["app_version"].string_value();
	mapped_emc_version = mapEmcVersion(emc_version);
	if (mapped_emc_version == EMC_UNKNOWN)
		return UPDATE_VERSION;
	authenticated = json["authentication"].bool_value();

	if (!authenticated) {
		std::ostringstream ostr;
		ostr << "login=" << emc_serial << "&password=" << (passwd.c_str() ? passwd.c_str() : "");

		genRequest(req, "/start.php", ostr.str().c_str());
		if (!doPost(req, res2))
			return UPDATE_HTTP_ERROR;
		if (res2.status != 200)
			return UPDATE_HTTP_NOT200;
		if (!parseJsonResult(res2, json, errmsg))
			return UPDATE_JSON_ERROR;
		dump_json(json);
		authenticated = json["authentication"].bool_value();
	}

	dump_class(this);
	return UPDATE_OK;
}

int UpdateFactory::loadUpdateFile()
{
	std::ifstream input(update_file, std::ios::binary);
	if (!input)
		return UPDATE_FILE;
	std::vector<unsigned char> buffer(
	   (std::istreambuf_iterator<char>(input)),
	   (std::istreambuf_iterator<char>())
	);
	update_buffer = buffer;

	return UPDATE_OK;
}

int UpdateFactory::doUpdate()
{
	httplib::Request req;
	httplib::Response res1, res2;
	json11::Json json;
	std::string errmsg;

	if (!http_client)
		return UPDATE_HTTP_ERROR;
	if (mapped_emc_version == EMC_UNKNOWN)
		return UPDATE_VERSION;
	if (!authenticated)
		return UPDATE_AUTH_ERROR;

	/* Verify: Is this required before update? */
	genRequest(req, "/setup.php?update_cleanup=1", nullptr);
	if (!doGet(req, res1))
		return UPDATE_HTTP_ERROR;
	if (res1.status != 200)
		return UPDATE_HTTP_NOT200;
	if (!parseJsonResult(res1, json, errmsg)) {
		return UPDATE_JSON_ERROR;
	}
	//dump_json(json);

	/* The update process itself. */
	std::ostringstream ostr;
	std::string out;

	ostr << "------WebKitFormBoundaryUPDATETOOL\r\n"
	     << "Content-Disposition: form-data; name=\"update_file\"; filename=\""
	     << update_file << "\"\r\n"
	     << "Content-Type: application/octet-stream\r\n\r\n";
	ostr.write((const char*) &update_buffer[0], update_buffer.size());
	ostr << "\r\n------WebKitFormBoundaryUPDATETOOL\r\n"
	     << "Content-Disposition: form-data; name=\"update_install\"\r\n\r\n\r\n"
	     << "------WebKitFormBoundaryUPDATETOOL--\r\n";
	out = ostr.str();
	genRequest(req, "/mum-webservice/0/update.php", out);
	req.headers.erase("Content-Type");
	req.set_header("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundaryUPDATETOOL");
	if (!doPost(req, res2))
		return UPDATE_HTTP_ERROR;
	if (res2.status != 200)
		return UPDATE_HTTP_NOT200;

	return UPDATE_OK;
}

void UpdateFactory::cleanup()
{
	if (http_client) {
		delete http_client;
		http_client = nullptr;
	}
	authenticated = false;
	emc_serial = "";
	emc_version = "";
}

void UpdateFactory::genRequest(httplib::Request& req, const char *path,
                               const char *body)
{
	std::string b( (body ? body : "") );
	this->genRequest(req, path, b);
}

void UpdateFactory::genRequest(httplib::Request& req, const char *path,
                               std::string& body)
{
	if (!http_client)
		return;

	std::ostringstream ostr;
	ostr << hostname;

	req.headers.clear();
	req.path = path;
	req.set_header("Content-Type", "application/x-www-form-urlencoded");
	req.set_header("Host", ostr.str().c_str());
	if (!phpsessid.empty())
		req.set_header("Cookie", phpsessid.c_str());
	if (!body.empty()) {
		req.body = body;
		req.set_header("Content-Length", std::to_string(req.body.length()).c_str());
	}
}

bool UpdateFactory::doGet(httplib::Request& req, httplib::Response& res)
{
	req.method = "GET";
	dump_request(req);

	return http_client->send(req, res);
}

bool UpdateFactory::doPost(httplib::Request& req, httplib::Response& res)
{
	req.method = "POST";
	dump_request(req);

	if (!http_client)
		return false;
	return http_client->send(req, res);
}

bool UpdateFactory::parseJsonResult(httplib::Response& res, json11::Json& result, std::string& errmsg)
{
	if (res.status != 200) {
		std::ostringstream os;
		os << "HTTP Response Code " << res.status;
		errmsg = os.str();
		return false;
	}

	result = json11::Json::parse(res.body, errmsg);
	return !result.is_null();
}

int loadUpdateFactoriesFromCSV(const char *csv_file, const char *update_file, std::vector<UpdateFactory*>& update_list)
{
	std::vector<int> err_line;
	io::CSVReader<3> in(csv_file);
	in.read_header(io::ignore_extra_column, "hostname", "port", "password");
	std::string hostname, port, passwd;

	try {
		while (in.read_row(hostname, port, passwd)) {
			UpdateFactory *uf = new UpdateFactory();
			uf->setDest(hostname, port);
			uf->setPass(passwd);
			uf->setUpdateFile(update_file);
			update_list.push_back(uf);
		}
	} catch (io::error::with_file_line& err) {
		err_line.push_back(err.file_line);
	} catch (io::error::with_file_name& err) {
	}

	return UPDATE_OK;
}
