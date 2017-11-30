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

#include <memory>
#include <sstream>
#include "com/centreon/engine/downtime.hh"
#include "com/centreon/engine/globals.hh"
#include "com/centreon/engine/logging/logger.hh"
#include "com/centreon/engine/notifications/notifier.hh"
#include "com/centreon/shared_ptr.hh"

using namespace com::centreon;
using namespace com::centreon::engine;
using namespace com::centreon::engine::checks;
using namespace com::centreon::engine::configuration;
using namespace com::centreon::engine::logging;
using namespace com::centreon::engine::notifications;


/**************************************                                         
*                                     *                                         
*           Public Methods            *                                         
*                                     *                                         
**************************************/                                         

/**
 * Constructor.
 */
notifier::notifier()
  : _type(NONE),
    _last_notification(0),
    _current_notification_number(0),
    _notified_states(0),
    _notification_interval(60) {}

/**
 * Copy constructor.
 *
 * @param[in] other Object to copy.
 */
notifier::notifier(notifier const& other)
  : _type(other._type),
    _last_notification(other._last_notification),
    _current_notification_number(other._current_notification_number),
    _notified_states(other._notified_states),
    _notification_interval(other._notification_interval) {}

/**
 * Assignment operator.
 *
 * @param[in] other Object to copy.
 *
 * @return This object
 */
notifier& notifier::operator=(notifier const& other) {
  _type = other._type;
  _last_notification = other._last_notification;
  _current_notification_number = other._current_notification_number;
  _notified_states = other._notified_states;
  _notification_interval = other._notification_interval;
  return (*this);
}

/**
 * Destructor.
 */
notifier::~notifier() {
  logger(dbg_functions, basic)
    << "notifier: destructor";
}

void notifier::add_contactgroup(shared_ptr<engine::contactgroup> cg) {
  _contact_groups[cg->get_name()] = cg;
}

void notifier::add_contact(shared_ptr<engine::contact> user) {
  _contacts[user->get_name()] = user;
}

/**
 *  Clear the contacts list
 */
void notifier::clear_contacts() {
  _contacts.clear();
}

/**
 *  Clear the contactgroups list
 */
void notifier::clear_contactgroups() {
  _contact_groups.clear();
}

/**
 * This method tells if notifications are enabled globally
 *
 * @return a boolean
 */
bool notifier::are_notifications_enabled() const {
  return config->enable_notifications();
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

void notifier::notify(
    notification_type type,
    std::string const& author,
    std::string const& comment) {
  std::list<shared_ptr<engine::contact> > users_to_notify = get_contacts_list();
  if (users_to_notify.empty())
    return ;

  if (are_notifications_enabled()) {
    notifier_filter should_notify = _get_filter(type);
    if ((this->*should_notify)()) {
      nagios_macros mac;
      std::ostringstream oss;
      bool first_time = true;

      // Notify each contact
      for (
        std::list<shared_ptr<engine::contact> >::iterator
                                                it(users_to_notify.begin()),
                                                end(users_to_notify.end());
        it != end;
        ++it) {

        logger(dbg_notifications, most)
          << "** Notifying contact '" << (*it)->get_name() << "'";

        if (first_time)
          oss << (*it)->get_name();
        else
          oss << ',' << (*it)->get_name();
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
        set_contact::const_iterator it = config->contacts_find(author);
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
      if (_type == ACKNOWLEDGEMENT) {
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
                _notification_string[_type]);

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

      time(&_last_notification);
    }
  }
}

static bool _compare_shared_ptr(
    shared_ptr<engine::contact> const& a,
    shared_ptr<engine::contact> const& b) {
  return (*a < *b);
}

/**
 *  get the users to notify in a string form each one separated by a comma.
 *
 *  @return A string.
 */
std::list<shared_ptr<engine::contact> > notifier::get_contacts_list() {

  /* See if this notification should be escalated */
  _escalate_notification = should_be_escalated();

  std::list<shared_ptr<engine::contact> > retval;

  for (
    umap<std::string, shared_ptr<engine::contact> >::const_iterator
      it(_contacts.begin()),
      end(_contacts.end());
    it != end;
    ++it) {
    retval.push_back(it->second);
  }

  retval.sort(_compare_shared_ptr);
  retval.unique();
  return retval;
}

/**
 * This method tells if the notifier should notify on current state.
 *
 * @return a boolean
 */
bool notifier::is_state_notification_enabled(int state) const {
  return (_notified_states & (1 << state));
}

/**
 * This method tells if this checkable is in downtime.
 *
 * @return a boolean
 */
bool notifier::is_in_downtime() const {
  return _in_downtime;
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

umap<std::string, shared_ptr<engine::contact> > const& notifier::get_contacts() const {
  return _contacts;
}

umap<std::string, shared_ptr<engine::contact> >& notifier::get_contacts() {
  return _contacts;
}

umap<std::string, shared_ptr<engine::contactgroup> > const& notifier::get_contactgroups() const {
  return _contact_groups;
}

umap<std::string, shared_ptr<engine::contactgroup> >& notifier::get_contactgroups() {
  return _contact_groups;
}

/**************************************
*                                     *
*           Static Objects            *
*                                     *
**************************************/

notifier::notifier_filter notifier::_filter[] = {
  0,
  &notifier::_problem_filter,
  &notifier::_recovery_filter
};

std::string notifier::_notification_string[] = {
  "NONE",
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
  if (is_in_downtime()
      || get_flapping()
      || !is_state_notification_enabled(get_current_state()))
    return false;

  int notif_number = get_current_notification_number();

  /* We already have been notified */
  if (notif_number >= 1
      && _type == PROBLEM
      && get_last_state() == get_current_state()) {
    /* No notification if the delay between previous notification and now
       is less than notification_interval */
    if (now - get_last_notification() < _get_notification_interval())
      return false;

  }
  return true;
}

notifier::notification_type notifier::get_current_notification_type() const {
  return _type;
}

/**
 * Filter method on recovery notifications
 *
 * @return a boolean
 */
bool notifier::_recovery_filter() {

  if (is_in_downtime()
      || get_current_notification_type() != PROBLEM
      || get_current_state() != 0)
    return false;

  return true;
}

int notifier::get_current_notification_id() const {
  return _current_notification_id;
}

/**
 * Getter to the notification number.
 *
 * @return an integer.
 */
int notifier::get_current_notification_number() const {
  return _current_notification_number;
}

long notifier::_get_notification_interval() const {
  return _notification_interval;
}

long notifier::get_last_notification() const {
  return _last_notification;
}

void notifier::enable_state_notification(int state) {
  _notified_states |= (1 << state);
}

void notifier::set_last_notification(time_t last_notification) {
  _last_notification = last_notification;
}

void notifier::set_next_notification(time_t next_notification) {
  _next_notification = next_notification;
}

bool notifier::contains_contact(contact* user) const {
  std::string const& name(user->get_name());
  return contains_contact(name);
}

bool notifier::contains_contact(std::string const& username) const {
  umap<std::string, shared_ptr<contact> >::const_iterator it(
    get_contacts().find(username));
  if (it != get_contacts().end())
    return true;

  for (umap<std::string, shared_ptr<engine::contactgroup> >::const_iterator
         cgit(get_contactgroups().begin()),
         end(get_contactgroups().end());
       cgit != end;
       ++cgit) {
    if (cgit->second->contains_member(username))
      return true;
  }
  return false;
}

bool notifier::is_acknowledged() const {
  // FIXME DBR: to implement...
  return false;
}

int notifier::get_acknowledgement_type() const {
  // FIXME DBR: to implement...
  return 0;
}

void notifier::set_acknowledged(bool acked) {
  // FIXME DBR: to implement...
}

void notifier::set_acknowledgement_type(notifier::acknowledgement_type type) {
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

void notifier::set_current_notification_number(int number) {
  // FIXME DBR: to implement...
}

int notifier::get_pending_flex_downtime() const {
  // FIXME DBR: to implement...
  return 0;
}

bool notifier::check_pending_flex_downtime() {
  // FIXME DBR: to implement...
  return true;
}

time_t notifier::get_next_notification() const {
  // FIXME DBR: to implement...
  return 0;
}

bool notifier::get_no_more_notifications() const {
  // FIXME DBR: to implement...
  return true;
}

int notifier::get_scheduled_downtime_depth() const {
  return 0;
}

#if 0
#include "com/centreon/engine/broker.hh"
#include "com/centreon/engine/configuration/applier/state.hh"
#include "com/centreon/engine/globals.hh"
#include "com/centreon/engine/logging/logger.hh"
#include "com/centreon/engine/macros.hh"
#include "com/centreon/engine/neberrors.hh"
#include "com/centreon/engine/notifications.hh"
#include "com/centreon/engine/shared.hh"
#include "com/centreon/engine/statusdata.hh"
#include "com/centreon/engine/string.hh"
#include "com/centreon/engine/timeperiod.hh"
#include "com/centreon/engine/timezone_locker.hh"
#include "com/centreon/engine/utils.hh"

using namespace com::centreon;
using namespace com::centreon::engine;
using namespace com::centreon::engine::configuration::applier;
using namespace com::centreon::engine::logging;

static char const* tab_notification_str[] = {
  "NORMAL",
  "ACKNOWLEDGEMENT",
  "FLAPPINGSTART",
  "FLAPPINGSTOP",
  "FLAPPINGDISABLED",
  "DOWNTIMESTART",
  "DOWNTIMEEND",
  "DOWNTIMECANCELLED",
};

static char const* tab_host_state_str[] = {
  "UP",
  "DOWN",
  "UNREACHABLE"
};

static char const* tab_service_state_str[] = {
  "OK",
  "WARNING",
  "CRITICAL"
};

/******************************************************************/
/***************** SERVICE NOTIFICATION FUNCTIONS *****************/
/******************************************************************/

/* notify contacts about a service problem or recovery */
int service_notification(
      service* svc,
      unsigned int type,
      char* not_author,
      char* not_data,
      int options) {
  host* temp_host = NULL;
  notification* temp_notification = NULL;
  contact* temp_contact = NULL;
  time_t current_time;
  struct timeval start_time;
  struct timeval end_time;
  int escalated = false;
  int contacts_notified = 0;
  int increment_notification_number = false;
  nagios_macros mac;
  int neb_result;

  logger(dbg_functions, basic)
    << "service_notification()";

  /* get the current time */
  time(&current_time);
  gettimeofday(&start_time, NULL);

  logger(dbg_notifications, basic)
    << "** Service Notification Attempt ** Host: '" << svc->host_name
    << "', Service: '" << svc->description
    << "', Type: " << type << ", Options: " << options
    << ", Current State: " << svc->current_state
    << ", Last Notification: " << my_ctime(&svc->last_notification);

  /* if we couldn't find the host, return an error */
  if ((temp_host = svc->host_ptr) == NULL) {
    logger(dbg_notifications, basic)
      << "Couldn't find the host associated with this service, so we "
      "won't send a notification!";
    return (ERROR);
  }

  /* check the viability of sending out a service notification */
  if (check_service_notification_viability(
        svc,
        type,
        options) == ERROR) {
    logger(dbg_notifications, basic)
      << "Notification viability test failed.  No notification will "
      "be sent out.";
    return (OK);
  }

  logger(dbg_notifications, basic)
    << "Notification viability test passed.";

  /* should the notification number be increased? */
  if (type == NOTIFICATION_NORMAL
      || (options & NOTIFICATION_OPTION_INCREMENT)) {
    svc->current_notification_number++;
    increment_notification_number = true;
  }

  logger(dbg_notifications, more)
    << "Current notification number: "
    << svc->current_notification_number
    << " (" << (increment_notification_number == true ? "incremented)" : "unchanged)");

  /* save and increase the current notification id */
  svc->current_notification_id = next_notification_id;
  next_notification_id++;

  logger(dbg_notifications, most)
    << "Creating list of contacts to be notified.";

  /* create the contact notification list for this service */
  memset(&mac, 0, sizeof(mac));
  create_notification_list_from_service(&mac, svc, options, &escalated);

  /* send data to event broker */
  end_time.tv_sec = 0L;
  end_time.tv_usec = 0L;
  neb_result = broker_notification_data(
    NEBTYPE_NOTIFICATION_START,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    SERVICE_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)svc,
    not_author,
    not_data,
    escalated,
    0,
    NULL);
  if (NEBERROR_CALLBACKCANCEL == neb_result) {
    free_notification_list();
    return (ERROR);
  }
  else if (NEBERROR_CALLBACKOVERRIDE == neb_result) {
    free_notification_list();
    return (OK);
  }

  /* we have contacts to notify... */
  if (notification_list != NULL) {

    /* grab the macro variables */
    grab_host_macros_r(&mac, temp_host);
    grab_service_macros_r(&mac, svc);

    /* if this notification has an author, attempt to lookup the associated contact */
    if (not_author != NULL) {

      /* see if we can find the contact - first by name, then by alias */
      if ((temp_contact = find_contact(not_author)) == NULL) {
        for (temp_contact = contact_list;
	     temp_contact != NULL;
             temp_contact = temp_contact->next) {
          if (!strcmp(temp_contact->alias, not_author))
            break;
        }
      }
    }

    /* get author and comment macros */
    string::setstr(mac.x[MACRO_NOTIFICATIONAUTHOR], not_author);
    string::setstr(mac.x[MACRO_NOTIFICATIONCOMMENT], not_data);
    if (temp_contact) {
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME], temp_contact->name);
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS], temp_contact->alias);
    }
    else {
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME]);
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS]);
    }

    /* NOTE: these macros are deprecated and will likely disappear in next major release */
    /* if this is an acknowledgement, get author and comment macros */
    if (type == NOTIFICATION_ACKNOWLEDGEMENT) {
      string::setstr(mac.x[MACRO_SERVICEACKAUTHOR], not_author);
      string::setstr(mac.x[MACRO_SERVICEACKCOMMENT], not_data);
      if (temp_contact) {
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME], temp_contact->name);
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS], temp_contact->alias);
      }
      else {
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME]);
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS]);
      }
    }

    /* set the notification type macro */
    if (type == NOTIFICATION_ACKNOWLEDGEMENT)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "ACKNOWLEDGEMENT");
    else if (type == NOTIFICATION_FLAPPINGSTART)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGSTART");
    else if (type == NOTIFICATION_FLAPPINGSTOP)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGSTOP");
    else if (type == NOTIFICATION_FLAPPINGDISABLED)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGDISABLED");
    else if (type == NOTIFICATION_DOWNTIMESTART)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMESTART");
    else if (type == NOTIFICATION_DOWNTIMEEND)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMEEND");
    else if (type == NOTIFICATION_DOWNTIMECANCELLED)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMECANCELLED");
    else if (type == NOTIFICATION_CUSTOM)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "CUSTOM");
    else if (svc->current_state == STATE_OK)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "RECOVERY");
    else
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "PROBLEM");

    /* set the notification number macro */
    string::setstr(mac.x[MACRO_SERVICENOTIFICATIONNUMBER], svc->current_notification_number);

    /* the $NOTIFICATIONNUMBER$ macro is maintained for backward compatability */
    char const* notificationnumber(mac.x[MACRO_SERVICENOTIFICATIONNUMBER]);
    string::setstr(mac.x[MACRO_NOTIFICATIONNUMBER], notificationnumber ? notificationnumber : "");

    /* set the notification id macro */
    string::setstr(mac.x[MACRO_SERVICENOTIFICATIONID], svc->current_notification_id);

    /* notify each contact (duplicates have been removed) */
    for (temp_notification = notification_list;
         temp_notification != NULL;
         temp_notification = temp_notification->next) {

      /* grab the macro variables for this contact */
      grab_contact_macros_r(&mac, temp_notification->cntct);

      /* clear summary macros (they are customized for each contact) */
      clear_summary_macros_r(&mac);

      /* notify this contact */
      int result = notify_contact_of_service(
                     &mac,
                     temp_notification->cntct,
                     svc,
                     type,
                     not_author,
                     not_data,
                     options,
                     escalated);

      /* keep track of how many contacts were notified */
      if (result == OK)
        contacts_notified++;
    }

    /* free memory allocated to the notification list */
    free_notification_list();

    /* clear summary macros so they will be regenerated without contact filters when needed next */
    clear_summary_macros_r(&mac);

    if (type == NOTIFICATION_NORMAL) {

      /* adjust last/next notification time and notification flags if we notified someone */
      if (contacts_notified > 0) {

        /* calculate the next acceptable re-notification time */
        svc->next_notification = get_next_service_notification_time(
                                   svc,
                                   current_time);

        logger(dbg_notifications, basic)
          << contacts_notified << " contacts were notified.  "
          "Next possible notification time: "
          << my_ctime(&svc->next_notification);

        /* update the last notification time for this service (this is needed for rescheduling later notifications) */
        svc->last_notification = current_time;

        /* update notifications flags */
        if (svc->current_state == STATE_UNKNOWN)
          svc->notified_on_unknown = true;
        else if (svc->current_state == STATE_WARNING)
          svc->notified_on_warning = true;
        else if (svc->current_state == STATE_CRITICAL)
          svc->notified_on_critical = true;
      }

      /* we didn't end up notifying anyone */
      else if (increment_notification_number == true) {

        /* adjust current notification number */
        svc->current_notification_number--;

        logger(dbg_notifications, basic)
          << "No contacts were notified.  Next possible "
          "notification time: " << my_ctime(&svc->next_notification);
      }
    }

    logger(dbg_notifications, basic)
      << contacts_notified << " contacts were notified.";
  }

  /* there were no contacts, so no notification really occurred... */
  else {
    /* readjust current notification number, since one didn't go out */
    if (increment_notification_number == true)
      svc->current_notification_number--;

    logger(dbg_notifications, basic)
      << "No contacts were found for notification purposes.  "
      "No notification was sent out.";
  }

  /* get the time we finished */
  gettimeofday(&end_time, NULL);

  /* send data to event broker */
  broker_notification_data(
    NEBTYPE_NOTIFICATION_END,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    SERVICE_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)svc,
    not_author,
    not_data,
    escalated,
    contacts_notified,
    NULL);

  /* update the status log with the service information */
  update_service_status(svc, false);

  /* clear volatile macros */
  clear_volatile_macros_r(&mac);

  /* Update recovery been sent parameter */
  if (svc->current_state == STATE_OK)
    service_other_props[std::make_pair(
                        svc->host_ptr->name,
                        svc->description)].recovery_been_sent = true;

  return (OK);
}

