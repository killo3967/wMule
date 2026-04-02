//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Alc Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Pixmaps from http://jimmac.musichall.cz/ikony.php3 | http://www.everaldo.com | http://www.icomania.com
///
/// This program is free software; you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "config.h"		// Needed for PACKAGE
#include "alc.h"

#include <wx/log.h>

namespace {
	bool LoadAlcCatalog(wxLocale& locale)
	{
		const wxChar* const primary = wxT("wmule");
		const wxChar* const legacy = wxT("amule");

		if (locale.AddCatalog(primary)) {
			wxLogDebug(wxT("[i18n] Loaded locale catalog '%s'"), primary);
			return true;
		}

		wxLogDebug(wxT("[i18n] Locale catalog '%s' not found, trying legacy domain"), primary);
		if (wxStrcmp(primary, legacy) != 0 && locale.AddCatalog(legacy)) {
			wxLogDebug(wxT("[i18n] Loaded locale catalog '%s' (fallback)"), legacy);
			return true;
		}

		wxLogWarning(wxT("[i18n] Unable to load translation catalog for alc."));
		return false;
	}
}

// Application implementation
IMPLEMENT_APP (alc)

bool alc::OnInit ()
{
  // Used to tell alc to use aMule catalog
  m_locale.Init();
  LoadAlcCatalog(m_locale);

	m_alcFrame = new AlcFrame (_("aLinkCreator, the wMule eD2k link creator"));
  m_alcFrame->Show (true);
  SetTopWindow (m_alcFrame);
  return true;
}

AlcFrame *alc::GetMainFrame()
{
  return (m_alcFrame);
}
// File_checked_for_headers
