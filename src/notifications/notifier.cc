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
#include <sstream>
#include "com/centreon/engine/downtime.hh"
#include "com/centreon/engine/globals.hh"
#include "com/centreon/engine/host.hh"
#include "com/centreon/engine/logging/logger.hh"
#include "com/centreon/engine/notifications/notifier.hh"
#include "com/centreon/engine/service.hh"

using namespace com::centreon;
using namespace com::centreon::engine;
using namespace com::centreon::engine::logging;
using namespace com::centreon::engine::notifications;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 */
notifier::notifier()
  : _acknowledgement_timeout(0),
    _current_notifications(0),
    _in_downtime(false),
    _current_acknowledgement(ACKNOWLEDGEMENT_NONE),
    _current_notification_id(0),
    _current_notification_number(0),
    _escalate_notification(false),
    _first_notification_delay(0),
    _last_acknowledgement(0),
    _last_notification(0),
    _next_notification(0),
    _notification_interval(5 * 60),
    _notification_period(NULL),
    _notifications_enabled(true),
    _notified_states(0),
    _pending_flex_downtime(0),
    _recovery_notification_delay(0),
    _scheduled_downtime_depth(0) {}

/**
 *  Copy constructor.
 *
 *  @param[in] other  Object to copy.
 */
notifier::notifier(notifier const& other) : checks::checkable(other) {
  _internal_copy(other);
}

/**
 * Destructor.
 */
notifier::~notifier() {
  logger(dbg_functions, basic) << "notifier: destructor";
}

/**
 *  Assignment operator.
 *
 *  @param[in] other  Object to copy.
 *
 *  @return This object
 */
notifier& notifier::operator=(notifier const& other) {
  if (this != &other) {
    checks::checkable::operator=(other);
    _internal_copy(other);
  }
  return (*this);
}

/**************************************
*                                     *
*            Configuration            *
*                                     *
**************************************/

/**
 *  Get first notification delay.
 *
 *  @return First notification delay in seconds.
 */
int notifier::get_first_notification_delay() const {
  return (_first_notification_delay);
}

/**
 *  Set first notification delay.
 *
 *  @param[in] delay  Delay in seconds.
 */
void notifier::set_first_notification_delay(int delay) {
  _first_notification_delay = delay;
  return ;
}

/**
 *  Get notification period.
 *
 *  @return Pointer to the notification period object.
 */
timeperiod* notifier::get_notification_period() const {
  return (_notification_period);
}

/**
 *  Set notification period of this notifier.
 *
 *  @param[in] tperiod  Pointer to timeperiod.
 */
void notifier::set_notification_period(timeperiod* tperiod) {
  _notification_period = tperiod;
  return ;
}

/**
 *  Get notification interval.
 *
 *  @return Notification interval in seconds.
 */
long notifier::get_notification_interval() const {
  return (_notification_interval);
}

/**
 *  Set notification interval.
 *
 *  @param[in] interval  New notification interval in seconds.
 */
void notifier::set_notification_interval(long interval) {
  _notification_interval = interval;
  return ;
}

/**
 *  Check if notifications are enabled for this notifier.
 *
 *  @return True if this notifier has notifications enabled.
 */
bool notifier::get_notifications_enabled() const {
  return (_notifications_enabled);
}

/**
 *  Enable or disable notifications for this notifier.
 *
 *  @param[in] enable  True to enable.
 */
void notifier::set_notifications_enabled(bool enable) {
  _notifications_enabled = enable;
  return ;
}

/**
 *  Check if notification is enabled on some state.
 *
 *  @param[in] state  Target state.
 *
 *  @return True if notification is enabled for the specified state.
 */
bool notifier::get_notify_on(action_on state) const {
  return (_notified_states & state);
}

/**
 *  Enable or disable notification on a state.
 *
 *  @param[in] state   State.
 *  @param[in] enable  True to enable.
 */
void notifier::set_notify_on(action_on state, bool enable) {
  if (enable)
    _notified_states |= state;
  else
    _notified_states &= ~state;
  return ;
}

/**
 *  This method tells if the notifier should notify for a specific
 *  state.
 *
 *  @param[in] state  Target state.
 *
 *  @return True if notifications are enabled for state.
 */
bool notifier::is_state_notification_enabled(int state) const {
  return (_notified_states & (1 << state));
}

