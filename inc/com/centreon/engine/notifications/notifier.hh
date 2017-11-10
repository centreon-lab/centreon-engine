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

#ifndef CCE_NOTIFICATIONS_NOTIFIER_HH
#  define CCE_NOTIFICATIONS_NOTIFIER_HH

#  include <ctime>
#  include <list>
#  include "com/centreon/engine/checks/checkable.hh"
#  include "com/centreon/engine/configuration/contact.hh"
#  include "com/centreon/engine/configuration/contactgroup.hh"
#  include "com/centreon/shared_ptr.hh"

CCE_BEGIN()

namespace           notifications {

  /**
   *  @class notifier notifier.hh "com/centreon/engine/notifications/notifier.hh"
   *  @brief Object validating notifications and sending them if needed.
   *
   */
  class               notifier : public checks::checkable {
   public:
    typedef bool (notifier::* notifier_filter)();

    enum              notification_type {
                      NONE,
                      PROBLEM,
                      RECOVERY,
                      ACKNOWLEDGEMENT,
                      FLAPPINGSTART,
                      FLAPPINGSTOP,
                      FLAPPINGDISABLED,
                      DOWNTIMESTART,
                      DOWNTIMESTOP,
                      DOWNTIMECANCELLED
    };
                      notifier();
                      notifier(notifier const& other);
    virtual           ~notifier();
    notifier&         operator=(notifier const& other);
    void              add_contact(shared_ptr<configuration::contact> user);
    bool              are_notifications_enabled() const;
    void              clear_contacts();
    void              enable_state_notification(int state);
    std::list<shared_ptr<configuration::contact> >
                      get_contacts_list();

    int               get_current_notification_number() const;
    notification_type get_current_notification_type() const;
    time_t            get_last_notification() const;
    time_t            get_next_notification() const;
    bool              is_in_downtime() const;
    bool              is_state_notification_enabled(int state) const;
    void              notify(
                        notification_type type,
                        std::string const& author,
                        std::string const& comment);

    void              set_last_notification(time_t last_notification);
    void              set_next_notification(time_t next_notification);
    bool              should_be_escalated() const;

   protected:
    time_t            _last_notification;
    time_t            _next_notification;
    long              _notification_interval;
    int               _current_notification_number;
    int               _current_notification_id;
    notification_type _type;

   private:
    notifier_filter   _get_filter(notification_type type) const;
    bool              _problem_filter();
    bool              _recovery_filter();
    long              _get_notification_interval() const;

    static notifier_filter
                      _filter[];
    static std::string
                      _notification_string[];
    bool              _escalate_notification;

    int               _notified_states;
    std::list<shared_ptr<configuration::contact> >
                      _contacts;
  };
}

CCE_END()

#endif // !CCE_NOTIFICATIONS_NOTIFIER_HH
