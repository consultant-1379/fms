/*
 * fms_cpf_trace.cpp
 *
 *  Created on: Jul 22, 2011
 *      Author: enungai
 */


#include "fms_cpf_trace.h"
#include "ACS_TRA_trace.h"

/*============================================================================
	ROUTINE: trautil_trace
 ============================================================================ */
void trautil_trace(ACS_TRA_trace* trace_class, const ACE_TCHAR* messageFormat, ...)
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