/**
 *  Get recovery notification delay.
 *
 *  @return Recovery notification delay in seconds.
 */
int notifier::get_recovery_notification_delay() const {
  return (_recovery_notification_delay);
}

/**
 *  Set recovery notification delay.
 *
 *  @param[in] delay  New recovery notification delay in seconds.
 */
void notifier::set_recovery_notification_delay(int delay) {
  _recovery_notification_delay = delay;
  return ;
}

/**************************************
*                                     *
*               Runtime               *
*                                     *
**************************************/

/**
 *  Get current notification ID.
 *
 *  @return An integer identifying uniquely the current notification.
 */
int notifier::get_current_notification_id() const {
  return (_current_notification_id);
}

/**
 *  Set current notification ID.
 *
 *  @param[in] id  New notification ID.
 */
void notifier::set_current_notification_id(int id) {
  _current_notification_id = id;
  return ;
}

/**
 *  Get the current notification number.
 *
 *  @return The number of the current notification.
 */
int notifier::get_current_notification_number() const {
  return _current_notification_number;
}

/**
 *  Set the current notification number.
 *
 *  @param[in] number  New notification number.
 */
void notifier::set_current_notification_number(int number) {
  _current_notification_number = number;
  return ;
}

/**
 *  Get the last time a notification was sent.
 *
 *  @return Timestamp.
 */
time_t notifier::get_last_notification() const {
  return _last_notification;
}

/**
 *  Set the last time a notification was sent.
 *
 *  @param[in] last_notification  Timestamp.
 */
void notifier::set_last_notification(time_t last_notification) {
  _last_notification = last_notification;
  return ;
}

/**
 *  Get the time at which the next notification is expected to be sent.
 *
 *  @return Timestamp.
 */
time_t notifier::get_next_notification() const {
  return (_next_notification);
}

/**
 *  Set when the next notification is expected to be sent.
 *
 *  @param[in] next_notification  Timestamp.
 */
void notifier::set_next_notification(time_t next_notification) {
  _next_notification = next_notification;
  return ;
}

/**
 *  Attempt to send a notification.
 *
 *  @param[in] type     Type of notification.
 *  @param[in] author   Author name.
 *  @param[in] comment  Comment.
 *  @param[in] options  Options.
 */
