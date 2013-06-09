#include "irc2sql.h"

void IRC2SQL::OnShutdown() anope_override
{

	// TODO: instead of looping through all users, channels and servers
	//	 call a stored procedure that cleans the database.

	// Test (remove it later)
	for (Anope::map<Server *>::const_iterator it = Servers::ByName.begin(), it_end = Servers::ByName.end(); it != it_end; ++it)
		this->OnServerQuit(it->second);

	// send a blocking query
	SQL::Result r = this->sql->RunQuery(SQL::Query("SELECT 1"));
	quitting = true;
}

void IRC2SQL::OnReload(Configuration::Conf *config) anope_override
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

void IRC2SQL::OnNewServer(Server *server) anope_override
{
	query = "INSERT DELAYED INTO `" + prefix + "servers` (name, hops, comment, link_time, online, ulined) "
		"VALUES (@name@, @hops@, @comment@, unix_timestamp(), 'Y', @ulined@) "
		"ON DUPLICATE KEY UPDATE name=VALUES(name), hops=VALUES(hops), comment=VALUES(comment), link_time=VALUES(link_time), online=VALUES(online), ulined=(ulined)";
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

	query = "INSERT DELAYED INTO `" + prefix + "servers` (name, online, split_time) "
		"VALUES (@name@, 'Y', now()) "
		"ON DUPLICATE KEY UPDATE name=VALUES(name), online=VALUES(online), split_time=VALUES(split_time)";
	query.SetValue("name", server->GetName());
	this->RunQuery(query);
}

void IRC2SQL::OnUserConnect(User *u, bool &exempt)
{
	/* TODO: - add more fields to the user table
	 *	 - make a stored procedure
	 */
	query = "CALL " + prefix + "new_user(@nick@,@ident@,@vident@,@hostname@,@hiddenhostname@,"
		"@nickip@,@realname@,@account@,@fingerprint@,@swhois@);";
	query.SetValue("nick", u->nick);
	query.SetValue("ident", u->GetIdent());
	query.SetValue("vident", u->GetVIdent());
	this->RunQuery(query);
}

MODULE_INIT(IRC2SQL)

