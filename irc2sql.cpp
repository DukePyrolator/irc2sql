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
	Anope::string engine = block->Get<const Anope::string>("engine");
	this->sql = ServiceReference<SQL::Provider>("SQL::Provider", engine);
	if (sql)
		this->CheckTables();
	else
		Log() << "IRC2SQL: no database connection to " << engine;
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

/* Called when anope connects to its uplink
 * We can not use OnNewServer to get our own server struct because this event
 * is triggered before module load
 */
void IRC2SQL::OnServerConnect() anope_override
{
	this->OnNewServer(Me);
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

void IRC2SQL::OnUserConnect(User *u, bool &exempt)
{
	query = "CALL " + prefix + "UserConnect(@nick@,@host@,@vhost@,@chost@,@realname@,@ip@,@ident@,@vident@,"
			"@account@,@fingerprint@,@signon@,@server@,@uuid@,@modes@)";
	query.SetValue("nick", u->nick);
	query.SetValue("host", u->host);
	query.SetValue("vhost", u->vhost);
	query.SetValue("chost", u->chost);
	query.SetValue("realname", u->realname);
	query.SetValue("ip", u->ip);
	query.SetValue("ident", u->GetIdent());
	query.SetValue("vident", u->GetVIdent());
	query.SetValue("account", u->Account() ? u->Account()->display : "");
	query.SetValue("fingerprint", u->fingerprint);
	query.SetValue("signon", u->signon);
	query.SetValue("server", u->server->GetName());
	query.SetValue("uuid", u->GetUID());
	query.SetValue("modes", u->GetModes());
	this->RunQuery(query);
}

void IRC2SQL::OnUserQuit(User *u, const Anope::string &msg)
{
	if (quitting)
		return;

	query = "CALL " + prefix + "UserQuit(@nick@)";
	query.SetValue("nick", u->nick);
	this->RunQuery(query);
}

MODULE_INIT(IRC2SQL)