/* checks the viability of sending out a service alert (top level filters) */
int check_service_notification_viability(
      service* svc,
      unsigned int type,
      int options) {
  host* temp_host;
  timeperiod* temp_period;
  time_t current_time;
  time_t timeperiod_start;

  logger(dbg_functions, basic)
    << "check_service_notification_viability()";

  /* forced notifications bust through everything */
  if (options & NOTIFICATION_OPTION_FORCED) {
    logger(dbg_notifications, more)
      << "This is a forced service notification, so we'll send it out.";
    return (OK);
  }

  /* get current time */
  time(&current_time);

  /* are notifications enabled? */
  if (config->enable_notifications() == false) {
    logger(dbg_notifications, more)
      << "Notifications are disabled, so service notifications will "
      "not be sent out.";
    return (ERROR);
  }

  /* find the host this service is associated with */
  if ((temp_host = (host*)svc->host_ptr) == NULL) {
    logger(dbg_notifications, more)
      << "Couldn't find the host associated with this service, "
      "so we won't send a notification.";
    return (ERROR);
  }

  /* if the service has no notification period, inherit one from the host */
  temp_period = svc->notification_period_ptr;
  if (temp_period == NULL)
    temp_period = svc->host_ptr->notification_period_ptr;

  // See if the service can have notifications sent out at this time.
  {
    timezone_locker lock(get_service_timezone(
                           svc->host_name,
                           svc->description));
    if (check_time_against_period(current_time, temp_period) == ERROR) {
      logger(dbg_notifications, more)
        << "This service shouldn't have notifications sent out "
        "at this time.";

      // Calculate the next acceptable notification time,
      // once the next valid time range arrives...
      if (type == NOTIFICATION_NORMAL) {
        get_next_valid_time(
          current_time,
          &timeperiod_start,
          svc->notification_period_ptr);

        // Looks like there are no valid notification times defined, so
        // schedule the next one far into the future (one year)...
        if (timeperiod_start == (time_t)0)
          svc->next_notification
            = (time_t)(current_time + (60 * 60 * 24 * 365));
        // Else use the next valid notification time.
        else
          svc->next_notification = timeperiod_start;
        logger(dbg_notifications, more)
          << "Next possible notification time: "
          << my_ctime(&svc->next_notification);
      }

      return (ERROR);
    }
  }

  /* are notifications temporarily disabled for this service? */
  if (svc->notifications_enabled == false) {
    logger(dbg_notifications, more)
      << "Notifications are temporarily disabled for "
      "this service, so we won't send one out.";
    return (ERROR);
  }

  /*********************************************/
  /*** SPECIAL CASE FOR CUSTOM NOTIFICATIONS ***/
  /*********************************************/

  /* custom notifications are good to go at this point... */
  if (type == NOTIFICATION_CUSTOM) {
    if (svc->scheduled_downtime_depth > 0
        || temp_host->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't send custom notification during "
        "scheduled downtime.";
      return (ERROR);
    }
    return (OK);
  }

  /****************************************/
  /*** SPECIAL CASE FOR ACKNOWLEGEMENTS ***/
  /****************************************/

  /* acknowledgements only have to pass three general filters, although they have another test of their own... */
  if (type == NOTIFICATION_ACKNOWLEDGEMENT) {

    /* don't send an acknowledgement if there isn't a problem... */
    if (svc->current_state == STATE_OK) {
      logger(dbg_notifications, more)
        << "The service is currently OK, so we won't send an "
        "acknowledgement.";
      return (ERROR);
    }

    /* acknowledgement viability test passed, so the notification can be sent out */
    return (OK);
  }


  /****************************************/
  /*** SPECIAL CASE FOR FLAPPING ALERTS ***/
  /****************************************/

  /* flapping notifications only have to pass three general filters */
  if (type == NOTIFICATION_FLAPPINGSTART
      || type == NOTIFICATION_FLAPPINGSTOP
      || type == NOTIFICATION_FLAPPINGDISABLED) {

    /* don't send a notification if we're not supposed to... */
    if (svc->notify_on_flapping == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about FLAPPING events for this "
        "service.";
      return (ERROR);
    }

    /* don't send notifications during scheduled downtime */
    if (svc->scheduled_downtime_depth > 0
        || temp_host->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about FLAPPING events during "
        "scheduled downtime.";
      return (ERROR);
    }

    /* flapping viability test passed, so the notification can be sent out */
    return (OK);
  }

  /****************************************/
  /*** SPECIAL CASE FOR DOWNTIME ALERTS ***/
  /****************************************/

  /* downtime notifications only have to pass three general filters */
  if (type == NOTIFICATION_DOWNTIMESTART
      || type == NOTIFICATION_DOWNTIMEEND
      || type == NOTIFICATION_DOWNTIMECANCELLED) {

    /* don't send a notification if we're not supposed to... */
    if (svc->notify_on_downtime == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about DOWNTIME events for "
        "this service.";
      return (ERROR);
    }

    /* don't send notifications during scheduled downtime (for service only, not host) */
    if (svc->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about DOWNTIME events during "
        "scheduled downtime.";
      return (ERROR);
    }

    /* downtime viability test passed, so the notification can be sent out */
    return (OK);
  }

  /****************************************/
  /*** NORMAL NOTIFICATIONS ***************/
  /****************************************/

  /* is this a hard problem/recovery? */
  if (svc->state_type == SOFT_STATE) {
    logger(dbg_notifications, more)
      << "This service is in a soft state, so we won't send a "
      "notification out.";
    return (ERROR);
  }

  /* has this problem already been acknowledged? */
  if (svc->problem_has_been_acknowledged == true) {
    logger(dbg_notifications, more)
      << "This service problem has already been acknowledged, "
      "so we won't send a notification out.";
    return (ERROR);
  }

  /* check service notification dependencies */
  if (check_service_dependencies(
        svc,
        NOTIFICATION_DEPENDENCY) == DEPENDENCIES_FAILED) {
    logger(dbg_notifications, more)
      << "Service notification dependencies for this service "
      "have failed, so we won't sent a notification out.";
    return (ERROR);
  }

  /* check host notification dependencies */
  if (check_host_dependencies(
        temp_host,
        NOTIFICATION_DEPENDENCY) == DEPENDENCIES_FAILED) {
    logger(dbg_notifications, more)
      << "Host notification dependencies for this service have failed, "
      "so we won't sent a notification out.";
    return (ERROR);
  }

  /* see if we should notify about problems with this service */
  if (svc->current_state == STATE_UNKNOWN
      && svc->notify_on_unknown == false) {
    logger(dbg_notifications, more)
      << "We shouldn't notify about UNKNOWN states for this service.";
    return (ERROR);
  }
  if (svc->current_state == STATE_WARNING
      && svc->notify_on_warning == false) {
    logger(dbg_notifications, more)
      << "We shouldn't notify about WARNING states for this service.";
    return (ERROR);
  }
  if (svc->current_state == STATE_CRITICAL
      && svc->notify_on_critical == false) {
    logger(dbg_notifications, more)
      << "We shouldn't notify about CRITICAL states for this service.";
    return (ERROR);
  }
  if (svc->current_state == STATE_OK) {
    if (svc->notify_on_recovery == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about RECOVERY states for this service.";
      return (ERROR);
    }
    if (!(svc->notified_on_unknown == true
          || svc->notified_on_warning == true
          || svc->notified_on_critical == true)) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about this recovery.";
      return (ERROR);
    }
  }

  /* see if enough time has elapsed for first notification */
  if (type == NOTIFICATION_NORMAL
      && (svc->current_notification_number == 0
          || (svc->current_state == STATE_OK
                && !service_other_props[std::make_pair(
                    svc->host_ptr->name,
                    svc->description)].recovery_been_sent))) {

    /* get the time at which a notification should have been sent */
    time_t& initial_notif_time(
              service_other_props[std::make_pair(
                                         svc->host_ptr->name,
                                         svc->description)].initial_notif_time);

    /* if not set, set it to now */
    if (!initial_notif_time)
      initial_notif_time = time(NULL);

    double notification_delay = (svc->current_state != STATE_OK ?
             svc->first_notification_delay
             : service_other_props[std::make_pair(
                 svc->host_ptr->name,
                 svc->description)].recovery_notification_delay)
        * config->interval_length();

    if (current_time
        < (time_t)(initial_notif_time
                   + (time_t)(notification_delay))) {
      if (svc->current_state == STATE_OK)
        logger(dbg_notifications, more)
          << "Not enough time has elapsed since the service changed to a "
          "OK state, so we should not notify about this problem yet";
      else
        logger(dbg_notifications, more)
          << "Not enough time has elapsed since the service changed to a "
          "non-OK state, so we should not notify about this problem yet";
      return (ERROR);
    }
  }

  /* if this service is currently flapping, don't send the notification */
  if (svc->is_flapping == true) {
    logger(dbg_notifications, more)
      << "This service is currently flapping, so we won't send "
      "notifications.";
    return (ERROR);
  }

  /* if this service is currently in a scheduled downtime period, don't send the notification */
  if (svc->scheduled_downtime_depth > 0) {
    logger(dbg_notifications, more)
      << "This service is currently in a scheduled downtime, so "
      "we won't send notifications.";
    return (ERROR);
  }

  /* if this host is currently in a scheduled downtime period, don't send the notification */
  if (temp_host->scheduled_downtime_depth > 0) {
    logger(dbg_notifications, more)
      << "The host this service is associated with is currently in "
      "a scheduled downtime, so we won't send notifications.";
    return (ERROR);
  }

  /***** RECOVERY NOTIFICATIONS ARE GOOD TO GO AT THIS POINT IF ANY OTHER NOTIFICATION WAS SENT *****/
  if (svc->current_state == STATE_OK)
    return ((svc->current_notification_number > 0)
            ? OK
            : ERROR);

  /* don't notify contacts about this service problem again if the notification interval is set to 0 */
  if (svc->no_more_notifications == true) {
    logger(dbg_notifications, more)
      << "We shouldn't re-notify contacts about this service problem.";
    return (ERROR);
  }

  /* if the host is down or unreachable, don't notify contacts about service failures */
  if (temp_host->current_state != HOST_UP) {
    logger(dbg_notifications, more)
      << "The host is either down or unreachable, so we won't "
      "notify contacts about this service.";
    return (ERROR);
  }

  /* don't notify if we haven't waited long enough since the last time (and the service is not marked as being volatile) */
  if ((current_time < svc->next_notification)
      && svc->is_volatile == false) {
    logger(dbg_notifications, more)
      << "We haven't waited long enough to re-notify contacts "
      "about this service.";
    logger(dbg_notifications, more)
      << "Next valid notification time: "
      << my_ctime(&svc->next_notification);
    return (ERROR);
  }

  return (OK);
}

