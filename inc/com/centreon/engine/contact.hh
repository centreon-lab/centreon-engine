/*
** Copyright 2017 Centreon
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

#ifndef CCE_CONTACT_HH
#  define CCE_CONTACT_HH

#  include <list>
#  include <vector>
#  include "com/centreon/engine/namespace.hh"
#  include "com/centreon/engine/objects/timeperiod.hh"
#  include "com/centreon/shared_ptr.hh"
#  include "com/centreon/unordered_hash.hh"

#  define MAX_CONTACT_ADDRESSES 6


/* Forward declaration. */
struct command_struct;
struct customvariablesmember_struct;

CCE_BEGIN()

namespace configuration {
  class contact;
}
class contactgroup;

  /**
   *  @class contact contact.hh "com/centreon/engine/contact.hh
   *  @brief Object representing a contact user
   *
   */
class                           contact {
 public:
  static contact*               add_contact(
                                  char const* name,
                                  char const* alias = NULL,
                                  char const* email = NULL,
                                  char const* pager = NULL,
                                  char const* const* addresses = NULL,
                                  char const* svc_notification_period = NULL,
                                  char const* host_notification_period = NULL,
                                  int notify_service_ok = 0,
                                  int notify_service_critical = 0,
                                  int notify_service_warning = 0,
                                  int notify_service_unknown = 0,
                                  int notify_service_flapping = 0,
                                  int notify_service_downtime = 0,
                                  int notify_host_up = 0,
                                  int notify_host_down = 0,
                                  int notify_host_unreachable = 0,
                                  int notify_host_flapping = 0,
                                  int notify_host_downtime = 0,
                                  int host_notifications_enabled = 0,
                                  int service_notifications_enabled = 0,
                                  int can_submit_commands = 0,
                                  int retain_status_information = 0,
                                  int retain_nonstatus_information = 0,
                                  std::string const& timezone = "");

                                contact(
                                  char const* name,
                                  char const* alias,
                                  char const* email,
                                  char const* pager,
                                  char const* const* addresses,
                                  char const* svc_notification_period,
                                  char const* host_notification_period,
                                  int notify_service_ok,
                                  int notify_service_critical,
                                  int notify_service_warning,
                                  int notify_service_unknown,
                                  int notify_service_flapping,
                                  int notify_service_downtime,
                                  int notify_host_up,
                                  int notify_host_down,
                                  int notify_host_unreachable,
                                  int notify_host_flapping,
                                  int notify_host_downtime,
                                  int host_notifications_enabled,
                                  int service_notifications_enabled,
                                  int can_submit_commands,
                                  int retain_status_information,
                                  int retain_nonstatus_information,
                                  std::string const& timezone);
                                contact();
                                contact(contact const& other);
  virtual                       ~contact();
  contact&                      operator=(contact const& other);
  bool                          operator<(contact const& other);

  std::string const&            get_name() const;
  std::string const&            get_alias() const;
  std::string const&            get_email() const;
  std::string const&            get_pager() const;
  bool                          check(int* w, int* e);

  // hosts methods
  umap<std::string, shared_ptr<command_struct> > const&
                                get_host_notification_commands() const;

  umap<std::string, shared_ptr<command_struct> >&
                                get_host_notification_commands();

  timeperiod_struct*            get_host_notification_period() const;
  void                          set_host_notification_period(
                                  timeperiod* tp);

  std::string const&            get_host_notification_period_name() const;
  void                          set_host_notification_period_name(
                                  std::string const& name);
  void                          enable_host_notifications();
  void                          disable_host_notifications();
  bool                          is_host_notifications_enabled() const;
  void                          set_host_notifications_enabled(bool enabled);

  time_t                        get_last_host_notification() const;
  void                          set_last_host_notification(time_t t);
  unsigned long                 get_modified_host_attributes() const;
  void                          set_modified_host_attributes(
                                  unsigned long attr);

