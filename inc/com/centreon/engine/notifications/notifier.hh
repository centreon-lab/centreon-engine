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

#ifndef CCE_NOTIFICATIONS_NOTIFIER_HH
#  define CCE_NOTIFICATIONS_NOTIFIER_HH

#  include <ctime>
#  include <list>
#  include "com/centreon/engine/checks/checkable.hh"
#  include "com/centreon/engine/downtime.hh"
#  include "com/centreon/engine/globals.hh"
#  include "com/centreon/engine/macros/defines.hh"
#  include "com/centreon/engine/namespace.hh"

CCE_BEGIN()

// Forward declaration
class contact;

namespace           notifications {

  /**
   *  @class notifier notifier.hh "com/centreon/engine/notifications/notifier.hh"
   *  @brief Object validating notifications and sending them if needed.
   *
   */
  class               notifier : public checks::checkable {
   public:
    typedef bool (notifier::* notifier_filter)();
    typedef void (notifier::* macro_builder)(struct nagios_macros&);

    enum              notifier_type {
      HOST_NOTIFICATION,
      SERVICE_NOTIFICATION
    };

    enum              notification_type {
      PROBLEM              = 0,
      RECOVERY             = 1,
      ACKNOWLEDGEMENT      = 2,
      FLAPPINGSTART        = 3,
      FLAPPINGSTOP         = 4,
      FLAPPINGDISABLED     = 5,
      DOWNTIMESTART        = 6,
      DOWNTIMESTOP         = 7,
      DOWNTIMECANCELLED    = 8,
      CUSTOM               = 9,
    };
    enum              action_on {
      ON_NONE = 0,
      ON_RECOVERY = (1 << 0),
      ON_UP = (1 << 1),
      ON_DOWN = (1 << 2),
      ON_UNREACHABLE = (1 << 3),
      ON_FLAPPING = (1 << 4),
      ON_DOWNTIME = (1 << 5),
      ON_OK = (1 << 6),
      ON_WARNING = (1 << 7),
      ON_UNKNOWN = (1 << 8),
      ON_CRITICAL = (1 << 9),
    };

    enum              acknowledgement_type {
      ACKNOWLEDGEMENT_NONE,
      ACKNOWLEDGEMENT_NORMAL,
      ACKNOWLEDGEMENT_STICKY
    };

    enum              downtime_propagation {
      DOWNTIME_PROPAGATE_NONE,
      DOWNTIME_PROPAGATE_SIMPLE,
      DOWNTIME_PROPAGATE_TRIGGERED
    };

                      notifier();
                      notifier(notifier const& other);
    virtual           ~notifier();
    notifier&         operator=(notifier const& other);
    bool              contains_contact(engine::contact* user) const;
    bool              contains_contact(std::string const& username) const;
    void              add_contact(engine::contact* user);
    void              add_contactgroup(engine::contactgroup* cg);
    void              delete_acknowledgement_comments();
    void              set_notifications_enabled(bool enabled);
    bool              get_notifications_enabled() const;
    acknowledgement_type
                      get_acknowledgement_type() const;
    void              enable_state_notification(int state);
    void              clear_contacts();
    std::list<engine::contact*>
                      get_contacts_list();
    umap<std::string, engine::contact*> const&
                      get_contacts() const;
    umap<std::string, engine::contact*>&
                      get_contacts();
    void              clear_contactgroups();
    umap<std::string, engine::contactgroup*> const&
                      get_contactgroups() const;
    umap<std::string, engine::contactgroup*>&
                      get_contactgroups();

