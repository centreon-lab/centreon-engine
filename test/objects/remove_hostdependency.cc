/*
** Copyright 2011 Merethis
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

#include <QDebug>
#include <exception>
#include "error.hh"
#include "objects.hh"
#include "utils.hh"
#include "macros.hh"
#include "globals.hh"

using namespace com::centreon::engine;

/**
 *  Check if remove hostdependency works with some hostdependency.
 */
static void remove_all_hostdependency() {
  init_object_skiplists();

  add_host_dependency("host_dependency_dependent_host_name_1",
		      "host_dependency_host_name_1",
		      0, 0, 0, 0, 0, 0,
		      "host_dependency_dependency_period");
  add_host_dependency("host_dependency_dependent_host_name_2",
		      "host_dependency_host_name_2",
		      0, 0, 0, 0, 0, 0,
		      "host_dependency_dependency_period");
  add_host_dependency("host_dependency_dependent_host_name_3",
		      "host_dependency_host_name_4",
		      0, 0, 0, 0, 0, 0,
		      "host_dependency_dependency_period");
  if (remove_hostdependency_by_id("host_dependency_host_name_2",
				  "host_dependency_dependent_host_name_2") != 1
      || remove_hostdependency_by_id("host_dependency_host_name_1",
				     "host_dependency_dependent_host_name_1") != 1
      || remove_hostdependency_by_id("host_dependency_host_name_3",
				     "host_dependency_dependent_host_name_3") != 1
      || hostdependency_list != NULL
      || hostdependency_list_tail != NULL)
    throw (engine_error() << "remove all hostdependency failed.");

  free_object_skiplists();
}

/**
 *  Check if remove hostdependency works with invalid call.
 */
static void remove_hostdependency_failed() {
  init_object_skiplists();

  if (remove_hostdependency_by_id("", "") == 1)
    throw (engine_error() << "hostdependency remove but dosen't exist.");
  if (remove_hostdependency_by_id(NULL, NULL) == 1)
    throw (engine_error() << "hostdependency remove but pointer is NULL.");

  free_object_skiplists();
}

/**
 *  Check if remove hostdependency works.
 */
int main(void) {
  try {
    remove_all_hostdependency();
    remove_hostdependency_failed();
  }
  catch (std::exception const& e) {
    qDebug() << "error: " << e.what();
    free_memory(get_global_macros());
    return (1);
  }
  return (0);
}