/* check viability of sending out a service notification to a specific contact (contact-specific filters) */
int check_contact_service_notification_viability(
      contact* cntct,
      service* svc,
      unsigned int type,
      int options) {

  logger(dbg_functions, basic)
    << "check_contact_service_notification_viability()";

  logger(dbg_notifications, most)
    << "** Checking service notification viability "
    "for contact '" << cntct->name << "'...";

  /* forced notifications bust through everything */
  if (options & NOTIFICATION_OPTION_FORCED) {
    logger(dbg_notifications, more)
      << "This is a forced service notification, so we'll "
      "send it out to this contact.";
    return (OK);
  }

  /* are notifications enabled? */
  if (cntct->service_notifications_enabled == false) {
    logger(dbg_notifications, most)
      << "Service notifications are disabled for this contact.";
    return (ERROR);
  }

  // See if the contact can be notified at this time.
  {
    timezone_locker lock(get_contact_timezone(cntct->name));
    if (check_time_against_period(
          time(NULL),
          cntct->service_notification_period_ptr) == ERROR) {
      logger(dbg_notifications, most)
        << "This contact shouldn't be notified at this time.";
      return (ERROR);
    }
  }

  /*********************************************/
  /*** SPECIAL CASE FOR CUSTOM NOTIFICATIONS ***/
  /*********************************************/

  /* custom notifications are good to go at this point... */
  if (type == NOTIFICATION_CUSTOM)
    return (OK);

  /****************************************/
  /*** SPECIAL CASE FOR FLAPPING ALERTS ***/
  /****************************************/

  if (type == NOTIFICATION_FLAPPINGSTART
      || type == NOTIFICATION_FLAPPINGSTOP
      || type == NOTIFICATION_FLAPPINGDISABLED) {

    if (cntct->notify_on_service_flapping == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about FLAPPING "
        "service events.";
      return (ERROR);
    }

    return (OK);
  }

  /****************************************/
  /*** SPECIAL CASE FOR DOWNTIME ALERTS ***/
  /****************************************/

  if (type == NOTIFICATION_DOWNTIMESTART
      || type == NOTIFICATION_DOWNTIMEEND
      || type == NOTIFICATION_DOWNTIMECANCELLED) {

    if (cntct->notify_on_service_downtime == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about DOWNTIME "
        "service events.";
      return (ERROR);
    }

    return (OK);
  }

  /*************************************/
  /*** ACKS AND NORMAL NOTIFICATIONS ***/
  /*************************************/

  /* see if we should notify about problems with this service */
  if (svc->current_state == STATE_UNKNOWN
      && cntct->notify_on_service_unknown == false) {
    logger(dbg_notifications, most)
      << "We shouldn't notify this contact about UNKNOWN "
      "service states.";
    return (ERROR);
  }

  if (svc->current_state == STATE_WARNING
      && cntct->notify_on_service_warning == false) {
    logger(dbg_notifications, most)
      << "We shouldn't notify this contact about WARNING "
      "service states.";
    return (ERROR);
  }

  if (svc->current_state == STATE_CRITICAL
      && cntct->notify_on_service_critical == false) {
    logger(dbg_notifications, most)
      << "We shouldn't notify this contact about CRITICAL "
      "service states.";
    return (ERROR);
  }

  if (svc->current_state == STATE_OK) {

    if (cntct->notify_on_service_recovery == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about RECOVERY "
        "service states.";
      return (ERROR);
    }

    if (!((svc->notified_on_unknown == true
           && cntct->notify_on_service_unknown == true)
          || (svc->notified_on_warning == true
              && cntct->notify_on_service_warning == true)
          || (svc->notified_on_critical == true
              && cntct->notify_on_service_critical == true))) {
      logger(dbg_notifications, most)
        << "We shouldn't notify about this recovery.";
      return (ERROR);
    }
  }

  logger(dbg_notifications, most)
    << "** Service notification viability for contact '"
    << cntct->name <<"' PASSED.";

  return (OK);
}