    int               get_current_notification_id() const;
    void              set_current_notification_id(int id);
    int               get_current_notification_number() const;
    unsigned int      get_current_notifications_flag() const;
    time_t            get_last_notification() const;
    time_t            get_next_notification() const;
    bool              get_no_more_notifications() const;
    int               get_pending_flex_downtime() const;
    void              inc_pending_flex_downtime();
    void              dec_pending_flex_downtime();
    void              check_pending_flex_downtime();
    void              inc_scheduled_downtime_depth();
    void              dec_scheduled_downtime_depth();
    int               get_scheduled_downtime_depth() const;
    bool              is_in_downtime() const;
    bool              is_state_notification_enabled(int state) const;
    void              notify(
                        notification_type type,
                        std::string const& author = "",
                        std::string const& comment = "",
                        int options = 0);

    void              set_last_notification(time_t last_notification);
    void              set_next_notification(time_t next_notification);
    bool              should_be_escalated() const;
    bool              is_acknowledged() const;
    void              set_acknowledged(acknowledgement_type type);
    time_t            get_last_acknowledgement() const;
    void              set_last_acknowledgement(time_t last_acknowledgement);
    bool              get_recovery_been_sent() const;
    bool              get_notify_on_down() const;
    void              set_notify_on_down(bool notify);
    bool              get_notify_on_downtime() const;
    void              set_notify_on_downtime(bool notify);
    bool              get_notify_on_flapping() const;
    void              set_notify_on_flapping(bool notify);
    bool              get_notify_on_recovery() const;
    void              set_notify_on_recovery(bool notify);
    bool              get_notify_on_unreachable() const;
    void              set_notify_on_unreachable(bool notify);
    void              set_current_notification_number(int number);
    time_t            get_initial_notif_time() const;
    void              set_initial_notif_time(time_t initial);
//    void              set_no_more_notifications(bool no_more);
    void              set_recovery_been_sent(bool sent);
    timeperiod*       get_notification_period() const;
    void              set_notification_period(timeperiod* tperiod);
    long              get_notification_interval() const;
    void              set_notification_interval(long interval);
    int               get_first_notification_delay() const;
    void              set_first_notification_delay(int delay);
    int               get_recovery_notification_delay() const;
    void              set_recovery_notification_delay(int delay);
    int               schedule_downtime(
                        downtime::downtime_type type,
                        time_t entry_time,
                        std::string const& author,
                        std::string const& comment_data,
                        time_t start_time,
                        time_t end_time,
                        bool fixed,
                        unsigned long triggered_by,
                        unsigned long duration,
                        downtime_propagation propagate =
                          DOWNTIME_PROPAGATE_NONE);
    std::string       get_info();

   protected:
    virtual void      _checkable_macro_builder(nagios_macros& mac) = 0;

    time_t            _last_notification;
    time_t            _next_notification;
    long              _notification_interval;
    int               _current_notification_id;
    int               _current_notification_number;
    unsigned int      _current_notifications;
    bool              _in_downtime;

   private:
    notifier_filter   _get_filter(notification_type type) const;
    macro_builder     _get_macro_builder(notification_type type) const;
    bool              _problem_filter();
    bool              _recovery_filter();
    bool              _acknowledgement_filter();
    bool              _flappingstart_filter();
    bool              _flappingstopdisabled_filter();
    bool              _downtimestart_filter();
    bool              _downtimestopcancelled_filter();
    bool              _custom_filter();

    void              _problem_macro_builder(nagios_macros& mac);
    void              _recovery_macro_builder(nagios_macros& mac);

    acknowledgement_type
                      _current_acknowledgement;
    time_t            _last_acknowledgement;

    static notifier_filter
                      _filter[];
    static macro_builder
                      _macro_builder[];
    static std::string
                      _notification_string[];
    int               _pending_flex_downtime;
    bool              _escalate_notification;

    int               _notified_states;
    umap<std::string, engine::contact*>
                      _contacts;
    umap<std::string, engine::contactgroup*>
                      _contact_groups;
    bool              _notifications_enabled;
    int               _scheduled_downtime_depth;
    int               _first_notification_delay;
    int               _recovery_notification_delay;
  };
}

CCE_END()

#endif // !CCE_NOTIFICATIONS_NOTIFIER_HH
