#pragma once
#include "model.h"
#include <string>

namespace lap {
struct ProfileLoadResult {
    FactoryProfile profile;
    bool exact{false};
    bool loaded{false};
    std::wstring source;
    std::wstring error;
};
ProfileLoadResult LoadFactoryProfile(const std::wstring& profilesDir,
                                     const std::wstring& model,
                                     const std::wstring& serviceTag);
}
