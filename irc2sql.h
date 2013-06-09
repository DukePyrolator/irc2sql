#include "module.h"
#include "modules/sql.h"

class MySQLInterface : public SQL::Interface
{
 public:
	MySQLInterface(Module *o) : SQL::Interface(o) { }

	void OnResult(const SQL::Result &r) anope_override
	{
	}

	void OnError(const SQL::Result &r) anope_override
	{
		if (!r.GetQuery().query.empty())
			Log(LOG_DEBUG) << "m_irc2sql: Error executing query " << r.finished_query << ": " << r.GetError();
		else
			Log(LOG_DEBUG) << "m_irc2sql: Error executing query: " << r.GetError();
	}
};

class IRC2SQL : public Module
{
	ServiceReference<SQL::Provider> sql;
	MySQLInterface sqlinterface;
	SQL::Query query;
	std::vector<Anope::string> TableList, ProcedureList, EventList;
	Anope::string prefix;
	bool quitting;

	void RunQuery(const SQL::Query &q);
	void GetTables();
	void DropTable(const Anope::string &table);

	bool HasTable(const Anope::string &table);
	bool HasProcedure(const Anope::string &table);
	bool HasEvent(const Anope::string &table);

	void CheckTables();

 public:
	IRC2SQL(const Anope::string &modname, const Anope::string &creator) :
		Module(modname, creator, EXTRA | THIRD), sql("", ""), sqlinterface(this)
	{
		quitting = false;
	}

	void OnShutdown() anope_override;
	void OnReload(Configuration::Conf *config) anope_override;
	void OnNewServer(Server *server) anope_override;
	void OnServerConnect() anope_override;
	void OnServerQuit(Server *server) anope_override;
	void OnUserConnect(User *u, bool &exempt);
};


