/*
** Copyright 1999-2010 Ethan Galstad
** Copyright 2011      Merethis
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

#ifndef CCE_MACROS_HH
# define CCE_MACROS_HH

# include "objects.hh"

# ifdef __cplusplus
extern "C" {
# endif

// Length Limitations
# define MAX_COMMAND_ARGUMENTS                  32   // maximum number of $ARGx$ macros

// Macro Definitions
# define MACRO_ENV_VAR_PREFIX                   "NAGIOS_"
# define MAX_USER_MACROS                        256  // maximum number of $USERx$ macros
# define MACRO_X_COUNT                          153  // size of macro_x[] array

# define MACRO_HOSTNAME                         0
# define MACRO_HOSTALIAS                        1
# define MACRO_HOSTADDRESS                      2
# define MACRO_SERVICEDESC                      3
# define MACRO_SERVICESTATE                     4
# define MACRO_SERVICESTATEID                   5
# define MACRO_SERVICEATTEMPT                   6
# define MACRO_LONGDATETIME                     7
# define MACRO_SHORTDATETIME                    8
# define MACRO_DATE                             9
# define MACRO_TIME                             10
# define MACRO_TIMET                            11
# define MACRO_LASTHOSTCHECK                    12
# define MACRO_LASTSERVICECHECK                 13
# define MACRO_LASTHOSTSTATECHANGE              14
# define MACRO_LASTSERVICESTATECHANGE           15
# define MACRO_HOSTOUTPUT                       16
# define MACRO_SERVICEOUTPUT                    17
# define MACRO_HOSTPERFDATA                     18
# define MACRO_SERVICEPERFDATA                  19
# define MACRO_CONTACTNAME                      20
# define MACRO_CONTACTALIAS                     21
# define MACRO_CONTACTEMAIL                     22
# define MACRO_CONTACTPAGER                     23
# define MACRO_ADMINEMAIL                       24
# define MACRO_ADMINPAGER                       25
# define MACRO_HOSTSTATE                        26
# define MACRO_HOSTSTATEID                      27
# define MACRO_HOSTATTEMPT                      28
# define MACRO_NOTIFICATIONTYPE                 29
# define MACRO_NOTIFICATIONNUMBER               30  // deprecated - see HOSTNOTIFICATIONNUMBER and SERVICENOTIFICATIONNUMBER macros
# define MACRO_HOSTEXECUTIONTIME                31
# define MACRO_SERVICEEXECUTIONTIME             32
# define MACRO_HOSTLATENCY                      33
# define MACRO_SERVICELATENCY                   34
# define MACRO_HOSTDURATION                     35
# define MACRO_SERVICEDURATION                  36
# define MACRO_HOSTDURATIONSEC                  37
# define MACRO_SERVICEDURATIONSEC               38
# define MACRO_HOSTDOWNTIME                     39
# define MACRO_SERVICEDOWNTIME                  40
# define MACRO_HOSTSTATETYPE                    41
# define MACRO_SERVICESTATETYPE                 42
# define MACRO_HOSTPERCENTCHANGE                43
# define MACRO_SERVICEPERCENTCHANGE             44
# define MACRO_HOSTGROUPNAME                    45
# define MACRO_HOSTGROUPALIAS                   46
# define MACRO_SERVICEGROUPNAME                 47
# define MACRO_SERVICEGROUPALIAS                48
# define MACRO_HOSTACKAUTHOR                    49
# define MACRO_HOSTACKCOMMENT                   50
# define MACRO_SERVICEACKAUTHOR                 51
# define MACRO_SERVICEACKCOMMENT                52
# define MACRO_LASTSERVICEOK                    53
# define MACRO_LASTSERVICEWARNING               54
# define MACRO_LASTSERVICEUNKNOWN               55
# define MACRO_LASTSERVICECRITICAL              56
# define MACRO_LASTHOSTUP                       57
# define MACRO_LASTHOSTDOWN                     58
# define MACRO_LASTHOSTUNREACHABLE              59
# define MACRO_SERVICECHECKCOMMAND              60
# define MACRO_HOSTCHECKCOMMAND                 61
# define MACRO_MAINCONFIGFILE                   62
# define MACRO_STATUSDATAFILE                   63
# define MACRO_HOSTDISPLAYNAME                  64
# define MACRO_SERVICEDISPLAYNAME               65
# define MACRO_RETENTIONDATAFILE                66
# define MACRO_OBJECTCACHEFILE                  67
# define MACRO_TEMPFILE                         68
# define MACRO_LOGFILE                          69
# define MACRO_RESOURCEFILE                     70
# define MACRO_COMMANDFILE                      71
# define MACRO_HOSTPERFDATAFILE                 72
# define MACRO_SERVICEPERFDATAFILE              73
# define MACRO_HOSTACTIONURL                    74
# define MACRO_HOSTNOTESURL                     75
# define MACRO_HOSTNOTES                        76
# define MACRO_SERVICEACTIONURL                 77
# define MACRO_SERVICENOTESURL                  78
# define MACRO_SERVICENOTES                     79
# define MACRO_TOTALHOSTSUP                     80
# define MACRO_TOTALHOSTSDOWN                   81
# define MACRO_TOTALHOSTSUNREACHABLE            82
# define MACRO_TOTALHOSTSDOWNUNHANDLED          83
# define MACRO_TOTALHOSTSUNREACHABLEUNHANDLED   84
# define MACRO_TOTALHOSTPROBLEMS                85
# define MACRO_TOTALHOSTPROBLEMSUNHANDLED       86
# define MACRO_TOTALSERVICESOK                  87
# define MACRO_TOTALSERVICESWARNING             88
# define MACRO_TOTALSERVICESCRITICAL            89
# define MACRO_TOTALSERVICESUNKNOWN             90
# define MACRO_TOTALSERVICESWARNINGUNHANDLED    91
# define MACRO_TOTALSERVICESCRITICALUNHANDLED   92
# define MACRO_TOTALSERVICESUNKNOWNUNHANDLED    93
# define MACRO_TOTALSERVICEPROBLEMS             94
# define MACRO_TOTALSERVICEPROBLEMSUNHANDLED    95
# define MACRO_PROCESSSTARTTIME                 96
# define MACRO_HOSTCHECKTYPE                    97
# define MACRO_SERVICECHECKTYPE                 98
# define MACRO_LONGHOSTOUTPUT                   99
# define MACRO_LONGSERVICEOUTPUT                100
# define MACRO_TEMPPATH                         101
# define MACRO_HOSTNOTIFICATIONNUMBER           102
# define MACRO_SERVICENOTIFICATIONNUMBER        103
# define MACRO_HOSTNOTIFICATIONID               104
# define MACRO_SERVICENOTIFICATIONID            105
# define MACRO_HOSTEVENTID                      106
# define MACRO_LASTHOSTEVENTID                  107
# define MACRO_SERVICEEVENTID                   108
# define MACRO_LASTSERVICEEVENTID               109
# define MACRO_HOSTGROUPNAMES                   110
# define MACRO_SERVICEGROUPNAMES                111
# define MACRO_HOSTACKAUTHORNAME                112
# define MACRO_HOSTACKAUTHORALIAS               113
# define MACRO_SERVICEACKAUTHORNAME             114
# define MACRO_SERVICEACKAUTHORALIAS            115
# define MACRO_MAXHOSTATTEMPTS                  116
# define MACRO_MAXSERVICEATTEMPTS               117
# define MACRO_SERVICEISVOLATILE                118
# define MACRO_TOTALHOSTSERVICES                119
# define MACRO_TOTALHOSTSERVICESOK              120
# define MACRO_TOTALHOSTSERVICESWARNING         121
# define MACRO_TOTALHOSTSERVICESUNKNOWN         122
# define MACRO_TOTALHOSTSERVICESCRITICAL        123
# define MACRO_HOSTGROUPNOTES                   124
# define MACRO_HOSTGROUPNOTESURL                125
# define MACRO_HOSTGROUPACTIONURL               126
# define MACRO_SERVICEGROUPNOTES                127
# define MACRO_SERVICEGROUPNOTESURL             128
# define MACRO_SERVICEGROUPACTIONURL            129
# define MACRO_HOSTGROUPMEMBERS                 130
# define MACRO_SERVICEGROUPMEMBERS              131
# define MACRO_CONTACTGROUPNAME                 132
# define MACRO_CONTACTGROUPALIAS                133
# define MACRO_CONTACTGROUPMEMBERS              134
# define MACRO_CONTACTGROUPNAMES                135
# define MACRO_NOTIFICATIONRECIPIENTS           136
# define MACRO_NOTIFICATIONISESCALATED          137
# define MACRO_NOTIFICATIONAUTHOR               138
# define MACRO_NOTIFICATIONAUTHORNAME           139
# define MACRO_NOTIFICATIONAUTHORALIAS          140
# define MACRO_NOTIFICATIONCOMMENT              141
# define MACRO_EVENTSTARTTIME                   142
# define MACRO_HOSTPROBLEMID                    143
# define MACRO_LASTHOSTPROBLEMID                144
# define MACRO_SERVICEPROBLEMID                 145
# define MACRO_LASTSERVICEPROBLEMID             146
# define MACRO_ISVALIDTIME                      147
# define MACRO_NEXTVALIDTIME                    148
# define MACRO_LASTHOSTSTATE                    149
# define MACRO_LASTHOSTSTATEID                  150
# define MACRO_LASTSERVICESTATE                 151
# define MACRO_LASTSERVICESTATEID               152

// Macro Cleaning Options
# define STRIP_ILLEGAL_MACRO_CHARS              1
# define ESCAPE_MACRO_CHARS                     2
# define URL_ENCODE_MACRO_CHARS                 4

// NAGIOS_MACROS structure
struct                   nagios_macros {
  char*                  x[MACRO_X_COUNT];
  char*                  argv[MAX_COMMAND_ARGUMENTS];
  char*                  contactaddress[MAX_CONTACT_ADDRESSES];
  char*                  ondemand;
  host*                  host_ptr;
  hostgroup*             hostgroup_ptr;
  service*               service_ptr;
  servicegroup*          servicegroup_ptr;
  contact*               contact_ptr;
  contactgroup*          contactgroup_ptr;
  customvariablesmember* custom_host_vars;
  customvariablesmember* custom_service_vars;
  customvariablesmember* custom_contact_vars;
};

typedef struct nagios_macros nagios_macros;

// Macro Functions
nagios_macros* get_global_macros(void);

// thread-safe version of process_macros.
int process_macros_r(nagios_macros* mac, char const* input_buffer, char** output_buffer, int options);

/*
 * Replace macros with their actual values
 * This function modifies the global_macros struct and is thus
 * not thread-safe.
 */
