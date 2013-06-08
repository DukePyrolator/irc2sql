irc2sql
=======

This module for anope ( www.anope.org ) provides an irc2sql gateway.
It collects data about users, channels and servers. It doesn't build stats
itself, however, it gives you the database, it's up to you how you use it.

Its designed to be light and fast. The sql scheme is improved, dont expect it
to work with current phpdenora/MagIRC installations.

CODING GUIDELINES
==================
- try to avoid SELECTS. If not possible, move the SELECT and all depending
  queries to a stored procedure.

  EXAMPE FOR BAD CODING:
  ----------------------

        SELECT `id` FROM server WHERE `servername` = 'irc.anope.org';

        INSERT INTO `users` (`nick`, `servid`)
         VALUES ('DukePyrolator', '<result from first query>');


 EXAMLE FOR BETTER CODING:
 -------------------------

       INSERT INTO `users` (`nick`, `servid`)
         SELECT 'DukePyrolator', id FROM servers
          WHERE name = 'irc.anope.org';

 In the second example we are sending only one query to the mysqld
 and dont have to wait for an answer.
