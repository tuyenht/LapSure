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
};

// Loads reviewed static profiles only. Mutable profiles/cache content is advisory
// and intentionally excluded from the factory-truth path until authenticated
// provenance is implemented.
ProfileLoadResult LoadFactoryProfile(const std::wstring& profilesDir,
                                     const std::wstring& model,
                                     const std::wstring& serviceTag);
}
