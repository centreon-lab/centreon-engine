/*
** Copyright 2011-2013 Merethis
**
** This file is part of Centreon Engine.
**
** Centreon Engine is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Engine is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Engine. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <exception>
#include "com/centreon/engine/error.hh"
#include "com/centreon/engine/modules/external_commands/commands.hh"
#include "com/centreon/engine/globals.hh"
#include "com/centreon/logging/engine.hh"
#include "test/unittest.hh"

using namespace com::centreon::engine;

/**
 *  Run change_retry_host_check_interval test.
 */
static int check_change_retry_host_check_interval(int argc, char** argv) {
  (void)argc;
  (void)argv;

  host* hst = add_host("name", NULL, NULL, "localhost", NULL, 0, 0.0, 0.0, 42,
                       0, 0, 0, 0, 0, 0.0, 0.0, NULL, 0, NULL, 0, 0, NULL, 0,
                       0, 0.0, 0.0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, NULL,
                       NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0.0, 0.0,
                       0.0, 0, 0, 0, 0, 0);
  if (!hst)
    throw (engine_error() << "create host failed.");

  hst->retry_interval = 0;
  char const* cmd("[1317196300] CHANGE_RETRY_HOST_CHECK_INTERVAL;name;42");
  process_external_command(cmd);

  if (!hst->retry_interval)
    throw (engine_error() << "change_retry_host_check_interval failed.");
  return (0);
}

/**
 *  Init unit test.
 */
int main(int argc, char** argv) {
  unittest utest(argc, argv, &check_change_retry_host_check_interval);
  return (utest.run());
}
