#include "irc2sql.h"

void IRC2SQL::OnShutdown() anope_override
{
	// TODO: test if we really have to use blocking query here
	// (sometimes m_mysql get unloaded before the other thread executed all queries)
	SQL::Result r = this->sql->RunQuery(SQL::Query("CALL " + prefix + "OnShutdown()"));
	quitting = true;
}

void IRC2SQL::OnReload(Configuration::Conf *conf) anope_override
{
	Configuration::Block *block = Config->GetModule(this);
	prefix = block->Get<const Anope::string>("prefix", "anope_");
	UseGeoIP = block->Get<bool>("GeoIPLookup", "no");
	GeoIPDB = block->Get<const Anope::string>("GeoIPDatabase", "country");
	ctcpuser = block->Get<bool>("ctcpuser", "no");
	ctcpeob = block->Get<bool>("ctcpeob", "yes");
	Anope::string engine = block->Get<const Anope::string>("engine");
	this->sql = ServiceReference<SQL::Provider>("SQL::Provider", engine);
	if (sql)
		this->CheckTables();
	else
		Log() << "IRC2SQL: no database connection to " << engine;

	const Anope::string &snick = block->Get<const Anope::string>("client");
	if (snick.empty())
		throw ConfigException(Module::name + ": <client> must be defined");
	StatServ = BotInfo::Find(snick, true);
	if (!StatServ)
		throw ConfigException(Module::name + ": no bot named " + snick);
}

void IRC2SQL::OnNewServer(Server *server) anope_override
{
	query = "INSERT DELAYED INTO `" + prefix + "server` (name, hops, comment, link_time, online, ulined) "
		"VALUES (@name@, @hops@, @comment@, now(), 'Y', @ulined@) "
		"ON DUPLICATE KEY UPDATE name=VALUES(name), hops=VALUES(hops), comment=VALUES(comment), "
			"link_time=VALUES(link_time), online=VALUES(online), ulined=(ulined)";
	query.SetValue("name", server->GetName());
	query.SetValue("hops", server->GetHops());
	query.SetValue("comment", server->GetDescription());
	query.SetValue("ulined", server->IsULined() ? "Y" : "N");
	this->RunQuery(query);
}

void IRC2SQL::OnServerQuit(Server *server) anope_override
{
	if (quitting)
		return;

	query = "UPDATE `" + prefix + "server` "
		"SET currentusers = 0, online = 'N', split_time = now()";
	query.SetValue("name", server->GetName());
	this->RunQuery(query);
}

void IRC2SQL::OnUserConnect(User *u, bool &exempt) anope_override
{
	if (!introduced_myself)
	{
		this->OnNewServer(Me);
		introduced_myself = true;
	}

	query = "CALL " + prefix + "UserConnect(@nick@,@host@,@vhost@,@chost@,@realname@,@ip@,@ident@,@vident@,"
			"@account@,@secure@,@fingerprint@,@signon@,@server@,@uuid@,@modes@,@oper@)";
	query.SetValue("nick", u->nick);
	query.SetValue("host", u->host);
	query.SetValue("vhost", u->vhost);
	query.SetValue("chost", u->chost);
	query.SetValue("realname", u->realname);
	query.SetValue("ip", u->ip);
	query.SetValue("ident", u->GetIdent());
	query.SetValue("vident", u->GetVIdent());
	query.SetValue("secure", u->HasMode("SSL") || u->HasExt("SSL") ? "Y" : "N");
	query.SetValue("account", u->Account() ? u->Account()->display : "");
	query.SetValue("fingerprint", u->fingerprint);
	query.SetValue("signon", u->signon);
	query.SetValue("server", u->server->GetName());
	query.SetValue("uuid", u->GetUID());
	query.SetValue("modes", u->GetModes());
	query.SetValue("oper", u->HasMode("OPER") ? "Y" : "N");
	this->RunQuery(query);

	if (ctcpuser && (Me->IsSynced() || ctcpeob))
		IRCD->SendPrivmsg(StatServ, u->GetUID(), "\1VERSION\1");

}

void IRC2SQL::OnUserQuit(User *u, const Anope::string &msg) anope_override
{
	if (quitting)
		return;

	query = "CALL " + prefix + "UserQuit(@nick@)";
	query.SetValue("nick", u->nick);
	this->RunQuery(query);
}

void IRC2SQL::OnUserNickChange(User *u, const Anope::string &oldnick) anope_override
{
	query = "UPDATE `" + prefix + "user` SET nick=@newnick@ WHERE nick=@oldnick@";
	query.SetValue("newnick", u->nick);
	query.SetValue("oldnick", oldnick);
	this->RunQuery(query);
}

