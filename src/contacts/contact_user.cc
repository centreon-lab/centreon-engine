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

#include "com/centreon/engine/globals.hh"
#include "com/centreon/engine/contacts/contact_user.hh"

using namespace com::centreon::engine;
using namespace com::centreon::engine::contacts;


/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 * Constructor.
 */
contact_user::contact_user() {}

/**
 * Constructor.
 */
contact_user::contact_user(std::string const& name)
  : _name(name) {}

/**
 * Copy constructor.
 *
 * @param[in] other Object to copy.
 */
contact_user::contact_user(contact_user const& other)
  : _name(other._name) {}

/**
 * Assignment operator.
 *
 * @param[in] other Object to copy.
 *
 * @return This object
 */
contact_user& contact_user::operator=(contact_user const& other) {
  _name = other._name;
  return (*this);
}

/**
 *  Destructor.
 */
contact_user::~contact_user() {
}

/**
 *  Returns a list containing this contact
 *
 *  @return std::list<individual_contact*>
 */
void contact_user::fill_contact_users(
                     std::list<shared_ptr<contact_user> >& lst) {
  lst.push_back(shared_ptr<contact_user>(new contact_user(*this)));
}

/**
 *  Returns the contact name as a reference.
 *
 *  @return a string reference.
 */
std::string const& contact_user::get_name() const {
  return _name;
}

bool contact_user::_lt(contact_generic const& other) const {
  contact_user const& other_user = static_cast<contact_user const&>(other);

  return (_name < other_user._name);
}
