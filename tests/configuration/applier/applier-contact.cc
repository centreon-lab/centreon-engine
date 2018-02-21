/*
** Copyright 2017-2018 Centreon
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

#include <memory>
#include <gtest/gtest.h>
#include "../../timeperiod/utils.hh"
#include "com/centreon/engine/configuration/applier/command.hh"
#include "com/centreon/engine/configuration/applier/contact.hh"
#include "com/centreon/engine/configuration/applier/contactgroup.hh"
#include "com/centreon/engine/configuration/applier/state.hh"
#include "com/centreon/engine/configuration/applier/timeperiod.hh"
#include "com/centreon/engine/configuration/contact.hh"
#include "com/centreon/engine/configuration/state.hh"
#include "com/centreon/engine/contact.hh"
#include "com/centreon/engine/contactgroup.hh"
#include "com/centreon/engine/notifications/notifier.hh"
#include "com/centreon/shared_ptr.hh"

using namespace com::centreon;
using namespace com::centreon::engine;
using namespace com::centreon::engine::configuration;
using namespace com::centreon::engine::configuration::applier;

extern configuration::state* config;
extern int config_errors;
extern int config_warnings;

class ApplierContact : public ::testing::Test {
 public:
  void SetUp() {
    config_errors = 0;
    config_warnings = 0;
    if (config == NULL)
      config = new configuration::state;
    configuration::applier::state::load();  // Needed to create a contact
  }

  void TearDown() {
    configuration::applier::state::unload();
    delete config;
    config = NULL;
  }

  configuration::contact valid_contact_config() const {
    // Add command.
    {
      configuration::command cmd;
      cmd.parse("command_name", "cmd");
      cmd.parse("command_line", "true");
      configuration::applier::command aplyr;
      aplyr.add_object(cmd);
    }
    // Add timeperiod.
    {
      configuration::timeperiod tperiod;
      tperiod.parse("timeperiod_name", "24x7");
      tperiod.parse("alias", "24x7");
      tperiod.parse("monday", "00:00-24:00");
      configuration::applier::timeperiod aplyr;
      aplyr.add_object(tperiod);
    }
    // Valid contact configuration
    // (will generate 0 warnings or 0 errors).
    configuration::contact ctct;
    ctct.parse("contact_name", "admin");
    ctct.parse("host_notification_period", "24x7");
    ctct.parse("service_notification_period", "24x7");
    ctct.parse("host_notification_commands", "cmd");
    ctct.parse("service_notification_commands", "cmd");
    return (ctct);
  }
};

// Given a contact applier
// And a configuration contact
// When we modify the contact configuration with an unexisting contact
// configuration
// Then an exception is thrown.
TEST_F(ApplierContact, ModifyUnexistingContactConfigFromConfig) {
  configuration::applier::contact aply;
  configuration::contact ctct("test");
  ASSERT_TRUE(ctct.parse("contactgroups", "test_group"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "cmd1,cmd2"));
  ASSERT_THROW(aply.modify_object(ctct), std::exception);
}

// Given a contact applier
// And a configuration contact
// When we modify the contact configuration with an unexisting contact
// Then an exception is thrown.
TEST_F(ApplierContact, ModifyUnexistingContactFromConfig) {
  configuration::applier::contact aply;
  configuration::contact ctct("test");
  ASSERT_TRUE(ctct.parse("contactgroups", "test_group"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "cmd1,cmd2"));
  config->contacts().insert(ctct);
  ASSERT_THROW(aply.modify_object(ctct), std::exception);
}

// Given contactgroup / contact appliers
// And a configuration contactgroup and a configuration contact
// that are already in configuration
// When we change the contact configuration
// Then the appliers modify_object updates the contact.
TEST_F(ApplierContact, ModifyContactFromConfig) {
  configuration::applier::contact aply;
  configuration::applier::contactgroup aply_grp;
  configuration::contactgroup grp("test_group");
  configuration::contact ctct("test");
  ASSERT_TRUE(ctct.parse("contactgroups", "test_group"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "cmd1,cmd2"));
  aply_grp.add_object(grp);
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_TRUE(ctct.parse("alias", "test_alias"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "host_cmd1,host_cmd2"));
  ASSERT_TRUE(ctct.parse("service_notification_commands", "svc_cmd1,svc_cmd2"));
  ASSERT_TRUE(ctct.parse("_cust", "MyValue"));
  ASSERT_TRUE(ctct.parse("email", "new_mail"));
  ASSERT_TRUE(ctct.parse("pager", "new_pager"));
  aply.modify_object(ctct);
  contact_map::const_iterator
    it(configuration::applier::state::instance().contacts().find("test"));
  ASSERT_NE(it, configuration::applier::state::instance().contacts().end());
  ASSERT_TRUE(it->second->get_alias() == "test_alias");
  ASSERT_TRUE(it->second->get_email() == "new_mail");
  ASSERT_TRUE(it->second->get_pager() == "new_pager");
}

// Given contactgroup / contact appliers
// And a configuration contactgroup and a configuration contact
// that are already in configuration
// When we change the contact configuration
// Then the appliers modify_object updates the contact.
TEST_F(ApplierContact, RemoveContactFromConfig) {
  configuration::applier::contact aply;
  configuration::applier::contactgroup aply_grp;
  configuration::contactgroup grp("test_group");
  configuration::contact ctct("test");
  ASSERT_TRUE(ctct.parse("contactgroups", "test_group"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "cmd1,cmd2"));
  ASSERT_TRUE(ctct.parse("service_notification_commands", "svc1,svc2"));
  ASSERT_TRUE(ctct.parse("_superVar", "superValue"));
  aply_grp.add_object(grp);
  aply.add_object(ctct);
  aply.expand_objects(*config);
  aply.remove_object(ctct);
  ASSERT_TRUE(configuration::applier::state::instance().contacts().empty());
}

// Given contactgroup / contact appliers
// And a configuration contactgroup and a configuration contact
// that are already in configuration
// When we resolve the contact configuration
// Then the contact contactgroups is cleared, nothing more if the
// contact check is OK. Here, since notification commands are empty,
// an exception is thrown.
TEST_F(ApplierContact, ResolveContactFromConfig) {
  configuration::applier::contact aply;
  configuration::applier::contactgroup aply_grp;
  configuration::contactgroup grp("test_group");
  configuration::contact ctct("test");
  ASSERT_TRUE(ctct.parse("contactgroups", "test_group"));
  ASSERT_TRUE(ctct.parse("host_notification_commands", "cmd1,cmd2"));
  aply_grp.add_object(grp);
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
}

// Given a contact
// And an applier
// When the contact is resolved by the applier
// Then an exception is thrown
// And 2 warnings and 2 errors are returned:
//  * error 1 => no service notification command
//  * error 2 => no host notification command
//  * warning 1 => no service notification period
//  * warning 2 => no host notification period
TEST_F(ApplierContact, ResolveContactNoNotification) {
  configuration::applier::contact aply;
  configuration::contact ctct("test");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
  ASSERT_EQ(config_warnings, 2);
  ASSERT_EQ(config_errors, 2);
}

// Given a valid contact
//   - valid host notification period
//   - valid service notification period
//   - valid host notification command
//   - valid service notification command
// And an applier
// When resolve_object() is called
// Then no exception is thrown
// And no errors are returned
// And links are properly resolved
TEST_F(ApplierContact, ResolveValidContact) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_NO_THROW(aply.resolve_object(ctct));
  ASSERT_EQ(config_warnings, 0);
  ASSERT_EQ(config_errors, 0);
}

// Given a valid contact
// And an applier
// When adding a non-existing service notification period to the contact
// Then the resolve method throws
// And returns 1 error
TEST_F(ApplierContact, ResolveNonExistingServiceNotificationTimeperiod) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("service_notification_period", "non_existing_period");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
  ASSERT_EQ(config_warnings, 0);
  ASSERT_EQ(config_errors, 1);
}

// Given a valid contact
// And an applier
// When adding a non-existing host notification period to the contact
// Then the resolve method throws
// And returns 1 error
TEST_F(ApplierContact, ResolveNonExistingHostNotificationTimeperiod) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("host_notification_period", "non_existing_period");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
  ASSERT_EQ(config_warnings, 0);
  ASSERT_EQ(config_errors, 1);
}

// Given a valid contact
// And an applier
// When adding a non-existing service command to the contact
// Then the resolve method throws
// And returns 1 error
TEST_F(ApplierContact, ResolveNonExistingServiceCommand) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("service_notification_commands", "non_existing_command");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
  ASSERT_EQ(config_warnings, 0);
  ASSERT_EQ(config_errors, 1);
}

// Given a valid contact
// And an applier
// When adding a non-existing host command to the contact
// Then the resolve method throws
// And returns 1 error
TEST_F(ApplierContact, ResolveNonExistingHostCommand) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("host_notification_commands", "non_existing_command");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  ASSERT_THROW(aply.resolve_object(ctct), std::exception);
  ASSERT_EQ(config_warnings, 0);
  ASSERT_EQ(config_errors, 1);
}

// Given a valid contact configuration
// And the contact has multiple host notification commands
// When the applier resolve_object() method is called
// Then the contact has the multiple host notification commands
TEST_F(ApplierContact, ResolveContactWithMultipleHostNotificationCommand) {
  // Given
  configuration::contact ctct(valid_contact_config());

  // And
  {
    configuration::applier::command aplyr;
    for (int i(0); i < 3; ++i) {
      std::ostringstream cmdname;
      cmdname << "command" << i + 1;
      configuration::command cmd;
      cmd.parse("command_name", cmdname.str().c_str());
      cmd.parse("command_line", "true");
      aplyr.add_object(cmd);
    }
    aplyr.expand_objects(*config);
  }
  ctct.parse("host_notification_commands", "command1!ARG1,command2,command3!ARG3");
  configuration::applier::contact aplyr;
  aplyr.add_object(ctct);
  aplyr.expand_objects(*config);

  // When
  aplyr.resolve_object(ctct);

  // Then
  std::list<std::pair<commands::command*, std::string> > const&
    commands(configuration::applier::state::instance().contacts_find(
               ctct.contact_name())->get_host_notification_commands());
  ASSERT_EQ(commands.size(), 3);
  std::list<std::pair<commands::command*, std::string> >::const_iterator
    it(commands.begin()),
    end(commands.end());
  ASSERT_EQ(it->first->get_name(), "command1");
  ASSERT_EQ(it->second, "command1!ARG1");
  ++it;
  ASSERT_EQ(it->first->get_name(), "command2");
  ASSERT_EQ(it->second, "command2");
  ++it;
  ASSERT_EQ(it->first->get_name(), "command3");
  ASSERT_EQ(it->second, "command3!ARG3");
}

// Given a valid contact
// And the contact is notified on host recovery
// But not on down or unreachable host
// When resolve_object() is called
// Then a warning is returned
TEST_F(ApplierContact, ContactWithOnlyHostRecoveryNotification) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("host_notification_options", "r");
  ctct.parse("service_notification_options", "n");
  ctct.parse("host_notifications_enabled", "1");
  ctct.parse("service_notifications_enabled", "1");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  aply.resolve_object(ctct);
  ASSERT_EQ(config_warnings, 1);
  ASSERT_EQ(config_errors, 0);
}

// Given a valid contact
// And the contact is notified on service recovery
// But not on critical, warning or unknown service
// When resolve_object() is called
// Then a warning is returned
TEST_F(ApplierContact, ContactWithOnlyServiceRecoveryNotification) {
  configuration::applier::contact aply;
  configuration::contact ctct(valid_contact_config());
  ctct.parse("host_notification_options", "n");
  ctct.parse("service_notification_options", "r");
  ctct.parse("host_notifications_enabled", "1");
  ctct.parse("service_notifications_enabled", "1");
  aply.add_object(ctct);
  aply.expand_objects(*config);
  aply.resolve_object(ctct);
  ASSERT_EQ(config_warnings, 1);
  ASSERT_EQ(config_errors, 0);
}