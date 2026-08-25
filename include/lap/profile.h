#pragma once
#include "model.h"
#include <string>

namespace lap {
struct ProfileLoadResult {
    FactoryProfile profile;
    bool exact{false};
    bool trustedProvenance{false};
    bool loaded{false};
    std::wstring source;
    std::wstring error;

    bool TrustedExact() const noexcept {
        return loaded && exact && trustedProvenance;
    }
};

// Parses static profiles directly under profiles/. Portable files are mutable and
// therefore remain advisory until an authenticated/hash-pinned provenance scheme
// is implemented. Mutable profiles/cache content is excluded entirely.
ProfileLoadResult LoadFactoryProfile(const std::wstring& profilesDir,
                                     const std::wstring& model,
                                     const std::wstring& serviceTag);

// Production decision boundary: untrusted/advisory profile data is discarded
// before any collector can turn it into Factory PASS/FAIL evidence.
inline ProfileLoadResult LoadDecisionFactoryProfile(const std::wstring& profilesDir,
                                                    const std::wstring& model,
                                                    const std::wstring& serviceTag) {
    auto result = LoadFactoryProfile(profilesDir, model, serviceTag);
    if (!result.TrustedExact()) {
        result.loaded = false;
        result.exact = false;
        result.trustedProvenance = false;
        result.profile = FactoryProfile{};
    }
    return result;
}
}
