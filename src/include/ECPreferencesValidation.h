//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef ECPREFERENCESVALIDATION_H
#define ECPREFERENCESVALIDATION_H

#include <wx/string.h>

#include <common/Path.h>

/**
 * Outcome of applying a directory update coming from EC preferences.
 */
struct ECDirectoryUpdateOutcome
{
	bool m_ok;
	wxString m_error;
	wxString m_effectivePath;
	bool m_usedUnsafeBypass;
};

/**
 * Validates an incoming EC directory value against the internal path guards.
 *
 * The previous value is preserved on error. The provided context may be updated
 * when validation succeeds so subsequent validations can rely on the refreshed
 * base (e.g. OSDir depends on IncomingDir).
 */
inline ECDirectoryUpdateOutcome ApplyECPathPreference(EInternalPathKind kind,
	const wxString& candidate, const CPath& previous, CInternalPathContext& context)
{
	CInternalPathResult result = NormalizeInternalDir(kind, candidate, context);
	ECDirectoryUpdateOutcome outcome = {false, wxEmptyString, previous.GetRaw(), false};
	if (!result.m_ok) {
		outcome.m_error = InternalPathErrorToString(result.m_error);
		return outcome;
	}

	outcome.m_ok = true;
	outcome.m_error = wxEmptyString;
	outcome.m_effectivePath = result.m_normalizedPath;
	outcome.m_usedUnsafeBypass = result.m_isExternalToBase;

	switch (kind) {
		case EInternalPathKind::IncomingDir:
			context.m_incomingDir = result.m_normalizedPath;
			break;
		case EInternalPathKind::TempDir:
			context.m_tempDir = result.m_normalizedPath;
			break;
		default:
			break;
	}

	return outcome;
}

#endif /* ECPREFERENCESVALIDATION_H */
