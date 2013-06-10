#include "irc2sql.h"

void IRC2SQL::CheckTables()
{
	/* TODO: remove the DropTable commands when the table layout is done
	 *       perhaps we should drop/recreate some tables by default in case anope crashed
	 *       and was unable to clear the content (ison)
	 *       TRUNCATE could perform better for this?
	 */
	SQL::Result r;
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "clients"));
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "countries"));
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "user"));
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "server"));
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "chan"));
	r = this->sql->RunQuery(SQL::Query("DROP TABLE " + prefix + "ison"));

	this->GetTables();
	/* TODO: do we need this table? we can store the client version in the user table
	 * phpdenora/MagIRC can use triggers or a view to gather this informations
	 */
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
			"`id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
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
	if (!this->HasTable(prefix + "server"))
	{
		query = "CREATE TABLE `" + prefix + "server` ("
			"`id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
			"`name` varchar(64) NOT NULL,"
			"`hops` tinyint(3) NOT NULL,"
			"`comment` varchar(255) NOT NULL,"
			"`link_time` datetime DEFAULT NULL,"
			"`split_time` datetime DEFAULT NULL,"
			"`version` varchar(127) NOT NULL,"
			"`currentusers` int(15) NOT NULL,"
			"`maxusers` int(15) NOT NULL,"
			"`maxusertime` int(15) NOT NULL,"
			"`online` enum('Y','N') NOT NULL DEFAULT 'Y',"
			"`ulined` enum('Y','N') NOT NULL DEFAULT 'N',"
			"PRIMARY KEY (`id`),"
			"UNIQUE KEY `name` (`name`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (!this->HasTable(prefix + "chan"))
	{
		query = "CREATE TABLE `" + prefix + "chan` ("
			"`chanid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
			"`channel` varchar(255) NOT NULL,"
			"`currentusers` int(15) NOT NULL DEFAULT 0,"
			"`topic` varchar(255) DEFAULT NULL,"
			"`topicauthor` varchar(255) DEFAULT NULL,"
			"`topictime` datetime DEFAULT NULL,"
			"`modes` varchar(512) DEFAULT NULL,"
			"PRIMARY KEY (`chanid`),"
			"UNIQUE KEY `channel`(`channel`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (!this->HasTable(prefix + "user"))
	{
		query = "CREATE TABLE `" + prefix + "user` ("
			"`nickid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
			"`nick` varchar(255) NOT NULL DEFAULT '',"
			"`host` varchar(255) NOT NULL DEFAULT '',"
			"`vhost` varchar(255) NOT NULL DEFAULT '',"
			"`chost` varchar(255) NOT NULL DEFAULT '',"
			"`realname` varchar(255) NOT NULL DEFAULT '',"
			"`ip` varchar(255) NOT NULL DEFAULT '',"
			"`ident` varchar(32) NOT NULL DEFAULT '',"
			"`vident` varchar(32) NOT NULL DEFAULT '',"
			"`modes` varchar(255) NOT NULL DEFAULT '',"
			"`account` varchar(255) NOT NULL DEFAULT '',"
			"`fingerprint` varchar(128) NOT NULL DEFAULT '',"
			"`signon` datetime DEFAULT NULL,"
			"`server` varchar(255) NOT NULL DEFAULT '',"
			"`servid` int(11) UNSIGNED NOT NULL DEFAULT '0',"
			"`uuid` varchar(32) NOT NULL DEFAULT '',"
			"`oper` enum('Y','N') NOT NULL DEFAULT 'N',"
			"`away` enum('Y','N') NOT NULL DEFAULT 'N',"
			"`awaymsg` varchar(255) NOT NULL DEFAULT '',"
			"`ctcpversion` varchar(255) NOT NULL DEFAULT '',"
			"`geocode` varchar(16) NOT NULL DEFAULT '',"
			"`geocountry` varchar(64) NOT NULL DEFAULT '',"
			"`geocity` varchar(128) NOT NULL DEFAULT '',"
			"PRIMARY KEY (`nickid`),"
			"UNIQUE KEY `nick` (`nick`),"
			"KEY `servid` (`servid`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (!this->HasTable(prefix + "ison"))
	{
		query = "CREATE TABLE `" + prefix + "ison` ("
			"`nickid` int(11) unsigned NOT NULL default '0',"
			"`chanid` int(11) unsigned NOT NULL default '0',"
			"`modes` varchar(255) NOT NULL default '',"
			"PRIMARY KEY  (`nickid`,`chanid`),"
			"KEY `modes` (`modes`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (this->HasProcedure(prefix + "UserConnect"))
		this->RunQuery(SQL::Query("DROP PROCEDURE " + prefix + "UserConnect"));
	query = "CREATE PROCEDURE `" + prefix + "UserConnect`"
		"(nick_ varchar(255), host_ varchar(255), vhost_ varchar(255), "
		"chost_ varchar(255), realname_ varchar(255), ip_ varchar(255), "
		"ident_ varchar(255), vident_ varchar(255), account_ varchar(255), "
		"fingerprint_ varchar(255), signon_ timestamp, server_ varchar(255), "
		"uuid_ varchar(32), modes_ varchar(255), oper_ enum('Y','N')) "
		"BEGIN "
			"INSERT INTO `" + prefix + "user` "
				"(nick, host, vhost, chost, realname, ip, ident, vident, account, "
				"fingerprint, signon, server, uuid, modes, oper)"
			"VALUES (nick_, host_, vhost_, chost_, realname_, ip_, ident_, vident_, "
				"account_, fingerprint_, signon_, server_, uuid_, modes_, oper_)"
			"ON DUPLICATE KEY UPDATE host=VALUES(host), vhost=VALUES(vhost), "
				"chost=VALUES(chost), realname=VALUES(realname), ip=VALUES(ip), "
				"ident=VALUES(ident), vident=VALUES(vident), account=VALUES(account), "
				"fingerprint=VALUES(fingerprint), signon=VALUES(signon), "
				"server=VALUES(server), uuid=VALUES(uuid), modes=VALUES(modes), "
				"oper=VALUES(oper);"
			"UPDATE `" + prefix + "user` as `u`, `" + prefix + "server` as `s`"
				"SET u.servid = s.id, "
					"s.currentusers = s.currentusers + 1 "
				"WHERE s.name = server_ AND u.nick = nick_;"
		"END";
	this->RunQuery(query);

	if (this->HasProcedure(prefix + "UserQuit"))
		this->RunQuery(SQL::Query("DROP PROCEDURE " +prefix + "UserQuit"));
	query = "CREATE PROCEDURE `" + prefix + "UserQuit`"
		"(nick_ varchar(255)) "
		"BEGIN "
			"UPDATE `" + prefix + "user` as `u`, `" + prefix + "server` as `s` "
				"SET s.currentusers = s.currentusers - 1 "
				"WHERE u.nick=nick_ AND u.servid = s.id; "
			"DELETE FROM `" + prefix + "user` WHERE nick = nick_; "
		"END";
	this->RunQuery(query);

	if (this->HasProcedure(prefix + "ShutDown"))
		this->RunQuery(SQL::Query("DROP PROCEDURE " + prefix + "ShutDown"));
	query = "CREATE PROCEDURE `" + prefix + "ShutDown`()"
		"BEGIN "
			"UPDATE `" +  prefix + "server` "
				"SET currentusers=0, online='N', split_time=now();"
			"TRUNCATE TABLE `" + prefix + "user`;"
			"TRUNCATE TABLE `" + prefix + "chan`;"
			"TRUNCATE TABLE `" + prefix + "ison`;"
		"END";
	this->RunQuery(query);

	if (this->HasProcedure(prefix + "JoinUser"))
		this->RunQuery(SQL::Query("DROP PROCEDURE " + prefix + "JoinUser"));
	query = "CREATE PROCEDURE `"+ prefix + "JoinUser`"
		"(nick_ varchar(255), channel_ varchar(255), modes_ varchar(255)) "
		"BEGIN "
			"INSERT INTO `" + prefix + "ison` (nickid, chanid, modes) "
				"SELECT u.nickid, c.chanid, modes_ "
				"FROM " + prefix + "user AS u, " + prefix + "chan AS c "
				"WHERE u.nick=nick_ AND c.channel=channel_;"
			"UPDATE `" + prefix + "chan` SET currentusers=currentusers+1 "
				"WHERE channel=channel_;"
		"END";
	this->RunQuery(query);

	if (this->HasProcedure(prefix + "PartUser"))
		this->RunQuery(SQL::Query("DROP PROCEDURE " + prefix + "PartUser"));
	query = "CREATE PROCEDURE `" + prefix + "PartUser`"
		"(nick_ varchar(255), channel_ varchar(255)) "
		"BEGIN "
// TODO: finish this query
//			"DELETE FROM `" + prefix + "ison` AS ison "
//				"WHERE 
			"UPDATE `" + prefix + "chan` SET currentusers=currentusers-1 "
				"WHERE channel=channel_;"
		"END";
	this->RunQuery(query);
}
