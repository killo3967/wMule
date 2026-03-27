//
// Helper utilities for storing and verifying secrets using PBKDF2.
//

#ifndef W_MULE_SECRETHASH_H
#define W_MULE_SECRETHASH_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "../../Types.h"

namespace SecretHash {

static constexpr const wchar_t* PBKDF2_PREFIX = L"pbkdf2-sha256";
static constexpr unsigned int PBKDF2_DEFAULT_ITERATIONS = 150000;

struct PBKDF2Info {
	unsigned int iterations = 0;
	wxString saltHex;
	wxString digestHex;
};

// Returns true if the given value looks like a PBKDF2 secret string.
bool IsPBKDF2Secret(const wxString& value);

// Returns true if the value looks like the legacy MD5 hash (32 hex chars).
bool IsLegacyMD5(const wxString& value);

// Parses a PBKDF2 secret string into its components.
bool ExtractPBKDF2Info(const wxString& value, PBKDF2Info& outInfo);

// Generates a PBKDF2 secret string from an MD5 hash (hex string).
wxString BuildPBKDF2FromMD5(const wxString& md5Hex, unsigned int iterations = PBKDF2_DEFAULT_ITERATIONS);

// Generates a PBKDF2 secret using provided salt (hex) and iterations.
wxString BuildPBKDF2FromMD5WithSalt(const wxString& md5Hex, unsigned int iterations, const wxString& saltHex);

// Derives the digest hex for a given MD5 hash using the supplied PBKDF2 parameters.
bool DeriveDigestFromMD5(const wxString& md5Hex, unsigned int iterations, const wxString& saltHex, wxString& outDigestHex);

// Normalizes a stored secret: migrates legacy hashes to PBKDF2 if needed.
wxString NormalizeStoredSecret(const wxString& stored, bool& migrated);

} // namespace SecretHash

#endif // W_MULE_SECRETHASH_H
