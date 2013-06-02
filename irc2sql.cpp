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

class M_irc2sql : public Module
{
	ServiceReference<SQL::Provider> sql;
	MySQLInterface sqlinterface;
	SQL::Query query;
	std::vector<Anope::string> TableList, ProcedureList, EventList;
	Anope::string prefix;

	void RunQuery(const SQL::Query &q)
	{
		if (sql)
			sql->Run(&sqlinterface, q);
	}

	void GetTables()
	{
		TableList.clear();
		ProcedureList.clear();
		EventList.clear();
		if (!sql)
			return;

		SQL::Result r = this->sql->RunQuery(this->sql->GetTables(prefix));
		for (int i = 0; i < r.Rows(); ++i)
		{
			const std::map<Anope::string, Anope::string> &map = r.Row(i);
			for (std::map<Anope::string, Anope::string>::const_iterator it = map.begin(); it != map.end(); ++it)
				TableList.push_back(it->second);
		}
		query = "SHOW PROCEDURE STATUS WHERE `Db` = Database();";
		r = this->sql->RunQuery(query);
		for (int i = 0; i < r.Rows(); ++i)
		{
			ProcedureList.push_back(r.Get(i, "Name"));
		}
		query = "SHOW EVENTS WHERE `Db` = Database();";
		r = this->sql->RunQuery(query);
		for (int i = 0; i < r.Rows(); ++i)
		{
			EventList.push_back(r.Get(i, "Name"));
		}
	}

	bool HasTable(const Anope::string &table)
	{
		for (std::vector<Anope::string>::const_iterator it = TableList.begin(); it != TableList.end(); ++it)
			if (*it == table)
				return true;
		return false;
	}

	bool HasProcedure(const Anope::string &table)
	{
		for (std::vector<Anope::string>::const_iterator it = ProcedureList.begin(); it != ProcedureList.end(); ++it)
			if (*it == table)
				return true;
		return false;
	}

	bool HasEvent(const Anope::string &table)
	{
		for (std::vector<Anope::string>::const_iterator it = EventList.begin(); it != EventList.end(); ++it)
			if (*it == table)
				return true;
		return false;
	}


	void CheckTables()
	{
		this->GetTables();
		// TODO: do we need this table? could probably selected into a virtual table from the php script
		if (!this->HasTable(prefix + "clients"))
		{
			query = "CREATE TABLE `" + prefix + "clients` ("
				"`id` int(11) NOT NULL AUTO_INCREMENT,"
				"`version` varchar(255) NOT NULL,"
				"`count` int(11) NOT NULL,"
				"`overall` int(11) NOT NULL,"
				"PRIMARY KEY (`id`),"
				"UNIQUE KEY `version` (`version`)"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
			this->RunQuery(query);
		}
		/* do we need the country table?
		 * we can do the geoiplookup via an SQL trigger and use the geoipdatabase from
		 * http://geolite.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip or
		 * http://geolite.maxmind.com/download/geoip/database/GeoLiteCity_CSV/GeoLiteCity-latest.zip
		 * and import the .csv into mysql via an external script.
		 * this should not be part of the irc2sql gateway. could be a part of MagIRC
		 */
		if (!this->HasTable(prefix + "countries"))
		{
			query = "CREATE TABLE `" + prefix + "countries` ("
				"`id` int(11) NOT NULL AUTO_INCREMENT,"
				"`code` varchar(3) NOT NULL,"
				"`name` varchar(32) NOT NULL,"
				"`count` int(11) NOT NULL,"
				"`overall` int(11) NOT NULL,"
				"PRIMARY KEY (`id`),"
				"UNIQUE KEY `code` (`code`),"
				"UNIQUE KEY `name` (`name`)"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
			this->RunQuery(query);
		}
		if (!this->HasTable(prefix + "servers"))
		{
			query = "CREATE TABLE `" + prefix + "servers` ("
				"`id` int(11) NOT NULL AUTO_INCREMENT,"
				"`name` varchar(64) NOT NULL,"
				"`hops` tinyint(3) NOT NULL,"
				"`comment` varchar(255) NOT NULL,"
				"`linked_id` int(11) DEFAULT NULL,"
				"`link_time` int(11) DEFAULT NULL,"
				"`online` tinyint(1) NOT NULL DEFAULT '1',"
				"`split_time` int(11) DEFAULT NULL,"
				"`version` varchar(127) NOT NULL,"
				"`uptime` mediumint(9) NOT NULL,"
				"`motd` text NOT NULL,"
				"`users_current` smallint(6) NOT NULL,"
				"`users_max` smallint(6) NOT NULL,"
				"`users_max_time` int(11) NOT NULL,"
				"`ping` smallint(6) NOT NULL,"
				"`ping_max` smallint(6) NOT NULL,"
				"`ping_time` int(11) NOT NULL,"
				"`ping_max_time` int(11) NOT NULL,"
				"`ulined` tinyint(1) NOT NULL DEFAULT '0',"
				"`kills_ircops` mediumint(9) NOT NULL,"
				"`kills_server` mediumint(9) NOT NULL,"
				"`opers` tinyint(4) NOT NULL,"
				"`opers_max` tinyint(4) NOT NULL,"
				"`opers_max_time` int(11) NOT NULL,"
				"PRIMARY KEY (`id`),"
				"UNIQUE KEY `name` (`name`),"
				"KEY `linked_id` (`linked_id`)"
				") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
			this->RunQuery(query);
		}
	}


 public:
	M_irc2sql(const Anope::string &modname, const Anope::string &creator) :
		Module(modname, creator, EXTRA | THIRD), sql("", ""), sqlinterface(this)
	{
	}

	void OnReload(Configuration::Conf *config) anope_override
	{
		Configuration::Block *block = config->GetModule(this);

		prefix = block->Get<const Anope::string>("prefix", "anope_");
		Anope::string engine = block->Get<const Anope::string>("engine");
		this->sql = ServiceReference<SQL::Provider>("SQL::Provider", engine);
		if (sql)
			this->CheckTables();
			else
			Log() << "m_irc2sql: no database connection to " << engine;
	}

	void OnNewServer(Server *server) anope_override
	{
		query = "INSERT DELAYED INTO `" + prefix + "servers` (name, hops, comment, link_time, online, ulined) "
			"VALUES (@name@, @hops@, @comment@, unix_timestamp(), '1', @ulined@) "
			"ON DUPLICATE KEY UPDATE name=VALUES(name), hops=VALUES(hops), comment=VALUES(comment), link_time=VALUES(link_time), online=VALUES(online), ulined=(ulined)";
		query.SetValue("name", server->GetName());
		query.SetValue("hops", server->GetHops());
		query.SetValue("comment", server->GetDescription());
		query.SetValue("ulined", server->IsULined() ? "1" : "0");
		this->RunQuery(query);
	}

	/* Called when anope connects to its uplink
	 * We can not use OnNewServer to get our own server struct because this event
	 * is triggered before module load
	 */
	void OnServerConnect() anope_override
	{
		this->OnNewServer(Me);
	}

	void OnServerQuit(Server *server) anope_override
	{
		query = "INSERT DELAYED INTO `" + prefix + "servers` (name, online, split_time) "
			"VALUES (@name@, '0', unix_timestamp()) "
			"ON DUPLICATE KEY UPDATE name=VALUES(name), online=VALUES(online), split_time=VALUES(split_time)";
		query.SetValue("name", server->GetName());
		this->RunQuery(query);
	}

};


MODULE_INIT(M_irc2sql)