void notifier::notify(
                 notification_type type,
                 std::string const& author,
                 std::string const& comment,
                 int options) {
  // Normal acknowledgement is lost when state changes. We must verify
  // that the acknowledgement is older than the state change.
  // Sticky acknowledgement is lost when state changes to OK. We must
  // verify that the acknowledgement is older than the state change.
  if (get_last_acknowledgement() < get_last_check()
      && ((get_acknowledgement_type() == ACKNOWLEDGEMENT_NORMAL
           && get_current_state() != get_last_state())
          || (get_acknowledgement_type() == ACKNOWLEDGEMENT_STICKY
              && get_current_state() == 0))) {
    set_acknowledged(ACKNOWLEDGEMENT_NONE);
    delete_acknowledgement_comments();
  }

  // Create list of users to notify.
  umap<std::string, engine::contact*> users_to_notify(_contacts);
  for (umap<std::string, engine::contactgroup*>::const_iterator
         grp_it(_contact_groups.begin()),
         grp_end(_contact_groups.end());
       grp_it != grp_end;
       ++grp_it) {
    for (umap<std::string, engine::contact*>::const_iterator
           cntct_it(grp_it->second->get_members().begin()),
           cntct_end(grp_it->second->get_members().end());
         cntct_it != cntct_end;
         ++cntct_it)
      users_to_notify[cntct_it->second->get_name()] = cntct_it->second;
  }
  if (users_to_notify.empty())
    return ;

  if (get_notifications_enabled()) {
    notifier_filter should_notify = _get_filter(type);
    if ((this->*should_notify)()) {
      nagios_macros mac;
      std::ostringstream oss;
      bool first_time = true;

      // Notify each contact
      for (umap<std::string, engine::contact*>::iterator
             it(users_to_notify.begin()),
             end(users_to_notify.end());
           it != end;
           ++it) {
        logger(dbg_notifications, most)
          << "** Notifying contact '" << it->second->get_name() << "'";
        if (first_time)
          oss << it->second->get_name();
        else
          oss << ',' << it->second->get_name();
      }

      memset(&mac, 0, sizeof(mac));

      // grab the macro variables
      // FIXME DBR: This code cannot work now. Could we improve macros ?
//      grab_host_macros_r(&mac, get_host());
//      if (!_is_host())
//        grab_service_macros_r(&mac, get_service());

      /* The author is a string taken from an external command. It can be
         the contact name or the contact alias. And if the external command
         is badly configured, maybe no contact is associated to this author */
      configuration::contact const* author_contact;
      if (!author.empty()) {
        configuration::set_contact::const_iterator it = config->contacts_find(author);
        if (it != config->contacts().end())
          author_contact = &(*it);
        else
          author_contact = NULL;
      }

      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHOR], author);
      string::setstr(mac.x[MACRO_NOTIFICATIONCOMMENT], comment);
      // FIXME DBR: temp_contact is not defined, see previous lines...
//      if (temp_contact) {
//        string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME], temp_contact->get_name());
//        string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS], temp_contact->get_alias());
//      }
//      else {
//        string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME]);
//        string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS]);
//      }

      /* Old Nagios comment: these macros are deprecated and will likely
         disappear in next major release. if this is an acknowledgement,
         get author and comment macros: FIXME: Is it useful for us ??? */
      if (_current_notifications == ACKNOWLEDGEMENT) {
        string::setstr(mac.x[MACRO_SERVICEACKAUTHOR], author);
        string::setstr(mac.x[MACRO_SERVICEACKCOMMENT], comment);
        // FIXME DBR: Same as previous comment
//        if (temp_contact) {
//          string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME], temp_contact->get_name());
//          string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS], temp_contact->get_alias());
//        }
//        else {
//          string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME]);
//          string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS]);
//        }
      }

      // Set the notification type macro
      string::setstr(
                mac.x[MACRO_NOTIFICATIONTYPE],
                _notification_string[_current_notifications]);

      // Set the notification number
      string::setstr(
                mac.x[MACRO_SERVICENOTIFICATIONNUMBER],
                _current_notification_number);

      /* set the notification id macro */
      // FIXME DBR: _current_notification_id is not managed now, the only
      // possible value is 0.
      string::setstr(
                mac.x[MACRO_SERVICENOTIFICATIONID],
                _current_notification_id);

      string::setstr(
                mac.x[MACRO_NOTIFICATIONISESCALATED],
                should_be_escalated());
      string::setstr(
                mac.x[MACRO_NOTIFICATIONRECIPIENTS],
                oss.str());

      _current_notifications |= (1 << type);
      switch (type) {
        case PROBLEM:
          _current_notifications &= ~(1 << RECOVERY);
          break;
        case RECOVERY:
          _current_notifications &= ~(1 << PROBLEM);
          break;
        case FLAPPINGSTART:
          _current_notifications &= ~((1 << FLAPPINGSTOP) | (1 << FLAPPINGDISABLED));
          break;
        case FLAPPINGSTOP:
        case FLAPPINGDISABLED:
          _current_notifications &= ~(1 << FLAPPINGSTART);
          break;
      }
      time(&_last_notification);
    }
  }
}

/**************************************
*                                     *
*      Contacts / contact groups      *
*                                     *
**************************************/

/**
 *  Add a contact to this notifier.
 *
 *  @param[in,out] user  Pointer to a contact that will receive
 *                       notifications from this object.
 */
void notifier::add_contact(engine::contact* user) {
  if (user)
    _contacts[user->get_name()] = user;
  return ;
}

/**
 *  Add a contact group to this notifier.
 *
 *  @param[in,out] cg  Pointer to a contact group that will receive
 *                     notifications from this object.
 */
void notifier::add_contactgroup(engine::contactgroup* cg) {
  if (cg)
    _contact_groups[cg->get_name()] = cg;
  return ;
}

/**
 *  Clear contact list.
 */
void notifier::clear_contacts() {
  _contacts.clear();
  return ;
}

/**
 *  Clear contact group list.
 */
void notifier::clear_contactgroups() {
  _contact_groups.clear();
  return ;
}

/**
 *  Check if some contact is notified by this object.
 *
 *  @param[in] username  User name.
 *
 *  @return True if user is notified by this object.
 */