/* notify a specific contact about a service problem or recovery */
int notify_contact_of_service(
      nagios_macros* mac,
      contact* cntct,
      service* svc,
      int type,
      char* not_author,
      char* not_data,
      int options,
      int escalated) {
  commandsmember* temp_commandsmember = NULL;
  char* command_name = NULL;
  char* command_name_ptr = NULL;
  char* raw_command = NULL;
  char* processed_command = NULL;
  int early_timeout = false;
  double exectime;
  struct timeval start_time, end_time;
  struct timeval method_start_time, method_end_time;
  int macro_options = STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS;
  int neb_result;

  logger(dbg_functions, basic)
    << "notify_contact_of_service()";
  logger(dbg_notifications, most)
    << "** Attempting to notifying contact '" << cntct->name << "'...";

  /* check viability of notifying this user */
  /* acknowledgements are no longer excluded from this test - added 8/19/02 Tom Bertelson */
  if (check_contact_service_notification_viability(
        cntct,
        svc,
        type,
        options) == ERROR)
    return (ERROR);

  logger(dbg_notifications, most)
    << "** Notifying contact '" << cntct->name << "'";

  /* get start time */
  gettimeofday(&start_time, NULL);

  /* send data to event broker */
  end_time.tv_sec = 0L;
  end_time.tv_usec = 0L;
  neb_result = broker_contact_notification_data(
                 NEBTYPE_CONTACTNOTIFICATION_START,
                 NEBFLAG_NONE,
                 NEBATTR_NONE,
                 SERVICE_NOTIFICATION,
                 type,
                 start_time,
                 end_time,
                 (void*)svc,
                 cntct,
                 not_author,
                 not_data,
                 escalated,
                 NULL);
  if (NEBERROR_CALLBACKCANCEL == neb_result)
    return (ERROR);
  else if (NEBERROR_CALLBACKOVERRIDE == neb_result)
    return (OK);

  /* process all the notification commands this user has */
  for (temp_commandsmember = cntct->service_notification_commands;
       temp_commandsmember != NULL;
       temp_commandsmember = temp_commandsmember->next) {

    /* get start time */
    gettimeofday(&method_start_time, NULL);

    /* send data to event broker */
    method_end_time.tv_sec = 0L;
    method_end_time.tv_usec = 0L;
    neb_result = broker_contact_notification_method_data(
                   NEBTYPE_CONTACTNOTIFICATIONMETHOD_START,
                   NEBFLAG_NONE,
                   NEBATTR_NONE,
                   SERVICE_NOTIFICATION,
                   type,
                   method_start_time,
                   method_end_time,
                   (void*)svc,
                   cntct,
                   temp_commandsmember->cmd,
                   not_author,
                   not_data,
                   escalated,
                   NULL);
    if (NEBERROR_CALLBACKCANCEL == neb_result)
      break;
    else if (NEBERROR_CALLBACKOVERRIDE == neb_result)
      continue;

    /* get the raw command line */
    get_raw_command_line_r(
      mac,
      temp_commandsmember->command_ptr,
      temp_commandsmember->cmd,
      &raw_command,
      macro_options);
    if (raw_command == NULL)
      continue;

    logger(dbg_notifications, most)
      << "Raw notification command: " << raw_command;

    /* process any macros contained in the argument */
    process_macros_r(
      mac,
      raw_command,
      &processed_command,
      macro_options);
    if (processed_command == NULL)
      continue;

    /* get the command name */
    command_name = string::dup(temp_commandsmember->cmd);
    command_name_ptr = strtok(command_name, "!");

    /* run the notification command... */

    logger(dbg_notifications, most)
      << "Processed notification command: " << processed_command;

    /* log the notification to program log file */
    if (config->log_notifications() == true) {
      char const* service_state_str("UNKNOWN");
      if ((unsigned int)svc->current_state < sizeof(tab_service_state_str) / sizeof(*tab_service_state_str))
        service_state_str = tab_service_state_str[svc->current_state];

      char const* notification_str("");
      if ((unsigned int)type < sizeof(tab_notification_str) / sizeof(*tab_notification_str))
        notification_str = tab_notification_str[type];

      std::string info;
      switch (type) {
      case NOTIFICATION_CUSTOM:
        notification_str = "CUSTOM";

      case NOTIFICATION_ACKNOWLEDGEMENT:
        info
          .append(";").append(not_author ? not_author : "")
          .append(";").append(not_data ? not_data : "");
        break;
      }

      std::string service_notification_state;
      if (strcmp(notification_str, "NORMAL") == 0)
        service_notification_state.append(service_state_str);
      else
        service_notification_state
          .append(notification_str)
          .append(" (")
          .append(service_state_str)
          .append(")");

      logger(log_service_notification, basic)
        << "SERVICE NOTIFICATION: " << cntct->name << ';'
        << svc->host_name << ';' << svc->description << ';'
        << service_notification_state << ";"
        << command_name_ptr << ';'
        << (svc->plugin_output ? svc->plugin_output : "")
        << info;
    }

    /* run the notification command */
    try {
      my_system_r(
        mac,
        processed_command,
        config->notification_timeout(),
        &early_timeout,
        &exectime,
        NULL,
        0);
    } catch (std::exception const& e) {
      logger(log_runtime_error, basic)
        << "Error: can't execute service notification '"
        << cntct->name << "' : " << e.what();
    }

    /* check to see if the notification command timed out */
    if (early_timeout == true) {
      logger(log_service_notification | log_runtime_warning, basic)
        << "Warning: Contact '" << cntct->name
        << "' service notification command '" << processed_command
        << "' timed out after " << config->notification_timeout()
        << " seconds";
    }

    /* free memory */
    delete[] command_name;
    delete[] raw_command;
    delete[] processed_command;

    /* get end time */
    gettimeofday(&method_end_time, NULL);

    /* send data to event broker */
    broker_contact_notification_method_data(
      NEBTYPE_CONTACTNOTIFICATIONMETHOD_END,
      NEBFLAG_NONE,
      NEBATTR_NONE,
      SERVICE_NOTIFICATION,
      type,
      method_start_time,
      method_end_time,
      (void*)svc,
      cntct,
      temp_commandsmember->cmd,
      not_author,
      not_data,
      escalated,
      NULL);
  }

  /* get end time */
  gettimeofday(&end_time, NULL);

  /* update the contact's last service notification time */
  cntct->last_service_notification = start_time.tv_sec;

  /* send data to event broker */
  broker_contact_notification_data(
    NEBTYPE_CONTACTNOTIFICATION_END,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    SERVICE_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)svc,
    cntct,
    not_author,
    not_data,
    escalated,
    NULL);
  return (OK);
}

/* checks to see if a service escalation entry is a match for the current service notification */
int is_valid_escalation_for_service_notification(
      service* svc,
      serviceescalation* se,
      int options) {
  int notification_number = 0;
  time_t current_time = 0L;
  service* temp_service = NULL;

  logger(dbg_functions, basic)
    << "is_valid_escalation_for_service_notification()";

  /* get the current time */
  time(&current_time);

  /* if this is a recovery, really we check for who got notified about a previous problem */
  if (svc->current_state == STATE_OK)
    notification_number = svc->current_notification_number - 1;
  else
    notification_number = svc->current_notification_number;

  /* this entry if it is not for this service */
  temp_service = se->service_ptr;
  if (temp_service == NULL || temp_service != svc)
    return (false);

  /*** EXCEPTION ***/
  /* broadcast options go to everyone, so this escalation is valid */
  if (options & NOTIFICATION_OPTION_BROADCAST)
    return (true);

  /* skip this escalation if it happens later */
  if (se->first_notification > notification_number)
    return (false);

  /* skip this escalation if it has already passed */
  if (se->last_notification != 0
      && se->last_notification < notification_number)
    return (false);

  /* skip this escalation if it has a timeperiod and the current time isn't valid */
  if (se->escalation_period != NULL
      && check_time_against_period(
           current_time,
           se->escalation_period_ptr) == ERROR)
    return (false);

  /* skip this escalation if the state options don't match */
  if (svc->current_state == STATE_OK
      && se->escalate_on_recovery == false)
    return (false);
  else if (svc->current_state == STATE_WARNING
           && se->escalate_on_warning == false)
    return (false);
  else if (svc->current_state == STATE_UNKNOWN
           && se->escalate_on_unknown == false)
    return (false);
  else if (svc->current_state == STATE_CRITICAL
           && se->escalate_on_critical == false)
    return (false);

  return (true);
}

/**
 *  Checks to see whether a service notification should be escalated.
 *
 *  @param[in] svc Service.
 *
 *  @return true if service notification should be escalated, false if
 *          it should not.
 */
// FIXME DBR: not implemented...
int should_service_notification_be_escalated(service* svc) {
  // Debug.
  logger(dbg_functions, basic)
    << "should_service_notification_be_escalated()";

  // Browse service escalations related to this service.
  typedef umultimap<std::pair<std::string, std::string>, shared_ptr<serviceescalation> > collection;
  std::pair<collection::iterator, collection::iterator> p;
  p = state::instance().serviceescalations().equal_range(
        std::make_pair(svc->host_name, svc->description));
  while (p.first != p.second) {
    serviceescalation* temp_se(p.first->second.get());

    // We found a matching entry, so escalate this notification!
    if (is_valid_escalation_for_service_notification(
          svc,
          temp_se,
          NOTIFICATION_OPTION_NONE) == true) {
      logger(dbg_notifications, more)
        << "Service notification WILL be escalated.";
      return (true);
    }

    ++p.first;
  }
  logger(dbg_notifications, more)
    << "Service notification will NOT be escalated.";
  return (false);
}