int process_macros(char const* input_buffer, char** output_buffer, int options);

/*
 * These functions updates **macros with the values from
 * their respective object type.
 */
int grab_host_macros(host* hst);
int grab_service_macros(service* svc);
int grab_hostgroup_macros(hostgroup* hg);
int grab_servicegroup_macros(servicegroup* sg);
int grab_contact_macros(contact* cntct);

int grab_macro_value(char* macro_buffer,
      char** output,
      int* clean_options,
      int* free_macro);
int grab_macrox_value(int macro_type,
      char const* arg1,
      char const* arg2,
      char** output,
      int* free_macro);
int grab_custom_macro_value(char* macro_name,
      char const* arg1,
      char const* arg2,
      char** output);
int grab_datetime_macro(int macro_type,
      char const* arg1,
      char const* arg2,
      char** output);
int grab_standard_host_macro(int macro_type,
      host* temp_host,
      char** output,
      int* free_macro);
int grab_standard_hostgroup_macro(int macro_type,
      hostgroup* temp_hostgroup,
      char** output);
int grab_standard_service_macro(int macro_type,
      service* temp_service,
      char** output,
      int* free_macro);
int grab_standard_servicegroup_macro(int macro_type,
      servicegroup* temp_servicegroup,
      char** output);
int grab_standard_contact_macro(int macro_type,
      contact* temp_contact,
      char** output);