bool notifier::contains_contact(std::string const& username) const {
  // Check in contact list.
  umap<std::string, contact*>::const_iterator it(
    get_contacts().find(username));
  if (it != get_contacts().end())
    return (true);

  // Check each contact group.
  for (umap<std::string, engine::contactgroup*>::const_iterator
         cgit(get_contactgroups().begin()),
         end(get_contactgroups().end());
       cgit != end;
       ++cgit) {
    if (cgit->second->has_member(username))
      return (true);
  }
  return (false);
}

/**
 *  Get contact container.
 *
 *  @return Reference to the contact container.
 */
umap<std::string, engine::contact*> const& notifier::get_contacts() const {
  return _contacts;
}

/**
 *  Get contact group container.
 *
 *  @return Reference to the contact group container.
 */
umap<std::string, engine::contactgroup*> const& notifier::get_contactgroups() const {
  return _contact_groups;
}

/**************************************
*                                     *
*           Acknowledgement           *
*                                     *
**************************************/

/**
 *  Get acknowledgement timeout.
 *
 *  @return Acknowledgement timeout in seconds.
 */
int notifier::get_acknowledgement_timeout() const {
  return (_acknowledgement_timeout);
}

/**
 *  Set acknowledgement timeout.
 *
 *  @param[in] timeout  Acknowledgement timeout in seconds.
 */
void notifier::set_acknowledgement_timeout(int timeout) {
  _acknowledgement_timeout = timeout;
  return ;
}

/**
 *  Get current acknowledgement type.
 *
 *  @return Acknowledgement type.
 */
notifier::acknowledgement_type notifier::get_acknowledgement_type() const {
  return (_current_acknowledgement);
}

/**
 *  Check if notifier is acknowledged.
 *
 *  @return True if acknowledged.
 */
bool notifier::is_acknowledged() const {
  return (_current_acknowledgement != ACKNOWLEDGEMENT_NONE);
}

/**
 *  Acknowledge / disacknowledge.
 *
 *  @param[in] type  Acknowledgement type.
 */
void notifier::set_acknowledged(acknowledgement_type type) {
  _current_acknowledgement = type;
  return ;
}

/**
 *  Get last time notifier was acknowledged.
 *
 *  @return Timestamp.
 */
time_t notifier::get_last_acknowledgement() const {
  return (_last_acknowledgement);
}

/**
 *  Set last time notifier was acknowledged.
 *
 *  @param[in] last_acknowledgement  Timestamp.
 */
void notifier::set_last_acknowledgement(time_t last_acknowledgement) {
  _last_acknowledgement = last_acknowledgement;
  return ;
}

/**************************************
*                                     *
*              Downtime               *
*                                     *
**************************************/

/**
 *  This method tells if this object is in downtime.
 *
 *  @return True if this object is in downtime.
 */
bool notifier::is_in_downtime() const {
  return _in_downtime;
}

/**
 *  Get the number of pending flexible downtimes targeting this object.
 *
 *  @return An integer.
 */
int notifier::get_pending_flex_downtime() const {
  return (_pending_flex_downtime);
}

/**
 *  Increment the number of pending flexible downtimes targeting this
 *  object.
 */
void notifier::inc_pending_flex_downtime() {
  ++_pending_flex_downtime;
  return ;
}

/**
 *  Decrement the number of pending flexible downtimes targeting this
 *  object.
 */
void notifier::dec_pending_flex_downtime() {
  --_pending_flex_downtime;
  return ;
}

/**
 *  Get the number of active downtimes targeting this object.
 *
 *  @return An integer.
 */
int notifier::get_scheduled_downtime_depth() const {
  return (_scheduled_downtime_depth);
}

/**
 *  Increment the number of active downtimes targeting this object.
 */
void notifier::inc_scheduled_downtime_depth() {
  ++_scheduled_downtime_depth;
  return ;
}

/**
 *  Decrement the number of active downtimes targeting this object.
 */
void notifier::dec_scheduled_downtime_depth() {
  --_scheduled_downtime_depth;
  return ;
}

