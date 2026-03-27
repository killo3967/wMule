//
// Implementation of PBKDF2-based secret storage helpers.
//

#include "SecretHash.h"

#include <algorithm>
#include <vector>

#include <cryptopp/osrng.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/secblock.h>

#include <wx/tokenzr.h>

#include "StringFunctions.h"

namespace SecretHash {

namespace {
	constexpr size_t kSaltBytes = 16;
	constexpr size_t kDerivedBytes = 32;

	wxString BytesToHex(const CryptoPP::byte* data, size_t len)
	{
		wxString out;
		out.Alloc(len * 2);
		for (size_t i = 0; i < len; ++i) {
			out += wxString::Format(wxT("%02x"), static_cast<unsigned int>(data[i]));
		}
		return out;
	}

	bool HexToBytes(const wxString& hex, std::vector<CryptoPP::byte>& out)
	{
		if (hex.Length() % 2 != 0) {
			return false;
		}
		out.resize(hex.Length() / 2);
		for (size_t i = 0; i < out.size(); ++i) {
			long value = 0;
			if (!hex.Mid(i * 2, 2).ToLong(&value, 16)) {
				return false;
			}
			out[i] = static_cast<CryptoPP::byte>(value & 0xFF);
		}
		return true;
	}

	wxString NormalizeHex(const wxString& value)
	{
		wxString lower(value);
		lower.MakeLower();
		return lower;
	}

} // namespace

bool IsPBKDF2Secret(const wxString& value)
{
	return value.StartsWith(wxString(PBKDF2_PREFIX));
}

bool IsLegacyMD5(const wxString& value)
{
	if (value.Length() != 32) {
		return false;
	}
	for (wxUniChar ch : value) {
		if (!wxIsxdigit(ch)) {
			return false;
		}
	}
	return true;
}

bool ExtractPBKDF2Info(const wxString& value, PBKDF2Info& outInfo)
{
	if (!IsPBKDF2Secret(value)) {
		return false;
	}
	wxArrayString tokens = wxSplit(value, '$');
	if (tokens.size() != 4) {
		return false;
	}
	unsigned long iter = 0;
	if (!tokens[1].ToULong(&iter)) {
		return false;
	}
	outInfo.iterations = static_cast<unsigned int>(iter);
	outInfo.saltHex = NormalizeHex(tokens[2]);
	outInfo.digestHex = NormalizeHex(tokens[3]);
	return true;
}

wxString BuildPBKDF2FromMD5(const wxString& md5Hex, unsigned int iterations)
{
	if (md5Hex.IsEmpty()) {
		return wxEmptyString;
	}
	CryptoPP::AutoSeededRandomPool rng;
	CryptoPP::SecByteBlock salt(kSaltBytes);
	rng.GenerateBlock(salt, salt.size());
	return BuildPBKDF2FromMD5WithSalt(md5Hex, iterations, BytesToHex(salt.data(), salt.size()));
}

wxString BuildPBKDF2FromMD5WithSalt(const wxString& md5Hex, unsigned int iterations, const wxString& saltHex)
{
	wxString normalizedSalt = NormalizeHex(saltHex);
	std::vector<CryptoPP::byte> saltBytes;
	if (!HexToBytes(normalizedSalt, saltBytes)) {
		return wxEmptyString;
	}
	wxString digest;
	if (!DeriveDigestFromMD5(md5Hex, iterations, normalizedSalt, digest)) {
		return wxEmptyString;
	}
	return wxString::Format(wxT("%s$%u$%s$%s"), PBKDF2_PREFIX, iterations, normalizedSalt, digest);
}

bool DeriveDigestFromMD5(const wxString& md5Hex, unsigned int iterations, const wxString& saltHex, wxString& outDigestHex)
{
	if (md5Hex.IsEmpty()) {
		return false;
	}
	wxString normalizedSalt = NormalizeHex(saltHex);
	std::vector<CryptoPP::byte> saltBytes;
	if (!HexToBytes(normalizedSalt, saltBytes)) {
		return false;
	}
	Unicode2CharBuf passwordBuf = unicode2char(md5Hex.Lower());
	const CryptoPP::byte* passwordBytes = reinterpret_cast<const CryptoPP::byte*>(passwordBuf.data());
	size_t passwordLen = passwordBuf.length();
	CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf;
	CryptoPP::SecByteBlock derived(kDerivedBytes);
	pbkdf.DeriveKey(derived, derived.size(), 0, passwordBytes, passwordLen, saltBytes.data(), saltBytes.size(), iterations);
	outDigestHex = BytesToHex(derived, derived.size());
	return true;
}

wxString NormalizeStoredSecret(const wxString& stored, bool& migrated)
{
	migrated = false;
	if (stored.IsEmpty()) {
		return wxEmptyString;
	}
	PBKDF2Info info;
	if (ExtractPBKDF2Info(stored, info)) {
		return wxString::Format(wxT("%s$%u$%s$%s"), PBKDF2_PREFIX, info.iterations, info.saltHex.Lower(), info.digestHex.Lower());
	}
	if (IsLegacyMD5(stored)) {
		migrated = true;
		return BuildPBKDF2FromMD5(stored);
	}
	return stored;
}

} // namespace SecretHash