int grab_contact_address_macro(unsigned int macro_num,
      contact* temp_contact,
      char** output);
int grab_standard_contactgroup_macro(int macro_type,
      contactgroup* temp_contactgroup,
      char** output);
int grab_custom_object_macro(char* macro_name,
      customvariablesmember* vars,
      char** output);

/* thread-safe version of the above */
int grab_host_macros_r(nagios_macros* mac,
      host* hst);
int grab_service_macros_r(nagios_macros* mac,
      service* svc);
int grab_hostgroup_macros_r(nagios_macros* mac,
      hostgroup* hg);
int grab_servicegroup_macros_r(nagios_macros* mac,
      servicegroup* sg);
int grab_contact_macros_r(nagios_macros* mac,
      contact* cntct);

int grab_macro_value_r(nagios_macros* mac,
      char* macro_buffer,
      char** output,
      int* clean_options,
      int* free_macro);
int grab_macrox_value_r(nagios_macros* mac,
      int macro_type,
      char const* arg1,
      char const* arg2,
      char** output,
      int* free_macro);
int grab_custom_macro_value_r(nagios_macros* mac,
      char* macro_name,
      char const* arg1,
      char const* arg2,
      char** output);
int grab_datetime_macro_r(nagios_macros* mac,
      int macro_type,
      char const* arg1,
      char const* arg2,
      char** output);