/* given a service, create a list of contacts to be notified, removing duplicates */
int create_notification_list_from_service(
      nagios_macros* mac,
      service* svc, int options,
      int* escalated) {
  contactsmember* temp_contactsmember = NULL;
  contact* temp_contact = NULL;
  contactgroupsmember* temp_contactgroupsmember = NULL;
  contactgroup* temp_contactgroup = NULL;
  int escalate_notification = false;

  logger(dbg_functions, basic)
    << "create_notification_list_from_service()";

  /* see if this notification should be escalated */
  escalate_notification = should_service_notification_be_escalated(svc);

  /* set the escalation flag */
  *escalated = escalate_notification;

  /* make sure there aren't any leftover contacts */
  free_notification_list();

  /* set the escalation macro */
  string::setstr(mac->x[MACRO_NOTIFICATIONISESCALATED], escalate_notification);

  if (options & NOTIFICATION_OPTION_BROADCAST)
    logger(dbg_notifications, more)
      << "This notification will be BROADCAST to all "
      "(escalated and normal) contacts...";

  /* use escalated contacts for this notification */
  if (escalate_notification == true
      || (options & NOTIFICATION_OPTION_BROADCAST)) {

    logger(dbg_notifications, more)
      << "Adding contacts from service escalation(s) to "
      "notification list.";

    std::pair<std::string, std::string>
      id(std::make_pair(svc->host_name, svc->description));
    umultimap<std::pair<std::string, std::string>, shared_ptr<serviceescalation> > const&
      escalations(state::instance().serviceescalations());
    for (umultimap<std::pair<std::string, std::string>, shared_ptr<serviceescalation> >::const_iterator
           it(escalations.find(id)), end(escalations.end());
         it != end && it->first == id;
         ++it) {
      serviceescalation* temp_se(&*it->second);

      /* skip this entry if it isn't appropriate */
      if (is_valid_escalation_for_service_notification(
            svc,
            temp_se,
            options) == false)
        continue;

      logger(dbg_notifications, most)
        << "Adding individual contacts from service escalation(s) "
        "to notification list.";

      /* add all individual contacts for this escalation entry */
      for (temp_contactsmember = temp_se->contacts;
           temp_contactsmember != NULL;
           temp_contactsmember = temp_contactsmember->next) {
        if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
          continue;
        add_notification(mac, temp_contact);
      }

      logger(dbg_notifications, most)
        << "Adding members of contact groups from service escalation(s) "
        "to notification list.";

      /* add all contacts that belong to contactgroups for this escalation */
      for (temp_contactgroupsmember = temp_se->contact_groups;
           temp_contactgroupsmember != NULL;
           temp_contactgroupsmember = temp_contactgroupsmember->next) {
        logger(dbg_notifications, most)
          << "Adding members of contact group '"
          << temp_contactgroupsmember->group_name
          << "' for service escalation to notification list.";

        if (!(temp_contactgroup = temp_contactgroupsmember->group_ptr))
          continue;
        for (temp_contactsmember = temp_contactgroup->members;
             temp_contactsmember != NULL;
             temp_contactsmember = temp_contactsmember->next) {
          if (!(temp_contact = temp_contactsmember->contact_ptr))
            continue;
          add_notification(mac, temp_contact);
        }
      }
    }
  }

  /* else use normal, non-escalated contacts */
  if (escalate_notification == false
      || (options & NOTIFICATION_OPTION_BROADCAST)) {

    logger(dbg_notifications, more)
      << "Adding normal contacts for service to notification list.";

    /* add all individual contacts for this service */
    for (temp_contactsmember = svc->contacts;
         temp_contactsmember != NULL;
         temp_contactsmember = temp_contactsmember->next) {
      if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
        continue;
      add_notification(mac, temp_contact);
    }

    /* add all contacts that belong to contactgroups for this service */
    for (temp_contactgroupsmember = svc->contact_groups;
         temp_contactgroupsmember != NULL;
         temp_contactgroupsmember = temp_contactgroupsmember->next) {
      logger(dbg_notifications, most)
        << "Adding members of contact group '"
        << temp_contactgroupsmember->group_name
        << "' for service to notification list.";

      if (!(temp_contactgroup = temp_contactgroupsmember->group_ptr))
        continue;
      for (temp_contactsmember = temp_contactgroup->members;
           temp_contactsmember != NULL;
           temp_contactsmember = temp_contactsmember->next) {
        if (!(temp_contact = temp_contactsmember->contact_ptr))
          continue;
        add_notification(mac, temp_contact);
      }
    }
  }

  return (OK);
}

/******************************************************************/
/******************* HOST NOTIFICATION FUNCTIONS ******************/
/******************************************************************/

/* notify all contacts for a host that the entire host is down or up */
int host_notification(
      host* hst,
      unsigned int type,
      char* not_author,
      char* not_data,
      int options) {
  notification* temp_notification = NULL;
  contact* temp_contact = NULL;
  time_t current_time;
  struct timeval start_time;
  struct timeval end_time;
  int escalated = false;
  int contacts_notified = 0;
  int increment_notification_number = false;
  nagios_macros mac;
  int neb_result;

  /* get the current time */
  time(&current_time);
  gettimeofday(&start_time, NULL);

  logger(dbg_notifications, basic)
    << "** Host Notification Attempt ** Host: '" << hst->name
    << "', Type: " << type << ", Options: " << options
    << ", Current State: " << hst->current_state
    << ", Last Notification: "
    << my_ctime(&hst->last_host_notification);


  /* check viability of sending out a host notification */
  if (check_host_notification_viability(hst, type, options) == ERROR) {
    logger(dbg_notifications, basic)
      << "Notification viability test failed.  No notification will "
      "be sent out.";
    return (OK);
  }

  /* allocate memory for local macro */
  memset(&mac, 0, sizeof(mac));

  logger(dbg_notifications, basic)
    << "Notification viability test passed.";

  /* should the notification number be increased? */
  if (type == NOTIFICATION_NORMAL
      || (options & NOTIFICATION_OPTION_INCREMENT)) {
    hst->current_notification_number++;
    increment_notification_number = true;
  }

  logger(dbg_notifications, more)
    << "Current notification number: "
    << hst->current_notification_number
    << " (" << (increment_notification_number == true ? "incremented" : "unchanged") << ")";

  /* save and increase the current notification id */
  hst->current_notification_id = next_notification_id;
  next_notification_id++;

  logger(dbg_notifications, most)
    << "Creating list of contacts to be notified.";

  /* create the contact notification list for this host */
  create_notification_list_from_host(&mac, hst, options, &escalated);

  /* send data to event broker */
  end_time.tv_sec = 0L;
  end_time.tv_usec = 0L;
  neb_result = broker_notification_data(
    NEBTYPE_NOTIFICATION_START,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    HOST_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)hst,
    not_author,
    not_data,
    escalated,
    0,
    NULL);
  if (NEBERROR_CALLBACKCANCEL == neb_result) {
    free_notification_list();
    return (ERROR);
  }
  else if (NEBERROR_CALLBACKOVERRIDE == neb_result) {
    free_notification_list();
    return (OK);
  }

  /* there are contacts to be notified... */
  if (notification_list != NULL) {

    /* grab the macro variables */
    grab_host_macros_r(&mac, hst);

    /* if this notification has an author, attempt to lookup the associated contact */
    if (not_author != NULL) {

      /* see if we can find the contact - first by name, then by alias */
      if ((temp_contact = find_contact(not_author)) == NULL) {
        for (temp_contact = contact_list;
             temp_contact != NULL;
             temp_contact = temp_contact->next) {
          if (!strcmp(temp_contact->alias, not_author))
            break;
        }
      }

    }

    /* get author and comment macros */
    string::setstr(mac.x[MACRO_NOTIFICATIONAUTHOR], not_author);
    string::setstr(mac.x[MACRO_NOTIFICATIONCOMMENT], not_data);
    if (temp_contact) {
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME], temp_contact->name);
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS], temp_contact->alias);
    }
    else {
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORNAME]);
      string::setstr(mac.x[MACRO_NOTIFICATIONAUTHORALIAS]);
    }

    /* NOTE: these macros are deprecated and will likely disappear in next major release */
    /* if this is an acknowledgement, get author and comment macros */
    if (type == NOTIFICATION_ACKNOWLEDGEMENT) {
      string::setstr(mac.x[MACRO_HOSTACKAUTHOR], not_author);
      string::setstr(mac.x[MACRO_HOSTACKCOMMENT], not_data);
      if (temp_contact) {
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME], temp_contact->name);
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS], temp_contact->alias);
      }
      else {
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORNAME]);
        string::setstr(mac.x[MACRO_SERVICEACKAUTHORALIAS]);
      }
    }

    /* set the notification type macro */
    if (type == NOTIFICATION_ACKNOWLEDGEMENT)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "ACKNOWLEDGEMENT");
    else if (type == NOTIFICATION_FLAPPINGSTART)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGSTART");
    else if (type == NOTIFICATION_FLAPPINGSTOP)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGSTOP");
    else if (type == NOTIFICATION_FLAPPINGDISABLED)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "FLAPPINGDISABLED");
    else if (type == NOTIFICATION_DOWNTIMESTART)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMESTART");
    else if (type == NOTIFICATION_DOWNTIMEEND)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMEEND");
    else if (type == NOTIFICATION_DOWNTIMECANCELLED)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "DOWNTIMECANCELLED");
    else if (type == NOTIFICATION_CUSTOM)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "CUSTOM");
    else if (hst->current_state == HOST_UP)
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "RECOVERY");
    else
      string::setstr(mac.x[MACRO_NOTIFICATIONTYPE], "PROBLEM");

    /* set the notification number macro */
    string::setstr(mac.x[MACRO_HOSTNOTIFICATIONNUMBER], hst->current_notification_number);

    /* the $NOTIFICATIONNUMBER$ macro is maintained for backward compatability */
    char const* notificationnumber(mac.x[MACRO_HOSTNOTIFICATIONNUMBER]);
    string::setstr(mac.x[MACRO_NOTIFICATIONNUMBER], notificationnumber ? notificationnumber : "");

    /* set the notification id macro */
    string::setstr(mac.x[MACRO_HOSTNOTIFICATIONID], hst->current_notification_id);

    /* notify each contact (duplicates have been removed) */
    for (temp_notification = notification_list;
         temp_notification != NULL;
         temp_notification = temp_notification->next) {

      /* grab the macro variables for this contact */
      grab_contact_macros_r(&mac, temp_notification->cntct);

      /* clear summary macros (they are customized for each contact) */
      clear_summary_macros_r(&mac);

      /* notify this contact */
      int result = notify_contact_of_host(
                     &mac,
                     temp_notification->cntct,
                     hst,
                     type,
                     not_author,
                     not_data,
                     options,
                     escalated);

      /* keep track of how many contacts were notified */
      if (result == OK)
        contacts_notified++;
    }

    /* free memory allocated to the notification list */
    free_notification_list();

    /* clear summary macros so they will be regenerated without contact filters when needednext */
    clear_summary_macros_r(&mac);

    if (type == NOTIFICATION_NORMAL) {

      /* adjust last/next notification time and notification flags if we notified someone */
      if (contacts_notified > 0) {

        /* calculate the next acceptable re-notification time */
        hst->next_host_notification = get_next_host_notification_time(
                                        hst,
                                        current_time);

        /* update the last notification time for this host (this is needed for scheduling the next problem notification) */
        hst->last_host_notification = current_time;

        /* update notifications flags */
        if (hst->current_state == HOST_DOWN)
          hst->notified_on_down = true;
        else if (hst->current_state == HOST_UNREACHABLE)
          hst->notified_on_unreachable = true;

        logger(dbg_notifications, basic)
          << contacts_notified << " contacts were notified.  "
          "Next possible notification time: "
          << my_ctime(&hst->next_host_notification);
      }

      /* we didn't end up notifying anyone */
      else if (increment_notification_number == true) {

        /* adjust current notification number */
        hst->current_notification_number--;

        logger(dbg_notifications, basic)
          << "No contacts were notified.  Next possible "
          "notification time: "
          << my_ctime(&hst->next_host_notification);
      }
    }

    logger(dbg_notifications, basic)
      << contacts_notified << " contacts were notified.";
  }

  /* there were no contacts, so no notification really occurred... */
  else {

    /* adjust notification number, since no notification actually went out */
    if (increment_notification_number == true)
      hst->current_notification_number--;

    logger(dbg_notifications, basic)
      << "No contacts were found for notification purposes.  "
      "No notification was sent out.";
  }

  /* get the time we finished */
  gettimeofday(&end_time, NULL);

  /* send data to event broker */
  broker_notification_data(
    NEBTYPE_NOTIFICATION_END,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    HOST_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)hst,
    not_author,
    not_data,
    escalated,
    contacts_notified,
    NULL);

  /* update the status log with the host info */
  update_host_status(hst, false);

  /* clear volatile macros */
  clear_volatile_macros_r(&mac);

  /* Update recovery been sent parameter */
  if (hst->current_state == HOST_UP)
    host_other_props[hst->name].recovery_been_sent = true;

  return (OK);
}

