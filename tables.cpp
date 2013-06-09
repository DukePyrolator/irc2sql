#include "irc2sql.h"

void IRC2SQL::CheckTables()
{
	// TODO: remove the DropTable commands when the module is done
	this->DropTable(prefix + "clients");
	this->DropTable(prefix + "countries");
	this->DropTable(prefix + "user");
	this->DropTable(prefix + "servers");
	this->DropTable(prefix + "chan");

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
	if (!this->HasTable(prefix + "servers"))
	{
		query = "CREATE TABLE `" + prefix + "servers` ("
			"`id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
			"`name` varchar(64) NOT NULL,"
			"`hops` tinyint(3) NOT NULL,"
			"`comment` varchar(255) NOT NULL,"
			"`linked_id` int(11) DEFAULT NULL,"
			"`link_time` int(11) DEFAULT NULL,"
			"`online` tinyint(1) NOT NULL DEFAULT '1',"
			"`split_time` int(11) DEFAULT NULL,"
			"`version` varchar(127) NOT NULL,"
			"`uptime` mediumint(9) NOT NULL,"         // remove or keep?
			"`motd` text NOT NULL,"
			"`users_current` smallint(6) NOT NULL,"   // remove or use triggers to fill it?
			"`users_max` smallint(6) NOT NULL,"       // remove or use triggers to fill it?
			"`users_max_time` int(11) NOT NULL,"      // remove this field ?
			"`ulined` tinyint(1) NOT NULL DEFAULT '0',"
			"`opers` tinyint(4) NOT NULL,"            // remove this field ?
			"`opers_max` tinyint(4) NOT NULL,"        // remove this field ?
			"`opers_max_time` int(11) NOT NULL,"      // remove this field ?
			"PRIMARY KEY (`id`),"
			"UNIQUE KEY `name` (`name`),"
			"KEY `linked_id` (`linked_id`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (!this->HasTable(prefix + "chan"))
	{
		query = "CREATE TABLE `" + prefix + "chan` ("
			"`chanid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,"
			"`channel` varchar(255) NOT NULL,"
			"`currentusers` int(15) NOT NULL DEFAULT 0,"
			"`maxusers` int(15) NOT NULL DEFAULT 0,"
			"`topic` varchar(255) DEFAULT NULL,"
			"`topicauthor` varchar(255) DEFAULT NULL,"
			"`topictime` timestamp DEFAULT NULL,"
			"`modes` varchar(512) DEFAULT NULL,"
			"PRIMARY KEY (`chanid`),"
			"UNIQUE KEY `channel`(`channel`),"
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
			"`account` varchar(255) NOT NULL DEFAULT '',"
			"`fingerprint` varchar(128) NOT NULL DEFAULT '',"
			"`signon` timestamp NOT NULL DEFAULT '0',"
			"`server` varchar(255) NOT NULL DEFAULT '',"
			"`servid` int(11) UNSIGNED NOT NULL DEFAULT '0',"
			"`uuid` varchar(32) UNSIGNED NOT NULL DEFAULT '',"
			"`away` enum('Y','N') NOT NULL DEFAULT 'N',"
			"`awaymsg` varchar(255) NOT NULL DEFAULT '',"
			"`ctcpversion` varchar(255) NOT NULL DEFAULT '',"
			"`online` enum('Y','N') NOT NULL DEFAULT 'Y',"
			"`geocode` varchar(16) NOT NULL DEFAULT '',"
			"`geocountry` varchar(64) NOT NULL DEFAULT '',"
			"`geocity` varchar(128) NOT NULL DEFAULT '',"
			"PRIMARY KEY (`nickid`),"
			"UNIQUE KEY `nick` (`nick`),"
			"KEY `servid` (`servid`),"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
		this->RunQuery(query);
	}
	if (!this->HasProcedure(prefix + "OnUserConnect"))
	{
		this->RunQuery(SQL::Query("DROP PROCEDURE " + prefix + "OnUserConnect"));
	}
	query = "CREATE PROCEDURE `" + prefix + "proc_OnUserConnect`"
		"(nick_ varchar(255), host_ varchar(255), vhost_ varchar(255), "
		"chost_ varchar(255), realname_ varchar(255), ip_ varchar(255), "
		"ident_ varchar(255), vident_ varchar(255), account_ varchar(255), "
		"fingerprint_ varchar(255), signon_ timestamp, server_ varchar(255), "
		"uuid_ varchar(32)) "
		"BEGIN "
			"INSERT INTO `" + prefix + "users` "
				"(nick, host, vhost, chost, realname, ip, ident, vident, account, "
				"fingerprint, signon, server, uuid, online)"
			"VALUES (nick_, host_, vhost_, chost_, realname_, ip_, ident_, vident_, "
				"account_, fingerprint_, signon_, server_, uuid_, 'Y')"
			"ON DUPLICATE KEY UPDATE host=VALUES(host), vhost=VALUES(vhost), "
				"chost=VALUES(chost), realname=VALUES(realname), ip=VALUES(ip), "
				"ident=VALUES(ident), vident=VALUES(vident), account=VALUES(account), "
				"fingerprint=VALUES(fingerprint), signon=VALUES(signon), "
				"server=VALUES(server), uuid=VALUES(uuid), online=VALUES(online);"
			"UPDATE `" + prefix + "users`, `" + prefix + "servers`"
				"SET users.servid = servers.id, "
					"servers.users_current = servers.users_current + 1 "
				"WHERE servers.name = server_ AND users.nick = nick_;"
		"END";
	this->RunQuery(query);
}
