//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef TYPES_H
#define TYPES_H

#ifndef USE_STD_STRING
#include <wx/string.h>		// Needed for wxString and wxEmptyString
#endif

#include <list>			// Needed for std::list
#include <vector>		// Needed for std::vector

#ifndef _MSC_VER
	#ifndef __STDC_FORMAT_MACROS
		#define __STDC_FORMAT_MACROS
	#endif
	#include <inttypes.h>
	#define LONGLONG(x) x##ll
	#define ULONGLONG(x) x##llu
#else
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
	typedef signed __int8 int8_t;
	typedef signed __int16 int16_t;
	typedef signed __int32 int32_t;
	typedef signed __int64 int64_t;
	#define LONGLONG(x) x##i64
	#define ULONGLONG(x) x##ui64
#endif

// These are _MSC_VER defines used in eMule. They should
// not be used in aMule, instead, use this table to
// find the type to use in order to get the desired
// effect.
//////////////////////////////////////////////////
// Name              // Type To Use In Amule    //
//////////////////////////////////////////////////
// BOOL              // bool                    //
// WORD              // uint16                  //
// INT               // int32                   //
// UINT              // uint32                  //
// UINT_PTR          // uint32*                 //
// PUINT             // uint32*                 //
// DWORD             // uint32                  //
// LONG              // long                    //
// ULONG             // unsigned long           //
// LONGLONG          // long long               //
// ULONGLONG         // unsigned long long      //
// LPBYTE            // char*                   //
// VOID              // void                    //
// PVOID             // void*                   //
// LPVOID            // void*                   //
// LPCVOID           // const void*             //
// CHAR              // char                    //
// LPSTR             // char*                   //
// LPCSTR            // const char*             //
// TCHAR             // char                    //
// LPTSTR            // char*                   //
// LPCTSTR           // const char*             //
// WCHAR             // wchar_t                 //
// LPWSTR            // wchar_t*                //
// LPCWSTR           // const wchar_t*          //
// WPARAM            // uint16                  //
// LPARAM            // uint32                  //
// POINT             // wxPoint                 //
//////////////////////////////////////////////////

/*
 * Backwards compatibility with emule.
 * Note that the int* types are indeed unsigned.
 */

// C++11 type aliases for modern code
using int8  = uint8_t;
using uint8  = uint8_t;
using int16  = uint16_t;
using uint16 = uint16_t;
using int32  = uint32_t;
using uint32 = uint32_t;
using int64  = uint64_t;
using uint64 = uint64_t;
using sint8  = int8_t;
using sint16 = int16_t;
using sint32 = int32_t;
using sint64 = int64_t;

// Compile-time type size verification
static_assert(sizeof(uint8)  == 1, "uint8 must be 1 byte");
static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes");
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes");
static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes");
static_assert(sizeof(sint8)  == 1, "sint8 must be 1 byte");
static_assert(sizeof(sint16) == 2, "sint16 must be 2 bytes");
static_assert(sizeof(sint32) == 4, "sint32 must be 4 bytes");
static_assert(sizeof(sint64) == 8, "sint64 must be 8 bytes");

// Legacy typedefs for backward compatibility (deprecated, prefer using aliases above)
typedef uint8_t  int8;
typedef uint8_t  uint8;
typedef uint16_t int16;
typedef uint16_t uint16;
typedef uint32_t int32;
typedef uint32_t uint32;
typedef uint64_t int64;
typedef uint64_t uint64;
typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;


class CKnownFile;

//! Various common list-types.
//@{
#ifndef USE_STD_STRING
typedef std::list<wxString> CStringList;
#endif
typedef std::list<CKnownFile*> CKnownFilePtrList;
//@}

// C++11 type aliases for container types
using ArrayOfUInts8  = std::vector<uint8>;
using ArrayOfUInts16 = std::vector<uint16>;
using ArrayOfUInts32 = std::vector<uint32>;
using ArrayOfUInts64 = std::vector<uint64>;
using ListOfUInts32  = std::list<uint32>;

// Legacy typedefs for backward compatibility
typedef std::vector<uint8>  ArrayOfUInts8;
typedef std::vector<uint16> ArrayOfUInts16;
typedef std::vector<uint32> ArrayOfUInts32;
typedef std::vector<uint64> ArrayOfUInts64;
typedef std::list<uint32>   ListOfUInts32;

/* This is the Evil Void String For Returning On Const References From Hell */
// IT MEANS I WANT TO USE IT EVERYWHERE. DO NOT MOVE IT.
// THE FACT SOMETHING IS USED IN JUST ONE PLACE DOESN'T MEAN IT HAS
// TO BE MOVED TO THAT PLACE. I MIGHT NEED IT ELSEWHERE LATER.
//

#ifndef USE_STD_STRING
static const wxString EmptyString = wxEmptyString;
#endif

#ifndef __cplusplus
	typedef int bool;
#endif


#ifdef _WIN32			// Used in non-wx-apps too (ed2k), so don't use __WINDOWS__ here !
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	// Include winsock2.h BEFORE windows.h to set _WINSOCKAPI_ and prevent
	// conflicts when windows.h includes winsock.h. winsock2.h sets _WINSOCKAPI_
	// which tells winsock.h to skip its content (avoiding sockaddr redefinition).
	#include <winsock2.h>
	#include <windows.h> // Needed for RECT  // Do_not_auto_remove
	// Windows compilers don't have these constants
	#ifndef W_OK
		enum
		{
			F_OK = 0,   // test for existence
			X_OK = 1,   //          execute permission
			W_OK = 2,   //          write
			R_OK = 4    //          read
		};
	#endif // W_OK
	#ifdef __WINDOWS__
		#include <wx/msw/winundef.h>	// Do_not_auto_remove
	#endif
	#undef GetUserName
#else // _WIN32
	typedef struct sRECT {
	  uint32 left;
	  uint32 top;
	  uint32 right;
	  uint32 bottom;
	} RECT;
#endif /* _WIN32 */


#endif /* TYPES_H */
// File_checked_for_headers