/* checks viability of sending a host notification */
int check_host_notification_viability(
      host* hst,
      unsigned int type,
      int options) {
  time_t current_time;
  time_t timeperiod_start;

  logger(dbg_functions, basic)
    << "check_host_notification_viability()";

  /* forced notifications bust through everything */
  if (options & NOTIFICATION_OPTION_FORCED) {
    logger(dbg_notifications, more)
      << "This is a forced host notification, so we'll send it out.";
    return (OK);
  }

  /* get current time */
  time(&current_time);

  /* are notifications enabled? */
  if (config->enable_notifications() == false) {
    logger(dbg_notifications, more)
      << "Notifications are disabled, so host notifications will not "
      "be sent out.";
    return (ERROR);
  }

  // See if the host can have notifications sent out at this time.
  {
    timezone_locker lock(get_host_timezone(hst->name));
    if (check_time_against_period(
          current_time,
          hst->notification_period_ptr) == ERROR) {
      logger(dbg_notifications, more)
        << "This host shouldn't have notifications sent out at "
           "this time.";

      // If this is a normal notification, calculate the next acceptable
      // notification time, once the next valid time range arrives...
      if (type == NOTIFICATION_NORMAL) {
        get_next_valid_time(
          current_time,
          &timeperiod_start,
          hst->notification_period_ptr);

        // It looks like there is no notification time defined, so
        // schedule next one far into the future (one year)...
        if (timeperiod_start == (time_t)0)
          hst->next_host_notification
            = (time_t)(current_time + (60 * 60 * 24 * 365));
        // Else use the next valid notification time.
        else
          hst->next_host_notification = timeperiod_start;
        logger(dbg_notifications, more)
          << "Next possible notification time: "
          << my_ctime(&hst->next_host_notification);
      }
      return (ERROR);
    }
  }

  /* are notifications temporarily disabled for this host? */
  if (hst->notifications_enabled == false) {
    logger(dbg_notifications, more)
      << "Notifications are temporarily disabled for this host, "
      "so we won't send one out.";
    return (ERROR);
  }

  /*********************************************/
  /*** SPECIAL CASE FOR CUSTOM NOTIFICATIONS ***/
  /*********************************************/

  /* custom notifications are good to go at this point... */
  if (type == NOTIFICATION_CUSTOM) {
    if (hst->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't send custom notification during "
        "scheduled downtime.";
      return (ERROR);
    }
    return (OK);
  }

  /****************************************/
  /*** SPECIAL CASE FOR ACKNOWLEGEMENTS ***/
  /****************************************/

  /* acknowledgements only have to pass three general filters, although they have another test of their own... */
  if (type == NOTIFICATION_ACKNOWLEDGEMENT) {

    /* don't send an acknowledgement if there isn't a problem... */
    if (hst->current_state == HOST_UP) {
      logger(dbg_notifications, more)
        << "The host is currently UP, so we won't send "
        "an acknowledgement.";
      return (ERROR);
    }

    /* acknowledgement viability test passed, so the notification can be sent out */
    return (OK);
  }

  /*****************************************/
  /*** SPECIAL CASE FOR FLAPPING ALERTS ***/
  /*****************************************/

  /* flapping notifications only have to pass three general filters */
  if (type == NOTIFICATION_FLAPPINGSTART
      || type == NOTIFICATION_FLAPPINGSTOP
      || type == NOTIFICATION_FLAPPINGDISABLED) {

    /* don't send a notification if we're not supposed to... */
    if (hst->notify_on_flapping == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about FLAPPING events for this host.";
      return (ERROR);
    }

    /* don't send notifications during scheduled downtime */
    if (hst->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about FLAPPING events during "
        "scheduled downtime.";
      return (ERROR);
    }

    /* flapping viability test passed, so the notification can be sent out */
    return (OK);
  }

  /*****************************************/
  /*** SPECIAL CASE FOR DOWNTIME ALERTS ***/
  /*****************************************/

  /* flapping notifications only have to pass three general filters */
  if (type == NOTIFICATION_DOWNTIMESTART
      || type == NOTIFICATION_DOWNTIMEEND
      || type == NOTIFICATION_DOWNTIMECANCELLED) {

    /* don't send a notification if we're not supposed to... */
    if (hst->notify_on_downtime == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about DOWNTIME events for this host.";
      return (ERROR);
    }

    /* don't send notifications during scheduled downtime */
    if (hst->scheduled_downtime_depth > 0) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about DOWNTIME events during "
        "scheduled downtime!";
      return (ERROR);
    }

    /* downtime viability test passed, so the notification can be sent out */
    return (OK);
  }

  /****************************************/
  /*** NORMAL NOTIFICATIONS ***************/
  /****************************************/

  /* is this a hard problem/recovery? */
  if (hst->state_type == SOFT_STATE) {
    logger(dbg_notifications, more)
      << "This host is in a soft state, so we won't send "
      "a notification out.";
    return (ERROR);
  }

  /* has this problem already been acknowledged? */
  if (hst->problem_has_been_acknowledged == true) {
    logger(dbg_notifications, more)
      << "This host problem has already been acknowledged, "
      "so we won't send a notification out!";
    return (ERROR);
  }

  /* check notification dependencies */
  if (check_host_dependencies(
        hst,
        NOTIFICATION_DEPENDENCY) == DEPENDENCIES_FAILED) {
    logger(dbg_notifications, more)
      << "Notification dependencies for this host have failed, "
      "so we won't sent a notification out!";
    return (ERROR);
  }

  /* see if we should notify about problems with this host */
  if (hst->current_state == HOST_UNREACHABLE
      && hst->notify_on_unreachable == false) {
    logger(dbg_notifications, more)
      << "We shouldn't notify about UNREACHABLE status for this host.";
    return (ERROR);
  }
  if (hst->current_state == HOST_DOWN
      && hst->notify_on_down == false) {
    logger(dbg_notifications, more)
      << "We shouldn't notify about DOWN states for this host.";
    return (ERROR);
  }
  if (hst->current_state == HOST_UP) {

    if (hst->notify_on_recovery == false) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about RECOVERY states for this host.";
      return (ERROR);
    }
    if (!(hst->notified_on_down == true
          || hst->notified_on_unreachable == true)) {
      logger(dbg_notifications, more)
        << "We shouldn't notify about this recovery.";
      return (ERROR);
    }

  }

  /* see if enough time has elapsed for first notification */
  if (type == NOTIFICATION_NORMAL
      && (hst->current_notification_number == 0
         || (hst->current_state == HOST_UP &&
             !host_other_props[std::string(hst->name)].recovery_been_sent))) {

    /* get the time at which a notification should have been sent */
    time_t& initial_notif_time(host_other_props[hst->name].initial_notif_time);

    /* if not set, set it to now */
    if (!initial_notif_time)
      initial_notif_time = time(NULL);

    double notification_delay = (hst->current_state != HOST_UP ?
             hst->first_notification_delay
             : host_other_props[std::string(hst->name)].recovery_notification_delay)
        * config->interval_length();

    if (current_time
        < (time_t)(initial_notif_time
                   + (time_t)(notification_delay))) {
      if (hst->current_state == HOST_UP)
        logger(dbg_notifications, more)
          << "Not enough time has elapsed since the host changed to an "
          "UP state (or since program start), so we shouldn't notify "
          "about this problem yet.";
      else
        logger(dbg_notifications, more)
          << "Not enough time has elapsed since the host changed to a "
          "non-UP state (or since program start), so we shouldn't notify "
          "about this problem yet.";
      return (ERROR);
    }
  }

  /* if this host is currently flapping, don't send the notification */
  if (hst->is_flapping == true) {
    logger(dbg_notifications, more)
      << "This host is currently flapping, so we won't "
      "send notifications.";
    return (ERROR);
  }

  /* if this host is currently in a scheduled downtime period, don't send the notification */
  if (hst->scheduled_downtime_depth > 0) {
    logger(dbg_notifications, more)
      << "This host is currently in a scheduled downtime, "
      "so we won't send notifications.";
    return (ERROR);
  }

  /***** RECOVERY NOTIFICATIONS ARE GOOD TO GO AT THIS POINT *****/
  if (hst->current_state == HOST_UP)
    return (OK);

  /* check if we shouldn't renotify contacts about the host problem */
  if (hst->no_more_notifications == true) {
    logger(dbg_notifications, more)
      << "We shouldn't re-notify contacts about this host problem.";
    return (ERROR);
  }

  /* check if its time to re-notify the contacts about the host... */
  if (current_time < hst->next_host_notification) {
    logger(dbg_notifications, more)
      << "Its not yet time to re-notify the contacts "
      "about this host problem...";
    logger(dbg_notifications, more)
      << "Next acceptable notification time: "
      << my_ctime(&hst->next_host_notification);
    return (ERROR);
  }

  return (OK);
}