/**
 *  Schedule a downtime on this object.
 *
 *  @param[in] type          Type of downtime.
 *  @param[in] entry_time    Downtime entry time.
 *  @param[in] author        Author.
 *  @param[in] comment_data  Comment.
 *  @param[in] start_time    Downtime start time.
 *  @param[in] end_time      Downtime end time.
 *  @param[in] fixed         True for a fixed downtime, false for
 *                           flexible.
 *  @param[in] triggered_by  ID of the downtime that triggered this one.
 *                           0 if downtime was not triggered.
 *  @param[in] duration      Downtime duration. Only for flexible
 *                           downtimes.
 *  @param[in] propagate     Downtime propagation parameters.
 *
 *  @return OK on success.
 */
int notifier::schedule_downtime(
      downtime::downtime_type type,
      time_t entry_time,
      std::string const& author,
      std::string const& comment_data,
      time_t start_time,
      time_t end_time,
      bool fixed,
      unsigned long triggered_by,
      unsigned long duration,
      downtime_propagation propagate) {
  logger(dbg_functions, basic)
    << "schedule_downtime()";

  /* don't add old or invalid downtimes */
  if (start_time >= end_time || end_time <= time(NULL))
    return (ERROR);

  /* add a new downtime entry */
  downtime* dt(new downtime(
                       type,
                       this,
                       entry_time,
                       author,
                       comment_data,
                       start_time,
                       end_time,
                       fixed,
                       triggered_by,
                       duration));
  // XXX: ID computation is not good
  if (scheduled_downtime_list.empty())
    dt->set_id(1);
  else
    dt->set_id(scheduled_downtime_list.rbegin()->first + 1);
  scheduled_downtime_list.insert(std::make_pair(dt->get_id(), dt));

  /* register the scheduled downtime */
  dt->registration();

  if (type == downtime::HOST_DOWNTIME) {
    host* temp_host = static_cast<host*>(this);

    int trig;
    downtime_propagation propagate;

    switch (propagate) {
      case DOWNTIME_PROPAGATE_NONE:
        trig = 0UL;
        propagate = DOWNTIME_PROPAGATE_NONE;
        break;
      case DOWNTIME_PROPAGATE_SIMPLE:
        trig = dt->get_triggered_by();
        propagate = DOWNTIME_PROPAGATE_NONE;
        break;
      case DOWNTIME_PROPAGATE_TRIGGERED:
        trig = dt->get_triggered_by();
        propagate = DOWNTIME_PROPAGATE_TRIGGERED;
        break;
    }
    /* check all child hosts... */
    for (host_set::const_iterator
           it(temp_host->get_children().begin()),
           end(temp_host->get_children().end());
         it != end;
         ++it) {
      host* child_host(*it);

      /* recurse... */
      child_host->schedule_downtime(
        downtime::HOST_DOWNTIME,
        entry_time,
        author,
        comment_data,
        start_time,
        end_time,
        fixed,
        trig,
        duration,
        propagate);
    }
  }
  return (OK);
}




/**
 *  Tell if the current notification is escalated
 *
 *  @return a boolean
 */
bool notifier::should_be_escalated() const {

  logger(dbg_functions, basic)
    << "notifier: should_be_escalated()";

  // FIXME DBR: not implemented
  return false;
}

/**
 *  Deletes all non-persistent acknowledgement comments for a particular
 *  notifier
 */
void notifier::delete_acknowledgement_comments() {
  comment* temp_comment = NULL;
  comment* next_comment = NULL;

  /* delete comments from memory */
  //FIXME DBR: to rewrite
//  for (temp_comment = comment_list;
//       temp_comment != NULL;
//       temp_comment = next_comment) {
//    next_comment = temp_comment->next;
//    if (temp_comment->comment_type == SERVICE_COMMENT
//        && !strcmp(temp_comment->host_name, get_host_name().c_str())
//        && !strcmp(temp_comment->service_description, get_description().c_str())
//        && temp_comment->entry_type == ACKNOWLEDGEMENT_COMMENT
//        && temp_comment->persistent == false)
//      delete_comment(SERVICE_COMMENT, temp_comment->comment_id);
//  }
}




/**
 * Get the filter method associated to the given type.
 *
 * @param type The notification's type.
 *
 * @return A filter method
 */
notifier::notifier_filter notifier::_get_filter(notification_type type) const {
  return _filter[type];
}



/**************************************
*                                     *
*           Static Objects            *
*                                     *
**************************************/

