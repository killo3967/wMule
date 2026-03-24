//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <cstdlib>			// Needed for std::abort()

#include "config.h"			// Needed for HAVE_CXXABI and HAVE_EXECINFO

#include "MuleDebug.h"			// Interface declaration
#include "StringFunctions.h"		// Needed for unicode2char
#include "Format.h"			// Needed for CFormat

#ifdef HAVE_EXECINFO
#	include <execinfo.h>
#	include <wx/utils.h>		// Needed for wxArrayString
#	ifndef HAVE_BFD
#		include <wx/thread.h>	// Needed for wxThread
#	endif
#endif

#ifdef HAVE_CXXABI
#	ifdef HAVE_TYPEINFO
#		include <typeinfo>	// Needed for some MacOSX versions with broken system headers
#	endif
#	include <cxxabi.h>
#endif


#if wxUSE_STACKWALKER && defined(__WINDOWS__)
	#include <wx/stackwalk.h> // Do_not_auto_remove
#elif defined(HAVE_BFD)
	#include <ansidecl.h> // Do_not_auto_remove
	#include <bfd.h> // Do_not_auto_remove
#endif

#include <vector>
#include <exception>


/**
 * This functions displays a verbose description of
 * any unhandled exceptions that occur and then
 * terminate the program by raising SIGABRT.
 */
void OnUnhandledException()
{
	// Revert to the original exception handler, to avoid
	// infinate recursion, in case something goes wrong in
	// this function.
	std::set_terminate(std::abort);

#ifdef HAVE_CXXABI
	std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
	FILE* output = stderr;
#else
	FILE* output = stdout;
	bool t = true;
#endif
	if (t) {
		int status = -1;
		char *dem = 0;
#ifdef HAVE_CXXABI
		// Note that "name" is the mangled name.
		char const *name = t->name();

		dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
#else
		const char* name = "Unknown";
#endif
		fprintf(output, "\nTerminated after throwing an instance of '%s'\n", (status ? name : dem));
		free(dem);

		try {
			throw;
		} catch (const std::exception& e) {
			fprintf(output, "\twhat(): %s\n", e.what());
		} catch (const CMuleException& e) {
			fprintf(output, "\twhat(): %s\n", (const char*)unicode2char(e.what()));
		} catch (const wxString& e) {
			fprintf(output, "\twhat(): %s\n", (const char*)unicode2char(e));
		} catch (...) {
			// Unable to retrieve cause of exception
		}

		fprintf(output, "\tbacktrace:\n%s\n", (const char*)unicode2char(get_backtrace(1)));
	}
	std::abort();
}


void InstallMuleExceptionHandler()
{
	std::set_terminate(OnUnhandledException);
}


// Make it 1 for getting the file path also
#define TOO_VERBOSE_BACKTRACE 0

#if wxUSE_STACKWALKER && defined(__WINDOWS__)

// Derived class to define the actions to be done on frame print.
// I was tempted to name it MuleSkyWalker
class MuleStackWalker : public wxStackWalker
{
public:
	MuleStackWalker() {};
	~MuleStackWalker() {};

	void OnStackFrame(const wxStackFrame& frame)
	{
		wxString btLine = CFormat(wxT("[%u] ")) % frame.GetLevel();
		wxString filename = frame.GetName();

		if (!filename.IsEmpty()) {
			btLine += filename + wxT(" (") +
#if TOO_VERBOSE_BACKTRACE
			        frame.GetModule()
#else
				frame.GetModule().AfterLast(wxT('/'))
#endif
			        + wxT(")");
		} else {
			btLine += CFormat(wxT("%p")) % frame.GetAddress();
		}

		if (frame.HasSourceLocation()) {
			btLine += wxT(" at ") +
#if TOO_VERBOSE_BACKTRACE
			        frame.GetFileName()
#else
				frame.GetFileName().AfterLast(wxT('/'))
#endif
			        + CFormat(wxT(":%u")) % frame.GetLine();
		} else {
			btLine += wxT(" (Unknown file/line)");
		}

		//! Contains the entire backtrace
		m_trace += btLine + wxT("\n");
	}

	wxString m_trace;
};


wxString get_backtrace(unsigned n)
{
	MuleStackWalker walker; // Texas ranger?
	walker.Walk(n); // Skip this one and Walk() also!

	return walker.m_trace;
}

#else  // Linux/macOS code disabled - Windows x64 only build
// Platform-specific backtrace code removed - Windows x64 only

wxString get_backtrace(unsigned WXUNUSED(n))
{
	return wxT("--== BACKTRACE disabled (Windows x64 only) ==--\n\n");
}

#endif /* Platform check */

// Linux backtrace code removed - Windows x64 only build
// The get_backtrace function is defined above for Windows

// macOS code removed - Windows x64 only build
/*
#elif defined( __APPLE__ )
// According to sources, parts of this code originate at http://www.tlug.org.za/wiki/index.php/Obtaining_a_stack_trace_in_C_upon_SIGSEGV
// which doesn't exist anymore.

// Other code (stack frame list and demangle related) has been modified from code with license:

// Copyright 2007 Edd Dawson.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <dlfcn.h>
#include <cxxabi.h>
#include <sstream>

class stack_frame
{
public:
	stack_frame(const void * f_instruction, const std::string& f_function) : frame_instruction(f_instruction), frame_function(f_function) {};

	const void *instruction() const { return frame_instruction; }
	const std::string& function() const { return frame_function; }

private:
	const void * frame_instruction;
	const std::string frame_function;
};

std::string demangle(const char *name)
{
	int status = 0;
	char *d = 0;
	std::string ret = name;
	try {
		if ((d = abi::__cxa_demangle(name, 0, 0, &status))) {
			ret = d;
		}
	} catch(...) {  }

	std::free(d);
	return ret;
}


void fill_frames(std::list<stack_frame> &frames)
{
	try {
		void **fp = (void **) __builtin_frame_address (1);
		void *saved_pc = nullptr;

		// First frame is skipped
		while (fp != nullptr) {
			fp = (void**)(*fp);
			if (*fp == nullptr) {
				break;
			}

#if defined(__i386__)
			saved_pc = fp[1];
#elif defined(__ppc__)
			saved_pc = *(fp + 2);
#else
			// ?
			saved_pc = *(fp + 2);
#endif
			if (saved_pc) {
				Dl_info info;

				if (dladdr(saved_pc, &info)) {
					frames.push_back(stack_frame(saved_pc, demangle(info.dli_sname) + " in " + info.dli_fname));
				}
			}
		}
	} catch (...) {
		// Nothing to be done here, just leave.
	}
}

wxString get_backtrace(unsigned n)
{
	std::list<stack_frame> frames;
	fill_frames(frames);
	std::ostringstream backtrace;
	std::list<stack_frame>::iterator it = frames.begin();

	int count = 0;
	while (it != frames.end()) {
		if (count >= n) {
			backtrace << (*it).instruction() << " : " << (*it).function() << std::endl;
			++it;
		}

		++count;
	}

	return wxString(backtrace.str().c_str(), wxConvUTF8);
}

#endif /* Platform check */

void print_backtrace(unsigned n)
{
	wxString trace = get_backtrace(n);

	// This is because the string is ansi anyway, and the conv classes are very slow
	fprintf(stderr, "%s\n", (const char*)unicode2char(trace));
}

// File_checked_for_headers
