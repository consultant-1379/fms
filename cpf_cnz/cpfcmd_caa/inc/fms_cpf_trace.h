/*
 * fms_cpf_trace.h
 *
 *  Created on: Jul 22, 2011
 *      Author: enungai
 */

#ifndef FMS_CPF_TRACE_H_
#define FMS_CPF_TRACE_H_
#include <ace/ACE.h>

class ACS_TRA_trace;

//#define FMS_CPF_LOG_APPENDER_NAME  "FMS_CPFD"
//#define FMS_CPF_DAEMON_NAME  "fms_cpfd"

// To avoid warning about unused parameter
#define UNUSED(expr) do { (void)(expr); } while (0)

#define TRACE(TRACE_CLASS, FMT, ...) \
		trautil_trace(TRACE_CLASS, FMT, __VA_ARGS__);

typedef enum {
	FMS_CPF_SUCCESS = 0,
	FMS_CPF_FAILURE = -1
}FMS_CPF_ReturnType;

void trautil_trace(ACS_TRA_trace* trace_class, const ACE_TCHAR* messageFormat, ...);

#endif /* FMS_CPF_TRACE_H_ */