notifier::notifier_filter const notifier::_filter[] = {
  &notifier::_problem_filter,
  &notifier::_recovery_filter,
  &notifier::_acknowledgement_filter,
  &notifier::_flappingstart_filter,
  &notifier::_flappingstopdisabled_filter,
  &notifier::_flappingstopdisabled_filter,
  &notifier::_downtimestart_filter,
  &notifier::_downtimestopcancelled_filter,
  &notifier::_downtimestopcancelled_filter,
  &notifier::_custom_filter
};

std::string const notifier::_notification_string[] = {
  "PROBLEM",
  "RECOVERY",
  "ACKNOWLEDGEMENT",
  "FLAPPINGSTART",
  "FLAPPINGSTOP",
  "FLAPPINGDISABLED",
  "DOWNTIMESTART",
  "DOWNTIMESTOP",
  "DOWNTIMECANCELLED",
  "CUSTOM"
};

/**************************************
*                                     *
*           Private Methods           *
*                                     *
**************************************/

/**
 * Filter method on problem notifications
 *
 * @return a boolean
 */
bool notifier::_problem_filter() {
  time_t now;
  time(&now);
  /* No notification sent if:
   *  * notifier in downtime
   *  * notifier is flapping
   *  * notifier is not configured to send notification on the current state
   *  * state is not hard.
   */
  if (is_in_downtime()
      || get_flapping()
      || !is_state_notification_enabled(get_current_state())
      || get_current_state_type() == SOFT_STATE
      || is_acknowledged())
    return false;

  int notif_number = get_current_notification_number();

  /* A PROBLEM notification has already been sent */
  if (notif_number >= 1
      && (get_current_notifications_flag() & (1 << PROBLEM))
      && get_last_state() == get_current_state()) {
    /* No notification if the delay between previous notification and now
       is less than notification_interval */
    if (now - get_last_notification() < get_notification_interval())
      return false;

  }
  return true;
}

unsigned int notifier::get_current_notifications_flag() const {
  return _current_notifications;
}

/**
 * Filter method on recovery notifications
 *
 * @return a boolean
 */
bool notifier::_recovery_filter() {

  if (is_in_downtime()
      || (get_current_notifications_flag() & (1 << PROBLEM)) == 0
      || get_current_state() != 0)
    return false;

  return true;
}

/**
 * Filter method on acknowledgement notifications
 *
 * @return a boolean
 */
bool notifier::_acknowledgement_filter() {

  if (is_in_downtime()
      || (get_current_notifications_flag() & (1 << PROBLEM)) == 0
      || get_current_state() == 0)
    return false;

  return true;
}

/**
 * Filter method on flappingstart notifications
 *
 * @return a boolean
 */
bool notifier::_flappingstart_filter() {

  if (is_in_downtime()
      || (get_current_notifications_flag() & (1 << FLAPPINGSTART)))
    return false;

  return true;
}

/**
 * Filter method on flapping stop/disabled notifications
 *
 * @return a boolean
 */
bool notifier::_flappingstopdisabled_filter() {

  if (is_in_downtime()
      || (get_current_notifications_flag() & (1 << FLAPPINGSTART)) == 0)
    return false;

  return true;
}

/**
 * Filter method on downtime start notifications
 *
 * @return a boolean
 */
bool notifier::_downtimestart_filter() {

  if (is_in_downtime())
    return false;

  return true;
}

/**
 * Filter method on downtime stop/cancelled notifications
 *
 * @return a boolean
 */
bool notifier::_downtimestopcancelled_filter() {

  if (!is_in_downtime())
    return false;

  return true;
}

/**
 * Filter method on custom notifications
 *
 * @return a boolean
 */
bool notifier::_custom_filter() {

  if (is_in_downtime())
    return false;

  return true;
}




time_t notifier::get_initial_notif_time() const {
  // FIXME DBR: to implement...
}

void notifier::set_initial_notif_time(time_t initial) {
  // FIXME DBR: to implement...
}

void notifier::set_recovery_been_sent(bool sent) {
  // FIXME DBR: to implement...
}

bool notifier::get_recovery_been_sent() const {
  // FIXME DBR: to implement...
  return false;
}

