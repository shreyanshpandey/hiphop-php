/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010- Facebook, Inc. (http://www.facebook.com)         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include <runtime/base/time/dateinterval.h>
#include <runtime/base/complex_types.h>
#include <runtime/base/time/datetime.h>
#include <runtime/base/execution_context.h>
#include <runtime/base/type_conversions.h>
#include <runtime/base/builtin_functions.h>
#include <runtime/base/runtime_error.h>
#include <util/logger.h>

namespace HPHP {

IMPLEMENT_OBJECT_ALLOCATION(DateInterval)
///////////////////////////////////////////////////////////////////////////////

StaticString DateInterval::s_class_name("DateInterval");

///////////////////////////////////////////////////////////////////////////////

DateInterval::DateInterval() {
  m_di = DateIntervalPtr();
}

DateInterval::DateInterval(CStrRef date_interval,
                           bool date_string /*= false */) {
  if (date_string) {
    setDateString(date_interval);
  } else {
    setInterval(date_interval);
  }
}

DateInterval::DateInterval(timelib_rel_time *di) {
  m_di = DateIntervalPtr(di, dateinterval_deleter());
}

bool DateInterval::setDateString(CStrRef date_string) {
  timelib_time *time;
  timelib_rel_time *di;
  timelib_error_container *errors = NULL;

  time = timelib_strtotime((char*)date_string.data(), date_string.size(),
                           &errors, TimeZone::GetDatabase());
  int error_count = errors->error_count;
  DateTime::setLastErrors(errors);

  if (error_count > 0) {
    timelib_time_dtor(time);
    return false;
  } else {
    di = timelib_rel_time_clone(&(time->relative));
    timelib_time_dtor(time);
    m_di = DateIntervalPtr(di, dateinterval_deleter());
    return true;
  }
}

bool DateInterval::setInterval(CStrRef date_interval) {
  timelib_rel_time *di = NULL;
  timelib_error_container *errors = NULL;

#ifdef TIMELIB_HAVE_INTERVAL
  timelib_time *start = NULL, *end = NULL;
  int r = 0;

  timelib_strtointerval((char*)date_interval.data(), date_interval.size(),
                        &start, &end, &di, &r, &errors);
#else
  throw NotImplementedException("timelib too old");
#endif

  int error_count  = errors->error_count;
  DateTime::setLastErrors(errors);
  if (error_count > 0) {
    timelib_rel_time_dtor(di);
    return false;
  } else {
    m_di = DateIntervalPtr(di, dateinterval_deleter());
    return true;
  }
}

String DateInterval::format(CStrRef format_spec) {
  StringBuffer s;
  for(int i = 0; i < format_spec.length(); i++) {
    const int MAXLEN = 22; // 64bit signed int string length, plus terminating \0
    char buf[MAXLEN];
    int l;
    char c = format_spec.charAt(i);

    if (c != '%') {
      s.append(c);
      continue;
    }
    i++;
    if (i == format_spec.length()) {
      // End of format, use literal % and finish
      s.append(c);
      break;
    }
    c = format_spec.charAt(i);

    switch(c) {
      case 'Y': l = snprintf(buf, MAXLEN, "%02"PRId64, getYears()); break;
      case 'y': l = snprintf(buf, MAXLEN, "%"PRId64,   getYears()); break;

      case 'M': l = snprintf(buf, MAXLEN, "%02"PRId64, getMonths()); break;
      case 'm': l = snprintf(buf, MAXLEN, "%"PRId64,   getMonths()); break;

      case 'D': l = snprintf(buf, MAXLEN, "%02"PRId64, getDays()); break;
      case 'd': l = snprintf(buf, MAXLEN, "%"PRId64,   getDays()); break;

      case 'H': l = snprintf(buf, MAXLEN, "%02"PRId64, getHours()); break;
      case 'h': l = snprintf(buf, MAXLEN, "%"PRId64,   getHours()); break;

      case 'I': l = snprintf(buf, MAXLEN, "%02"PRId64, getMinutes()); break;
      case 'i': l = snprintf(buf, MAXLEN, "%"PRId64,   getMinutes()); break;

      case 'S': l = snprintf(buf, MAXLEN, "%02"PRId64, getSeconds()); break;
      case 's': l = snprintf(buf, MAXLEN, "%"PRId64,   getSeconds()); break;

      case 'a':
        if (haveTotalDays()) {
          l = snprintf(buf, MAXLEN, "%"PRId64, getTotalDays());
        } else {
          l = snprintf(buf, MAXLEN, "(unknown)");
        }
        break;

      case 'R':
        l = snprintf(buf, MAXLEN, "%c", isInverted() ? '-' : '+'); break;
      case 'r':
        l = snprintf(buf, MAXLEN, "%s", isInverted() ? "-" : "");  break;

      case '%':
      default:
        l = 0;
        s.append('%');
        break;
    }

    if (l > 0) {
      s.append(buf, l);
    }
  }
  return s.detach();
}

SmartObject<DateInterval> DateInterval::cloneDateInterval() const {
  if (!m_di) return NEWOBJ(DateInterval)();
  return NEWOBJ(DateInterval)(timelib_rel_time_clone(m_di.get()));
}

///////////////////////////////////////////////////////////////////////////////
}