/* checks the viability of notifying a specific contact about a host */
int check_contact_host_notification_viability(
      contact* cntct,
      host* hst,
      unsigned int type,
      int options) {
  logger(dbg_functions, basic)
    << "check_contact_host_notification_viability()";
  logger(dbg_notifications, most)
    << "** Checking host notification viability for contact '"
    << cntct->name << "'...";

  /* forced notifications bust through everything */
  if (options & NOTIFICATION_OPTION_FORCED) {
    logger(dbg_notifications, most)
      << "This is a forced host notification, so we'll "
      "send it out for this contact.";
    return (OK);
  }

  /* are notifications enabled? */
  if (cntct->host_notifications_enabled == false) {
    logger(dbg_notifications, most)
      << "Host notifications are disabled for this contact.";
    return (ERROR);
  }

  // See if the contact can be notified at this time.
  {
    timezone_locker lock(get_contact_timezone(cntct->name));
    if (check_time_against_period(
          time(NULL),
          cntct->host_notification_period_ptr) == ERROR) {
      logger(dbg_notifications, most)
        << "This contact shouldn't be notified at this time.";
      return (ERROR);
    }
  }

  /*********************************************/
  /*** SPECIAL CASE FOR CUSTOM NOTIFICATIONS ***/
  /*********************************************/

  /* custom notifications are good to go at this point... */
  if (type == NOTIFICATION_CUSTOM)
    return (OK);

  /****************************************/
  /*** SPECIAL CASE FOR FLAPPING ALERTS ***/
  /****************************************/

  if (type == NOTIFICATION_FLAPPINGSTART
      || type == NOTIFICATION_FLAPPINGSTOP
      || type == NOTIFICATION_FLAPPINGDISABLED) {

    if (cntct->notify_on_host_flapping == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about FLAPPING "
        "host events.";
      return (ERROR);
    }

    return (OK);
  }

  /****************************************/
  /*** SPECIAL CASE FOR DOWNTIME ALERTS ***/
  /****************************************/

  if (type == NOTIFICATION_DOWNTIMESTART
      || type == NOTIFICATION_DOWNTIMEEND
      || type == NOTIFICATION_DOWNTIMECANCELLED) {

    if (cntct->notify_on_host_downtime == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about DOWNTIME "
        "host events.";
      return (ERROR);
    }

    return (OK);
  }

  /*************************************/
  /*** ACKS AND NORMAL NOTIFICATIONS ***/
  /*************************************/

  /* see if we should notify about problems with this host */
  if (hst->current_state == HOST_DOWN
      && cntct->notify_on_host_down == false) {
    logger(dbg_notifications, most)
      << "We shouldn't notify this contact about DOWN states.";
    return (ERROR);
  }

  if (hst->current_state == HOST_UNREACHABLE
      && cntct->notify_on_host_unreachable == false) {
    logger(dbg_notifications, most)
      << "We shouldn't notify this contact about UNREACHABLE states,";
    return (ERROR);
  }

  if (hst->current_state == HOST_UP) {

    if (cntct->notify_on_host_recovery == false) {
      logger(dbg_notifications, most)
        << "We shouldn't notify this contact about RECOVERY states.";
      return (ERROR);
    }

    if (!((hst->notified_on_down == true
           && cntct->notify_on_host_down == true)
          || (hst->notified_on_unreachable == true
              && cntct->notify_on_host_unreachable == true))) {
      logger(dbg_notifications, most)
        << "We shouldn't notify about this recovery.";
      return (ERROR);
    }

  }

  logger(dbg_notifications, most)
    << "** Host notification viability for contact '"
    << cntct->name << "' PASSED.";

  return (OK);
}

/* notify a specific contact that an entire host is down or up */
int notify_contact_of_host(
      nagios_macros* mac,
      contact* cntct,
      host* hst,
      int type,
      char* not_author,
      char* not_data,
      int options,
      int escalated) {
  commandsmember* temp_commandsmember = NULL;
  char* command_name = NULL;
  char* command_name_ptr = NULL;
  char* raw_command = NULL;
  char* processed_command = NULL;
  int early_timeout = false;
  double exectime;
  struct timeval start_time;
  struct timeval end_time;
  struct timeval method_start_time;
  struct timeval method_end_time;
  int macro_options = STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS;
  int neb_result;

  logger(dbg_functions, basic)
    << "notify_contact_of_host()";
  logger(dbg_notifications, most)
    << "** Attempting to notifying contact '" << cntct->name << "'...";

  /* check viability of notifying this user about the host */
  /* acknowledgements are no longer excluded from this test - added 8/19/02 Tom Bertelson */
  if (check_contact_host_notification_viability(
        cntct,
        hst,
        type,
        options) == ERROR)
    return (ERROR);

  logger(dbg_notifications, most)
    << "** Notifying contact '" << cntct->name << "'";

  /* get start time */
  gettimeofday(&start_time, NULL);

  /* send data to event broker */
  end_time.tv_sec = 0L;
  end_time.tv_usec = 0L;
  neb_result = broker_contact_notification_data(
                 NEBTYPE_CONTACTNOTIFICATION_START,
                 NEBFLAG_NONE,
                 NEBATTR_NONE,
                 HOST_NOTIFICATION,
                 type,
                 start_time,
                 end_time,
                 (void*)hst,
                 cntct,
                 not_author,
                 not_data,
                 escalated,
                 NULL);
  if (NEBERROR_CALLBACKCANCEL == neb_result)
    return (ERROR);
  else if (NEBERROR_CALLBACKOVERRIDE == neb_result)
    return (OK);

  /* process all the notification commands this user has */
  for (temp_commandsmember = cntct->host_notification_commands;
       temp_commandsmember != NULL;
       temp_commandsmember = temp_commandsmember->next) {

    /* get start time */
    gettimeofday(&method_start_time, NULL);

    /* send data to event broker */
    method_end_time.tv_sec = 0L;
    method_end_time.tv_usec = 0L;
    neb_result = broker_contact_notification_method_data(
                   NEBTYPE_CONTACTNOTIFICATIONMETHOD_START,
                   NEBFLAG_NONE,
                   NEBATTR_NONE,
                   HOST_NOTIFICATION,
                   type,
                   method_start_time,
                   method_end_time,
                   (void*)hst,
                   cntct,
                   temp_commandsmember->cmd,
                   not_author,
                   not_data,
                   escalated,
                   NULL);
    if (NEBERROR_CALLBACKCANCEL == neb_result)
      break;
    else if (NEBERROR_CALLBACKOVERRIDE == neb_result)
      continue;

    /* get the raw command line */
    get_raw_command_line_r(
      mac,
      temp_commandsmember->command_ptr,
      temp_commandsmember->cmd,
      &raw_command,
      macro_options);
    if (raw_command == NULL)
      continue;

    logger(dbg_notifications, most)
      << "Raw notification command: " << raw_command;

    /* process any macros contained in the argument */
    process_macros_r(
      mac,
      raw_command,
      &processed_command,
      macro_options);
    if (processed_command == NULL)
      continue;

    /* get the command name */
    command_name = string::dup(temp_commandsmember->cmd);
    command_name_ptr = strtok(command_name, "!");

    /* run the notification command... */

    logger(dbg_notifications, most)
      << "Processed notification command: " << processed_command;

    /* log the notification to program log file */
    if (config->log_notifications() == true) {
      char const* host_state_str("UP");
      if ((unsigned int)hst->current_state < sizeof(tab_host_state_str) / sizeof(*tab_host_state_str))
        host_state_str = tab_host_state_str[hst->current_state];

      char const* notification_str("");
      if ((unsigned int)type < sizeof(tab_notification_str) / sizeof(*tab_notification_str))
        notification_str = tab_notification_str[type];

      std::string info;
      switch (type) {
      case NOTIFICATION_CUSTOM:
        notification_str = "CUSTOM";

      case NOTIFICATION_ACKNOWLEDGEMENT:
        info
          .append(";").append(not_author ? not_author : "")
          .append(";").append(not_data ? not_data : "");
        break;
      }

      std::string host_notification_state;
      if (strcmp(notification_str, "NORMAL") == 0)
        host_notification_state.append(host_state_str);
      else
        host_notification_state
          .append(notification_str)
          .append(" (")
          .append(host_state_str)
          .append(")");

      logger(log_host_notification, basic)
        << "HOST NOTIFICATION: " << cntct->name
        << ';' << hst->name << ';' << host_notification_state
        << ";" << command_name_ptr
        << ';' << (hst->plugin_output ? hst->plugin_output : "")
        << info;
    }

    /* run the notification command */
    try {
      my_system_r(
        mac,
        processed_command,
        config->notification_timeout(),
        &early_timeout,
        &exectime,
        NULL,
        0);
    } catch (std::exception const& e) {
      logger(log_runtime_error, basic)
        << "Error: can't execute host notification '"
        << cntct->name << "' : " << e.what();
    }

    /* check to see if the notification timed out */
    if (early_timeout == true) {
      logger(log_host_notification | log_runtime_warning, basic)
        << "Warning: Contact '" << cntct->name
        << "' host notification command '" << processed_command
        << "' timed out after " << config->notification_timeout()
        << " seconds";
    }

    /* free memory */
    delete[] command_name;
    delete[] raw_command;
    delete[] processed_command;

    /* get end time */
    gettimeofday(&method_end_time, NULL);

    /* send data to event broker */
    broker_contact_notification_method_data(
      NEBTYPE_CONTACTNOTIFICATIONMETHOD_END,
      NEBFLAG_NONE,
      NEBATTR_NONE,
      HOST_NOTIFICATION,
      type,
      method_start_time,
      method_end_time,
      (void*)hst,
      cntct,
      temp_commandsmember->cmd,
      not_author,
      not_data,
      escalated,
      NULL);
  }

  /* get end time */
  gettimeofday(&end_time, NULL);

  /* update the contact's last host notification time */
  cntct->last_host_notification = start_time.tv_sec;

  /* send data to event broker */
  broker_contact_notification_data(
    NEBTYPE_CONTACTNOTIFICATION_END,
    NEBFLAG_NONE,
    NEBATTR_NONE,
    HOST_NOTIFICATION,
    type,
    start_time,
    end_time,
    (void*)hst,
    cntct,
    not_author,
    not_data,
    escalated,
    NULL);

  return (OK);
}

/* checks to see if a host escalation entry is a match for the current host notification */
int is_valid_escalation_for_host_notification(
      host* hst,
      hostescalation* he,
      int options) {
  int notification_number = 0;
  time_t current_time = 0L;
  host* temp_host = NULL;

  logger(dbg_functions, basic)
    << "is_valid_escalation_for_host_notification()";

  /* get the current time */
  time(&current_time);

  /* if this is a recovery, really we check for who got notified about a previous problem */
  if (hst->current_state == HOST_UP)
    notification_number = hst->current_notification_number - 1;
  else
    notification_number = hst->current_notification_number;

  /* find the host this escalation entry is associated with */
  temp_host = he->host_ptr;
  if (temp_host == NULL || temp_host != hst)
    return (false);

  /*** EXCEPTION ***/
  /* broadcast options go to everyone, so this escalation is valid */
  if (options & NOTIFICATION_OPTION_BROADCAST)
    return (true);

  /* skip this escalation if it happens later */
  if (he->first_notification > notification_number)
    return (false);

  /* skip this escalation if it has already passed */
  if (he->last_notification != 0
      && he->last_notification < notification_number)
    return (false);

  /* skip this escalation if it has a timeperiod and the current time isn't valid */
  if (he->escalation_period != NULL
      && check_time_against_period(
           current_time,
           he->escalation_period_ptr) == ERROR)
    return (false);

  /* skip this escalation if the state options don't match */
  if (hst->current_state == HOST_UP
      && he->escalate_on_recovery == false)
    return (false);
  else if (hst->current_state == HOST_DOWN
           && he->escalate_on_down == false)
    return (false);
  else if (hst->current_state == HOST_UNREACHABLE
           && he->escalate_on_unreachable == false)
    return (false);

  return (true);
}

