/*
 * fms_cpf_api_trace.h
 *
 *  Created on: August 17, 2011
 *      Author: enungai
 */

#ifndef FMS_CPF_API_TRACE_H_
#define FMS_CPF_API_TRACE_H_

class ACS_TRA_trace;

namespace CPF_API_TRACE{
	// To avoid warning about unused parameter
	#define UNUSED(expr) do { (void)(expr); } while (0)

	#define TRACE(TRACE_CLASS, FMT, ...) \
			trautil_trace(TRACE_CLASS, FMT, __VA_ARGS__);

};
	void trautil_trace(ACS_TRA_trace* trace_class, const char* messageFormat, ...);

#endif /* FMS_CPF_ACE_TRACE_H_ */