int grab_standard_host_macro_r(nagios_macros* mac,
      int macro_type,
      host* temp_host,
      char** output,
      int* free_macro);
int grab_standard_hostgroup_macro_r(nagios_macros* mac,
      int macro_type,
      hostgroup* temp_hostgroup,
      char** output);
int grab_standard_service_macro_r(nagios_macros* mac,
      int macro_type,
      service* temp_service,
      char** output,
      int* free_macro);
int grab_standard_servicegroup_macro_r(nagios_macros* mac,
      int macro_type,
      servicegroup* temp_servicegroup,
      char** output);
int grab_standard_contact_macro_r(nagios_macros* mac,
      int macro_type,
      contact* temp_contact,
      char** output);
int grab_custom_object_macro_r(nagios_macros* mac,
      char* macro_name,
      customvariablesmember* vars,
      char** output);

char const* clean_macro_chars(char* macro,int options); // cleans macros characters before insertion into output string

char* get_url_encoded_string(char* input);              // URL encode a string

int init_macros(void);
int init_macrox_names(void);
int free_macrox_names(void);

void copy_constant_macros(char** dest);

/* clear macros */
int clear_argv_macros();
int clear_volatile_macros();
int clear_host_macros();
int clear_service_macros();
int clear_hostgroup_macros();
int clear_servicegroup_macros();
int clear_contact_macros();
int clear_contactgroup_macros();
int clear_summary_macros();

/* thread-safe version of the above */
int clear_argv_macros_r(nagios_macros* mac);
int clear_volatile_macros_r(nagios_macros* mac);
int clear_host_macros_r(nagios_macros* mac);
int clear_service_macros_r(nagios_macros* mac);
int clear_hostgroup_macros_r(nagios_macros* mac);
int clear_servicegroup_macros_r(nagios_macros* mac);
int clear_contact_macros_r(nagios_macros* mac);
int clear_contactgroup_macros_r(nagios_macros* mac);
int clear_summary_macros_r(nagios_macros* mac);

int set_all_macro_environment_vars(int set);
int set_macrox_environment_vars(int set);
int set_argv_macro_environment_vars(int set);
int set_custom_macro_environment_vars(int set);
int set_contact_address_environment_vars(int set);
int set_macro_environment_var(char const* name,
      char const* value,
      int set);

/* thread-safe version of the above */
int set_all_macro_environment_vars_r(nagios_macros* mac, int set);
int set_macrox_environment_vars_r(nagios_macros* mac, int set);
int set_argv_macro_environment_vars_r(nagios_macros* mac, int set);
int set_custom_macro_environment_vars_r(nagios_macros* mac, int set);
int set_contact_address_environment_vars_r(nagios_macros* mac, int set);

# ifdef __cplusplus
}
# endif

#endif // !CCE_MACROS_HH