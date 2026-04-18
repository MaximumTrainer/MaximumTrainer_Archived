// WASM stub for CredentialStore.
// Third-party service tokens are not persisted in the browser context —
// browser-local storage has no reliable encryption boundary and the WASM
// build targets single-session use.  All operations are silent no-ops.

#include "credential_store.h"

bool CredentialStore::store(const QString & /*service*/,
                            const QString & /*key*/,
                            const QString & /*value*/)
{
    return false;
}

QString CredentialStore::load(const QString & /*service*/, const QString & /*key*/)
{
    return {};
}

void CredentialStore::remove(const QString & /*service*/, const QString & /*key*/)
{
}