void IRC2SQL::OnFingerprint(User *u) anope_override
{
	query = "UPDATE `" + prefix + "user` SET secure=@secure@, fingerprint=@fingerprint@ WHERE nick=@nick@";
	query.SetValue("secure", u->HasMode("SSL") || u->HasExt("SSL") ? "Y" : "N");
	query.SetValue("fingerprint", u->fingerprint);
	query.SetValue("nick", u->nick);
	this->RunQuery(query);
}

void IRC2SQL::OnUserModeSet(User *u, const Anope::string &mname) anope_override
{
	query = "UPDATE `" + prefix + "user` SET modes=@modes@, oper=@oper@ WHERE nick=@nick@";
	query.SetValue("nick", u->nick);
	query.SetValue("modes", u->GetModes());
	query.SetValue("oper", u->HasMode("OPER") ? "Y" : "N");
	this->RunQuery(query);
}

void IRC2SQL::OnUserModeUnset(User *u, const Anope::string &mname) anope_override
{
	this->OnUserModeSet(u, mname);
}

void IRC2SQL::OnUserLogin(User *u)
{
	query = "UPDATE `" + prefix + "user` SET account=@account@ WHERE nick=@nick@";
	query.SetValue("nick", u->nick);
	query.SetValue("account", u->Account() ? u->Account()->display : "");
	this->RunQuery(query);
}

void IRC2SQL::OnNickLogout(User *u) anope_override
{
	this->OnUserLogin(u);
}

void IRC2SQL::OnSetDisplayedHost(User *u) anope_override
{
	query = "UPDATE `" + prefix + "user` "
		"SET vhost=@vhost@ "
		"WHERE nick=@nick@";
	query.SetValue("vhost", u->GetDisplayedHost());
	query.SetValue("nick", u->nick);
	this->RunQuery(query);
}

void IRC2SQL::OnChannelCreate(Channel *c) anope_override
{
	query = "INSERT INTO `" + prefix + "chan` (channel, topic, topicauthor, topictime, modes) "
		"VALUES (@channel@,@topic@,@topicauthor@,@topictime@,@modes@) "
		"ON DUPLICATE KEY UPDATE channel=VALUES(channel), topic=VALUES(topic),"
			"topicauthor=VALUES(topicauthor), topictime=VALUES(topictime), modes=VALUES(modes)";
	query.SetValue("channel", c->name);
	query.SetValue("topic", c->topic);
	query.SetValue("topicauthor", c->topic_setter);
	query.SetValue("topictime", c->topic_ts);
	query.SetValue("modes", c->GetModes(true,true));
	this->RunQuery(query);
}

void IRC2SQL::OnChannelDelete(Channel *c) anope_override
{
	query = "DELETE FROM `" + prefix + "chan` WHERE channel=@channel@";
	query.SetValue("channel",  c->name);
	this->RunQuery(query);
}

void IRC2SQL::OnJoinChannel(User *u, Channel *c) anope_override
{
	Anope::string modes;
	ChanUserContainer *cu = u->FindChannel(c);
	if (cu)
		modes = cu->status.Modes();

	query = "CALL " + prefix + "JoinUser(@nick@,@channel@,@modes@)";
	query.SetValue("nick", u->nick);
	query.SetValue("channel", c->name);
	query.SetValue("modes", modes);
	this->RunQuery(query);
}

void IRC2SQL::OnLeaveChannel(User *u, Channel *c) anope_override
{
	if (quitting)
		return;

	query = "CALL " + prefix + "PartUser(@nick@,@channel@)";
	query.SetValue("nick", u->nick);
	query.SetValue("channel", c->name);
	this->RunQuery(query);
}

void IRC2SQL::OnTopicUpdated(Channel *c, const Anope::string &user, const Anope::string &topic) anope_override
{
	query = "UPDATE `" + prefix + "chan` "
		"SET topic=@topic@, topicauthor=@author@, topictime=FROM_UNIXTIME(@time@) "
		"WHERE channel=@channel@";
	query.SetValue("topic", c->topic);
	query.SetValue("author", c->topic_setter);
	query.SetValue("time", c->topic_ts);
	query.SetValue("channel", c->name);
	this->RunQuery(query);
}

void IRC2SQL::OnBotNotice(User *u, BotInfo *bi, Anope::string &message) anope_override
{
	Anope::string versionstr;
	if (bi != StatServ)
		return;
	if (message[0] == '\1' && message[message.length() - 1] == '\1')
	{
		if (message.substr(0, 9).equals_ci("\1VERSION "))
		{
			if (u->HasExt("CTCPVERSION"))
				return;
			u->Extend<bool>("CTCPVERSION");

			versionstr = Anope::NormalizeBuffer(message.substr(9, message.length() - 10));
			if (versionstr.empty())
				return;
			query = "UPDATE `" + prefix + "user` "
				"SET version=@version@ "
				"WHERE nick=@nick@";
			query.SetValue("version", versionstr);
			query.SetValue("nick", u->nick);
			this->RunQuery(query);
		}
	}
}

MODULE_INIT(IRC2SQL)
