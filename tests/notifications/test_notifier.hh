/*
** Copyright 2017 Centreon
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** For more information : contact@centreon.com
*/

#include "com/centreon/engine/notifications/notifier.hh"

using namespace com::centreon::engine;
using namespace com::centreon::engine::notifications;

class test_notifier : public notifier {
 public:
       test_notifier();
  void set_in_downtime(bool downtime);
  void set_is_host(bool is_host);
  void set_notification_interval(long interval);
  void set_current_notification_number(int number);
  void set_current_notification_type(notifier::notification_type type);

 protected:
  bool _is_host() const;
  void _checkable_macro_builder(nagios_macros& mac);

  bool __is_host;
};
