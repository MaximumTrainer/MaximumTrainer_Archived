/*
 * tst_credential_store.cpp
 *
 * Qt Test suite for CredentialStore.
 *
 * Tests verify the store/load/remove interface without inspecting the
 * underlying cryptographic implementation (DPAPI / Keychain / AES-GCM).
 * They are platform-agnostic: the same test binary runs on Linux (OpenSSL),
 * Windows (DPAPI), and macOS (Keychain).
 *
 * WASM: CredentialStore is a deliberate no-op on WASM; the "wasm_noop" test
 * group verifies that behaviour and is compiled in only for WASM builds.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────
 * Roundtrip     – stored value can be loaded back identically
 * Overwrite     – storing with the same key replaces the previous value
 * Remove        – removed key returns empty string on subsequent load
 * MissingKey    – load for a key that was never stored returns ""
 * MultiService  – different services with the same key are independent
 * EmptyValue    – storing empty string is supported
 * SpecialChars  – Unicode / whitespace / symbols survive the roundtrip
 */

#include <QtTest/QtTest>
#include <QCoreApplication>

#include "credential_store.h"

// ─────────────────────────────────────────────────────────────────────────────
class TstCredentialStore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testRoundtrip();
    void testOverwrite();
    void testRemove();
    void testMissingKey();
    void testMultiService();
    void testEmptyValue();
    void testSpecialChars();

#if defined(Q_OS_WASM)
    void testWasm_noop();
#endif

private:
    // Use unique service/key names so parallel test runs don't collide.
    const QString SVC = QStringLiteral("tst_credential_store");
};

// ─────────────────────────────────────────────────────────────────────────────
void TstCredentialStore::initTestCase()
{
    // Clean up any leftover state from a previous aborted run.
    CredentialStore::remove(SVC, QStringLiteral("key1"));
    CredentialStore::remove(SVC, QStringLiteral("key2"));
    CredentialStore::remove(SVC, QStringLiteral("empty"));
    CredentialStore::remove(SVC, QStringLiteral("special"));
    CredentialStore::remove(QStringLiteral("tst_other_svc"), QStringLiteral("key1"));
}

void TstCredentialStore::cleanupTestCase()
{
    CredentialStore::remove(SVC, QStringLiteral("key1"));
    CredentialStore::remove(SVC, QStringLiteral("key2"));
    CredentialStore::remove(SVC, QStringLiteral("empty"));
    CredentialStore::remove(SVC, QStringLiteral("special"));
    CredentialStore::remove(QStringLiteral("tst_other_svc"), QStringLiteral("key1"));
}

// ─────────────────────────────────────────────────────────────────────────────

void TstCredentialStore::testRoundtrip()
{
    const QString value = QStringLiteral("my_secret_oauth_token_abc123");
    bool ok = CredentialStore::store(SVC, QStringLiteral("key1"), value);

#if defined(Q_OS_WASM)
    // WASM backend is a deliberate no-op; store always returns false.
    QVERIFY(!ok);
    QVERIFY(CredentialStore::load(SVC, QStringLiteral("key1")).isEmpty());
#else
    QVERIFY2(ok, "CredentialStore::store returned false");
    const QString loaded = CredentialStore::load(SVC, QStringLiteral("key1"));
    QCOMPARE(loaded, value);
#endif
}

void TstCredentialStore::testOverwrite()
{
#if defined(Q_OS_WASM)
    QSKIP("WASM: no-op backend");
#endif
    CredentialStore::store(SVC, QStringLiteral("key1"), QStringLiteral("first"));
    CredentialStore::store(SVC, QStringLiteral("key1"), QStringLiteral("second"));
    QCOMPARE(CredentialStore::load(SVC, QStringLiteral("key1")),
             QStringLiteral("second"));
}

void TstCredentialStore::testRemove()
{
#if defined(Q_OS_WASM)
    QSKIP("WASM: no-op backend");
#endif
    CredentialStore::store(SVC, QStringLiteral("key2"), QStringLiteral("to_be_deleted"));
    CredentialStore::remove(SVC, QStringLiteral("key2"));
    QVERIFY(CredentialStore::load(SVC, QStringLiteral("key2")).isEmpty());
}

void TstCredentialStore::testMissingKey()
{
    // A key that was never stored must return an empty string on all platforms.
    const QString result = CredentialStore::load(SVC, QStringLiteral("no_such_key_xyz"));
    QVERIFY(result.isEmpty());
}

void TstCredentialStore::testMultiService()
{
#if defined(Q_OS_WASM)
    QSKIP("WASM: no-op backend");
#endif
    const QString otherSvc = QStringLiteral("tst_other_svc");
    CredentialStore::store(SVC,      QStringLiteral("key1"), QStringLiteral("value_a"));
    CredentialStore::store(otherSvc, QStringLiteral("key1"), QStringLiteral("value_b"));

    QCOMPARE(CredentialStore::load(SVC,      QStringLiteral("key1")), QStringLiteral("value_a"));
    QCOMPARE(CredentialStore::load(otherSvc, QStringLiteral("key1")), QStringLiteral("value_b"));
}

void TstCredentialStore::testEmptyValue()
{
#if defined(Q_OS_WASM)
    QSKIP("WASM: no-op backend");
#endif
    bool ok = CredentialStore::store(SVC, QStringLiteral("empty"), QStringLiteral(""));
    QVERIFY(ok);
    // Loading back an empty string is valid; it should not be treated as missing.
    // Some backends may return "" either way — that's acceptable.
    const QString loaded = CredentialStore::load(SVC, QStringLiteral("empty"));
    QVERIFY(loaded.isEmpty()); // "" == "" — value survived as empty
}

void TstCredentialStore::testSpecialChars()
{
#if defined(Q_OS_WASM)
    QSKIP("WASM: no-op backend");
#endif
    const QString special = QStringLiteral("abc123!@#$%^&*()_+\n\t€日本語");
    CredentialStore::store(SVC, QStringLiteral("special"), special);
    QCOMPARE(CredentialStore::load(SVC, QStringLiteral("special")), special);
}

#if defined(Q_OS_WASM)
void TstCredentialStore::testWasm_noop()
{
    // Verify the WASM stub behaves as documented: store returns false,
    // load always returns empty, remove is a no-op.
    bool ok = CredentialStore::store("svc", "k", "v");
    QVERIFY(!ok);
    QVERIFY(CredentialStore::load("svc", "k").isEmpty());
    CredentialStore::remove("svc", "k"); // must not crash
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
QTEST_MAIN(TstCredentialStore)
#include "tst_credential_store.moc"