  bool                          notify_on_host_critical() const;
  bool                          notify_on_host_down() const;
  bool                          notify_on_host_recovery() const;
  bool                          notify_on_host_unreachable() const;
  bool                          notify_on_host_warning() const;
  void                          add_host_notification_command(
                                  char const* command_name);
  void                          clear_host_notification_commands();

  // services methods
  umap<std::string, shared_ptr<command_struct> > const&
                                get_service_notification_commands() const;

  umap<std::string, shared_ptr<command_struct> >&
                                get_service_notification_commands();

  std::string const&            get_service_notification_period_name() const;
  void                          set_service_notification_period_name(
                                  std::string const& name);

  void                          set_service_notification_period(
                                  timeperiod* tp);

  bool                          notify_on_service_recovery() const;
  bool                          notify_on_service_critical() const;
  bool                          notify_on_service_warning() const;
  time_t                        get_last_service_notification() const;
  void                          set_last_service_notification(time_t t);
  unsigned long                 get_modified_service_attributes() const;
  void                          set_modified_service_attributes(
                                  unsigned long attr);
  timeperiod_struct*            get_service_notification_period() const;
  bool                          is_service_notifications_enabled() const;
  void                          set_service_notifications_enabled(bool enabled);
  void                          enable_service_notifications();
  void                          disable_service_notifications();
  void                          add_service_notification_command(
                                  char const* command_name);
  void                          clear_service_notification_commands();

  bool                          contains_illegal_object_chars() const;

  std::string const&            get_address(int index) const;
  std::list<shared_ptr<contactgroup> > const&
                                get_contactgroups() const;

  std::list<shared_ptr<contactgroup> >&
                                get_contactgroups();

  void                          add_custom_variable(
                                  char const* varname,
                                  char const* varvalue);

  void                          clear_custom_variables();
  customvariablesmember_struct* get_custom_variables();
  customvariablesmember_struct const*
                                get_custom_variables() const;

  unsigned long                 get_modified_attributes() const;
  void                          set_modified_attributes(unsigned long attr);
  std::string const&            get_timezone() const;
  void                          update_status(int aggregated_dump);
  void                          update_config(
                                  configuration::contact const& obj);

  bool                          get_retain_status_information() const;
  bool                          get_retain_nonstatus_information() const;
  bool                          update_custom_variable(
                                  std::string const& varname,
                                  std::string const& varvalue);

 private:

  std::string                   _name;
  std::string                   _alias;
  std::string                   _email;
  std::string                   _pager;
  bool                          _can_submit_commands;

  std::vector<std::string>      _address;
  std::string                   _timezone;
  umap<std::string, shared_ptr<command_struct> >
                                _host_notification_commands;

  umap<std::string, shared_ptr<command_struct> >
                                _service_notification_commands;

  std::string                   _host_notification_period_name;
  timeperiod*                   _host_notification_period;
  std::string                   _service_notification_period_name;
  timeperiod*                   _service_notification_period;
  unsigned long                 _modified_attributes;
  unsigned long                 _modified_host_attributes;
  unsigned long                 _modified_service_attributes;
  time_t                        _last_host_notification;
  time_t                        _last_service_notification;
  int                           _service_notified_states;
  int                           _host_notified_states;
  std::list<shared_ptr<contactgroup> >
                                _contact_groups;

  customvariablesmember_struct* _custom_variables;
  bool                          _notify_on_host_down;
  bool                          _notify_on_host_downtime;
  bool                          _notify_on_host_flapping;
  bool                          _notify_on_host_recovery;
  bool                          _notify_on_host_unreachable;
  bool                          _host_notifications_enabled;
  bool                          _notify_on_service_critical;
  bool                          _notify_on_service_downtime;
  bool                          _notify_on_service_flapping;
  bool                          _notify_on_service_recovery;
  bool                          _notify_on_service_unknown;
  bool                          _notify_on_service_warning;
  bool                          _service_notifications_enabled;
  bool                          _retain_nonstatus_information;
  bool                          _retain_status_information;
};

CCE_END()

#endif // !CCE_CONTACT_HH
