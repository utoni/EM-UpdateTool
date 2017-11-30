#ifndef UPDATE_FACTORY_H
#define UPDATE_FACTORY_H 1

#include <string>
#include <vector>

#include "Http.hpp"
#include "Json.hpp"

#define UPDATE_OK          0
#define UPDATE_HTTP_ERROR  1
#define UPDATE_HTTP_NOT200 2
#define UPDATE_HTTP_SID    3
#define UPDATE_JSON_ERROR  4
#define UPDATE_AUTH_ERROR  5
#define UPDATE_VERSION     6
#define UPDATE_UPDATED     7
#define UPDATE_FILE        8


enum EMCVersion {
	EMC_130, EMC_131, EMC_132, EMC_133, EMC_134, EMC_135,
	EMC_136, EMC_137, EMC_138, EMC_139,
	EMC_140, EMC_141, EMC_142, EMC_143, EMC_144, EMC_145,
	EMC_146, EMC_147, EMC_148, EMC_149,
	EMC_150, EMC_1501,
	EMC_200, EMC_201, EMC_202, EMC_203, EMC_204,
	EMC_204RC5, EMC_204RC6, /* only for testing */
	EMC_UNKNOWN
};

enum EMCVersion mapEmcVersion(std::string& emc_version);
std::string mapEmcVersion(enum EMCVersion ver);
bool isEmcVersionLowerThen(enum EMCVersion ver, enum EMCVersion check_ver);

void mapEmcError(int error, std::string& out);

class UpdateFactory
{
public:
	explicit UpdateFactory() {}
	UpdateFactory(const UpdateFactory&) = delete;
	~UpdateFactory() { cleanup(); }

	void setDest(const char *hostname, int port);
	void setDest(std::string& hostname, int port);
	void setDest(std::string& hostname, std::string& port);
	void setUpdateFile(const char *update_file);
	void setPass(const char *passwd);
	void setPass(std::string& passwd);
	const char *getUpdateFile() const { return this->update_file.c_str(); }
	const char *getHostname() const { return this->hostname.c_str(); }
	const char *getPassword() const { return this->passwd.c_str(); }
	enum EMCVersion getEmcVersion() const { return this->mapped_emc_version; }
	enum EMCVersion getFwVersion() const { return this->mapped_firmware_version; }
	int getPort() const { return this->port; }
	int doAuth();
	int loadUpdateFile();
	int doUpdate();
	friend void dump_class(UpdateFactory *uf);
protected:
	std::string phpsessid;
	std::string emc_serial;
	bool authenticated;
	enum EMCVersion mapped_emc_version, mapped_firmware_version;
	std::string update_file;
	std::string passwd;
	std::vector<unsigned char> update_buffer;
private:
	void cleanup();
	void genRequest(httplib::Request& req, const char *path,
	                const char *body);
	void genRequest(httplib::Request& req, const char *path,
	                std::string& body);
	bool doGet(httplib::Request& req, httplib::Response& res);
	bool doPost(httplib::Request& req, httplib::Response& res);
	bool parseJsonResult(httplib::Response& res, json11::Json& result, std::string& errmsg);

	httplib::Client *http_client = nullptr;
	std::string hostname;
	int port;
};

int loadUpdateFactoriesFromCSV(const char *csv_file, const char *update_file, std::vector<UpdateFactory*>& update_list);

void loadUpdateFactoriesFromStr(std::string& hostPorts, const char *update_file, const char *password, std::vector<UpdateFactory*>& update_list);

#endif
