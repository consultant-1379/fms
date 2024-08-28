/*
 * fms_cpf_api_trace.cpp
 *
 *  Created on: August 17, 2011
 *      Author: enungai
 */


#include "fms_cpf_api_trace.h"
#include "ACS_TRA_trace.h"

#include <ace/ACE.h>

/*============================================================================
	ROUTINE: trautil_trace
 ============================================================================ */
void trautil_trace(ACS_TRA_trace* trace_class, const char* messageFormat, ...)
{
	const ACE_UINT32 TRACE_BUF_SIZE = 1024;

	if( (NULL != trace_class) && (trace_class->isOn()) )
	{
		if( messageFormat && *messageFormat )
		{
			va_list params;
			va_start(params, messageFormat);
			ACE_TCHAR traceBuffer[TRACE_BUF_SIZE]={'\0'};
			ACE_OS::vsnprintf(traceBuffer, TRACE_BUF_SIZE - 1, messageFormat, params);
			trace_class->trace(traceBuffer);
			va_end(params);
		}
	}
}