/* checks to see whether a host notification should be escalation */
int should_host_notification_be_escalated(host* hst) {
  logger(dbg_functions, basic)
    << "should_host_notification_be_escalated()";

  if (hst == NULL)
    return (false);

  std::string id(hst->name);
  umultimap<std::string, shared_ptr<hostescalation> > const&
    escalations(state::instance().hostescalations());
  for (umultimap<std::string, shared_ptr<hostescalation> >::const_iterator
         it(escalations.find(id)), end(escalations.end());
       it != end && it->first == id;
       ++it) {
    hostescalation* temp_he(&*it->second);

    /* we found a matching entry, so escalate this notification! */
    if (is_valid_escalation_for_host_notification(
          hst,
          temp_he,
          NOTIFICATION_OPTION_NONE) == true)
      return (true);
  }

  logger(dbg_notifications, more)
    << "Host notification will NOT be escalated.";

  return (false);
}

/* given a host, create a list of contacts to be notified, removing duplicates */
int create_notification_list_from_host(
      nagios_macros* mac,
      host* hst,
      int options,
      int* escalated) {
  contactsmember* temp_contactsmember = NULL;
  contact* temp_contact = NULL;
  contactgroupsmember* temp_contactgroupsmember = NULL;
  contactgroup* temp_contactgroup = NULL;
  int escalate_notification = false;

  logger(dbg_functions, basic)
    << "create_notification_list_from_host()";

  /* see if this notification should be escalated */
  escalate_notification = should_host_notification_be_escalated(hst);

  /* set the escalation flag */
  *escalated = escalate_notification;

  /* make sure there aren't any leftover contacts */
  free_notification_list();

  /* set the escalation macro */
  string::setstr(mac->x[MACRO_NOTIFICATIONISESCALATED], escalate_notification);

  if (options & NOTIFICATION_OPTION_BROADCAST)
    logger(dbg_notifications, more)
      << "This notification will be BROADCAST to all (escalated and "
      "normal) contacts...";

  /* use escalated contacts for this notification */
  if (escalate_notification == true
      || (options & NOTIFICATION_OPTION_BROADCAST)) {

    logger(dbg_notifications, more)
      << "Adding contacts from host escalation(s) to "
      "notification list.";

    std::string id(hst->name);
    umultimap<std::string, shared_ptr<hostescalation> > const&
      escalations(state::instance().hostescalations());
    for (umultimap<std::string, shared_ptr<hostescalation> >::const_iterator
           it(escalations.find(id)), end(escalations.end());
         it != end && it->first == id;
         ++it) {
      hostescalation* temp_he(&*it->second);

      /* see if this escalation if valid for this notification */
      if (is_valid_escalation_for_host_notification(
            hst,
            temp_he,
            options) == false)
        continue;

      logger(dbg_notifications, most)
        << "Adding individual contacts from host escalation(s) "
        "to notification list.";

      /* add all individual contacts for this escalation */
      for (temp_contactsmember = temp_he->contacts;
           temp_contactsmember != NULL;
           temp_contactsmember = temp_contactsmember->next) {
        if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
          continue;
        add_notification(mac, temp_contact);
      }

      logger(dbg_notifications, most)
        << "Adding members of contact groups from host "
        "escalation(s) to notification list.";

      /* add all contacts that belong to contactgroups for this escalation */
      for (temp_contactgroupsmember = temp_he->contact_groups;
           temp_contactgroupsmember != NULL;
           temp_contactgroupsmember = temp_contactgroupsmember->next) {
        logger(dbg_notifications, most)
          << "Adding members of contact group '"
          << temp_contactgroupsmember->group_name
          << "' for host escalation to notification list.";

        if ((temp_contactgroup = temp_contactgroupsmember->group_ptr) == NULL)
          continue;
        for (temp_contactsmember = temp_contactgroup->members;
             temp_contactsmember != NULL;
             temp_contactsmember = temp_contactsmember->next) {
          if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
            continue;
          add_notification(mac, temp_contact);
        }
      }
    }
  }

  /* use normal, non-escalated contacts for this notification */
  if (escalate_notification == false
      || (options & NOTIFICATION_OPTION_BROADCAST)) {

    logger(dbg_notifications, more)
      << "Adding normal contacts for host to notification list.";

    logger(dbg_notifications, most)
      << "Adding individual contacts for host to notification list.";

    /* add all individual contacts for this host */
    for (temp_contactsmember = hst->contacts;
         temp_contactsmember != NULL;
         temp_contactsmember = temp_contactsmember->next) {
      if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
        continue;
      add_notification(mac, temp_contact);
    }

    logger(dbg_notifications, most)
      << "Adding members of contact groups for host to "
      "notification list.";

    /* add all contacts that belong to contactgroups for this host */
    for (temp_contactgroupsmember = hst->contact_groups;
         temp_contactgroupsmember != NULL;
         temp_contactgroupsmember = temp_contactgroupsmember->next) {
      logger(dbg_notifications, most)
        << "Adding members of contact group '"
        << temp_contactgroupsmember->group_name
        << "' for host to notification list.";

      if (!(temp_contactgroup = temp_contactgroupsmember->group_ptr))
        continue;
      for (temp_contactsmember = temp_contactgroup->members;
           temp_contactsmember != NULL;
           temp_contactsmember = temp_contactsmember->next) {
        if ((temp_contact = temp_contactsmember->contact_ptr) == NULL)
          continue;
        add_notification(mac, temp_contact);
      }
    }
  }

  return (OK);
}

/******************************************************************/
/***************** NOTIFICATION TIMING FUNCTIONS ******************/
/******************************************************************/

/* calculates next acceptable re-notification time for a service */
time_t get_next_service_notification_time(service* svc, time_t offset) {
  time_t next_notification = 0L;
  double interval_to_use = 0.0;
  serviceescalation* temp_se = NULL;
  int have_escalated_interval = false;

  logger(dbg_functions, basic)
    << "get_next_service_notification_time()";
  logger(dbg_notifications, most)
    << "Calculating next valid notification time...";

  /* default notification interval */
  interval_to_use = svc->notification_interval;

  logger(dbg_notifications, most)
    << "Default interval: " << interval_to_use;

  /* search all the escalation entries for valid matches for this service (at its current notification number) */
  for (temp_se = serviceescalation_list;
       temp_se != NULL;
       temp_se = temp_se->next) {

    /* interval < 0 means to use non-escalated interval */
    if (temp_se->notification_interval < 0.0)
      continue;

    /* skip this entry if it isn't appropriate */
    if (is_valid_escalation_for_service_notification(
          svc,
          temp_se,
          NOTIFICATION_OPTION_NONE) == false)
      continue;

    logger(dbg_notifications, most)
      << "Found a valid escalation w/ interval of "
      << temp_se->notification_interval;

    /* if we haven't used a notification interval from an escalation yet, use this one */
    if (have_escalated_interval == false) {
      have_escalated_interval = true;
      interval_to_use = temp_se->notification_interval;
    }

    /* else use the shortest of all valid escalation intervals */
    else if (temp_se->notification_interval < interval_to_use)
      interval_to_use = temp_se->notification_interval;

    logger(dbg_notifications, most)
      << "New interval: " << interval_to_use;
  }

  /* if notification interval is 0, we shouldn't send any more problem notifications (unless service is volatile) */
  if (interval_to_use == 0.0 && svc->is_volatile == false)
    svc->no_more_notifications = true;
  else
    svc->no_more_notifications = false;

  logger(dbg_notifications, most)
    << "Interval used for calculating next valid "
    "notification time: " << interval_to_use;

  /* calculate next notification time */
  next_notification = offset + static_cast<time_t>(interval_to_use * config->interval_length());
  return (next_notification);
}

/* calculates next acceptable re-notification time for a host */
time_t get_next_host_notification_time(host* hst, time_t offset) {
  time_t next_notification = 0L;
  double interval_to_use = 0.0;
  hostescalation* temp_he = NULL;
  int have_escalated_interval = false;

  logger(dbg_functions, basic)
    << "get_next_host_notification_time()";
  logger(dbg_notifications, most)
    << "Calculating next valid notification time...";

  /* default notification interval */
  interval_to_use = hst->notification_interval;

  logger(dbg_notifications, most)
    << "Default interval: " << interval_to_use;

  /* check all the host escalation entries for valid matches for this host (at its current notification number) */
  for (temp_he = hostescalation_list;
       temp_he != NULL;
       temp_he = temp_he->next) {

    /* interval < 0 means to use non-escalated interval */
    if (temp_he->notification_interval < 0.0)
      continue;

    /* skip this entry if it isn't appropriate */
    if (is_valid_escalation_for_host_notification(
          hst,
          temp_he,
          NOTIFICATION_OPTION_NONE) == false)
      continue;

    logger(dbg_notifications, most)
      << "Found a valid escalation w/ interval of "
      << temp_he->notification_interval;

    /* if we haven't used a notification interval from an escalation yet, use this one */
    if (have_escalated_interval == false) {
      have_escalated_interval = true;
      interval_to_use = temp_he->notification_interval;
    }

    /* else use the shortest of all valid escalation intervals  */
    else if (temp_he->notification_interval < interval_to_use)
      interval_to_use = temp_he->notification_interval;

    logger(dbg_notifications, most)
      << "New interval: " << interval_to_use;
  }

  /* if interval is 0, no more notifications should be sent */
  if (interval_to_use == 0.0)
    hst->no_more_notifications = true;
  else
    hst->no_more_notifications = false;

  logger(dbg_notifications, most)
    << "Interval used for calculating next valid notification time: "
    << interval_to_use;

  /* calculate next notification time */
  next_notification = static_cast<time_t>(offset
                                          + (interval_to_use * config->interval_length()));

  return (next_notification);
}

/******************************************************************/
/***************** NOTIFICATION OBJECT FUNCTIONS ******************/
/******************************************************************/

/* given a contact name, find the notification entry for them for the list in memory */
notification* find_notification(contact* cntct) {
  logger(dbg_functions, basic)
    << "find_notification()";

  if (cntct == NULL)
    return (NULL);

  for (notification* temp_notification = notification_list;
       temp_notification != NULL;
       temp_notification = temp_notification->next) {
    if (temp_notification->cntct == cntct)
      return (temp_notification);
  }

  /* we couldn't find the contact in the notification list */
  return (NULL);
}

/* add a new notification to the list in memory */
int add_notification(nagios_macros* mac, contact* cntct) {
  logger(dbg_functions, basic)
    << "add_notification()";

  if (cntct == NULL)
    return (ERROR);

  logger(dbg_notifications, most)
    << "Adding contact '" << cntct->name << "' to notification list.";

  /* don't add anything if this contact is already on the notification list */
  if (find_notification(cntct))
    return (OK);

  /* allocate memory for a new contact in the notification list */
  notification* new_notification(new notification);

  /* fill in the contact info */
  new_notification->cntct = cntct;

  /* add new notification to head of list */
  new_notification->next = notification_list;
  notification_list = new_notification;

  /* add contact to notification recipients macro */
  if (!mac->x[MACRO_NOTIFICATIONRECIPIENTS])
    string::setstr(mac->x[MACRO_NOTIFICATIONRECIPIENTS], cntct->name);
  else {
    std::string buffer(mac->x[MACRO_NOTIFICATIONRECIPIENTS]);
    buffer += ",";
    buffer += cntct->name;
    string::setstr(mac->x[MACRO_NOTIFICATIONRECIPIENTS], buffer);
  }

  return (OK);
}

#endif
