#include <muleunit/test.h>

#include <ECPreferencesValidation.h>
#include <common/Path.h>
#include <wx/filename.h>

using namespace muleunit;

static CInternalPathContext BuildECContext()
{
	wxString base = JoinPaths(wxFileName::GetTempDir(), wxT("wmule-ec-guard"));
	wxString incoming = JoinPaths(base, wxT("Incoming"));
	wxString temp = JoinPaths(base, wxT("Temp"));
	CPath::MakeDir(CPath(base));
	CPath::MakeDir(CPath(incoming));
	CPath::MakeDir(CPath(temp));

	CInternalPathContext context = {base, temp, incoming, true, false, false};
	return context;
}

DECLARE_SIMPLE(ECPreferencesValidationTest);

TEST(ECPreferencesValidationTest, RejectsTraversalKeepsPreviousAndReportsError)
{
	CInternalPathContext context = BuildECContext();
	CPath previous(context.m_incomingDir);
	wxString invalidPath = JoinPaths(context.m_configDir, wxT("..\\escape"));

	ECDirectoryUpdateOutcome outcome = ApplyECPathPreference(EInternalPathKind::IncomingDir,
		invalidPath, previous, context);

	ASSERT_FALSE(outcome.m_ok);
	ASSERT_EQUALS(previous.GetRaw(), outcome.m_effectivePath);
	ASSERT_EQUALS(wxT("path contains traversal"), outcome.m_error);
	ASSERT_EQUALS(previous.GetRaw(), context.m_incomingDir);
}

TEST(ECPreferencesValidationTest, AcceptsTempAndUpdatesContext)
{
	CInternalPathContext context = BuildECContext();
	CPath previous(context.m_tempDir);
	wxString newTemp = JoinPaths(context.m_configDir, wxT("TempNew"));

	ECDirectoryUpdateOutcome outcome = ApplyECPathPreference(EInternalPathKind::TempDir,
		newTemp, previous, context);

	ASSERT_TRUE(outcome.m_ok);
	ASSERT_EQUALS(outcome.m_effectivePath, context.m_tempDir);
	ASSERT_TRUE(context.m_tempDir.EndsWith(wxFileName::GetPathSeparator()));
	ASSERT_FALSE(outcome.m_usedUnsafeBypass);
}

TEST(ECPreferencesValidationTest, AcceptsExternalIncomingAndFlagsExternal)
{
	CInternalPathContext context = BuildECContext();
	CPath previous(context.m_incomingDir);
	wxString outside = JoinPaths(wxFileName::GetTempDir(), wxT("ec-escape"));

	ECDirectoryUpdateOutcome outcome = ApplyECPathPreference(EInternalPathKind::IncomingDir,
		outside, previous, context);

	ASSERT_TRUE(outcome.m_ok);
	ASSERT_TRUE(outcome.m_usedUnsafeBypass);
	ASSERT_EQUALS(outcome.m_effectivePath, context.m_incomingDir);
	ASSERT_TRUE(outcome.m_effectivePath.StartsWith(outside));
}

TEST(ECPreferencesValidationTest, AcceptsExternalTempWithoutUnsafeGate)
{
	CInternalPathContext context = BuildECContext();
	CPath previous(context.m_tempDir);
	context.m_allowUnsafePreference = false;
	context.m_allowUnsafeOutsideBase = false;
	wxString outside = JoinPaths(wxFileName::GetTempDir(), wxT("ec-external-temp"));

	ECDirectoryUpdateOutcome outcome = ApplyECPathPreference(EInternalPathKind::TempDir,
		outside, previous, context);

	ASSERT_TRUE(outcome.m_ok);
	ASSERT_TRUE(outcome.m_usedUnsafeBypass);
	ASSERT_EQUALS(outcome.m_effectivePath, context.m_tempDir);
}

// Validación manual mínima (rutas externas):
// 1) Desde la GUI/EC, apuntar Temp/Incoming a rutas absolutas fuera de ConfigDir: deben aceptarse sin activar
//    el modo inseguro y registrar un aviso "Accepted <kind> outside base".
// 2) Intentar rutas relativas, con traversal o apuntando a ficheros: debe rechazarse, conservar el valor previo
//    y mostrar el error "path must be absolute" / "path contains traversal" / "path points to a file".
// 3) Desconectar el almacenamiento externo (simular permiso denegado) y comprobar que aparece el error
//    "access denied creating directory" y el fallback conserva el valor anterior.