/////////////////////////////////////////////////////////
///* checks for flexible (non-fixed) host downtime that should start now */
//int check_pending_flex_host_downtime(host* hst) {
//  scheduled_downtime* temp_downtime(NULL);
//  time_t current_time(0L);
//
//  logger(dbg_functions, basic)
//    << "check_pending_flex_host_downtime()";
//
//  if (hst == NULL)
//    return (ERROR);
//
//  time(&current_time);
//
//  /* if host is currently up, nothing to do */
//  if (hst->current_state == HOST_UP)
//    return (OK);
//
//  /* check all downtime entries */
//  for (temp_downtime = scheduled_downtime_list;
//       temp_downtime != NULL;
//       temp_downtime = temp_downtime->next) {
//    if (temp_downtime->type != HOST_DOWNTIME
//        || temp_downtime->fixed == true
//        || temp_downtime->is_in_effect == true
//        || temp_downtime->triggered_by != 0)
//      continue;
//
//    /* this entry matches our host! */
//    if (find_host(temp_downtime->host_name) == hst) {
//      /* if the time boundaries are okay, start this scheduled downtime */
//      if (temp_downtime->start_time <= current_time
//          && current_time <= temp_downtime->end_time) {
//
//        logger(dbg_downtime, basic)
//          << "Flexible downtime (id=" << temp_downtime->downtime_id
//          << ") for host '" << hst->name << "' starting now...";
//
//        temp_downtime->start_flex_downtime = true;
//        handle_scheduled_downtime(temp_downtime);
//      }
//    }
//  }
//  return (OK);
//}
//
/////////////////////////////////////////////////////////

/**
 *  Checks for flexible (non-fixed) notifier downtime that should start now.
 */
void notifier::check_pending_flex_downtime() {
  time_t current_time(0L);

  logger(dbg_functions, basic)
    << "check_pending_flex_downtime()";

  time(&current_time);

  /* if notifier is currently ok, nothing to do */
  if (get_current_state() == STATE_OK)
    return ;

  /* Check all downtime entries */
  for (std::map<unsigned long, downtime*>::const_iterator
         it(scheduled_downtime_list.begin()),
         end(scheduled_downtime_list.begin());
       it != end;
       ++it) {
    downtime* temp_downtime = it->second;

    if (temp_downtime->get_type() != downtime::SERVICE_DOWNTIME
        || temp_downtime->get_fixed()
        || temp_downtime->get_in_effect()
        || temp_downtime->get_triggered_by() != 0)
      continue;

    /* This entry matches our notifier */
    /* and the time boundaries are ok, start this scheduled downtime */
    if (temp_downtime->get_parent() == this
        && temp_downtime->get_start_time() <= current_time
        && current_time <= temp_downtime->get_end_time()) {
      logger(dbg_downtime, basic)
        << "Flexible downtime (id=" << temp_downtime->get_id()
        << ") for notifier '" << get_info() << "' starting now...";

      temp_downtime->handle();
    }
  }
}

bool notifier::get_no_more_notifications() const {
  // FIXME DBR: to implement...
  return true;
}

std::string notifier::get_info() {
  std::ostringstream oss;
  service* svc = dynamic_cast<service*>(this);

  if (svc)
    oss << "Notifier: service '" << svc->get_description() << "' ; host '"
        << svc->get_host_name() << "'";
  else
    oss << "Notifier: host '"
        << svc->get_host_name() << "'";
  return oss.str();
}

/**
 *  Copy internal data members.
 *
 *  @param[in] other  Object to copy.
 */
void notifier::_internal_copy(notifier const& other) {
  _acknowledgement_timeout = other._acknowledgement_timeout;
  _contacts = other._contacts;
  _contact_groups = other._contact_groups;
  _current_acknowledgement = other._current_acknowledgement;
  _current_notification_id = other._current_notification_id;
  _current_notification_number = other._current_notification_number;
  _current_notifications = other._current_notifications;
  _escalate_notification = other._escalate_notification;
  _first_notification_delay = other._first_notification_delay;
  _in_downtime = other._in_downtime;
  _last_acknowledgement = other._last_acknowledgement;
  _last_notification = other._last_notification;
  _next_notification = other._next_notification;
  _notification_interval = other._notification_interval;
  _notification_period = other._notification_period;
  _notifications_enabled = other._notifications_enabled;
  _notified_states = other._notified_states;
  _pending_flex_downtime = other._pending_flex_downtime;
  _recovery_notification_delay = other._recovery_notification_delay;
  _scheduled_downtime_depth = other._scheduled_downtime_depth;
  return ;
}
