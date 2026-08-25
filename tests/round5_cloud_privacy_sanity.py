from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
APP = read_app_source(ROOT)
CLOUD_H = (ROOT / "include/lap/cloud_lookup.h").read_text(encoding="utf-8")
CLOUD = (ROOT / "src/cloud_lookup.cpp").read_text(encoding="utf-8")
PROFILE_H = (ROOT / "include/lap/profile.h").read_text(encoding="utf-8")
PROFILE = (ROOT / "src/profile.cpp").read_text(encoding="utf-8")
SECURITY = (ROOT / "SECURITY.md").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# Normal GUI/inventory calls use the default fail-closed network policy.
require("bool allowNetwork = false" in CLOUD_H,
        "cloud lookup must be network-disabled by default")
require("if (!allowNetwork)" in CLOUD and "Cloud lookup is disabled by default" in CLOUD,
        "cloud implementation must fail before cache/network access unless explicitly opted in")
require("RunBatchPreCache" in APP and "--cache-tag" in APP,
        "explicit technician pre-cache CLI must remain the opt-in network path")
require("LookupFactoryProfileOnline(appDir, vendor, L\"\", tag, timeoutMs, true)" in CLOUD,
        "only batch pre-cache may opt in to network lookup")
app_lookup_lines = [line for line in APP.splitlines() if "LookupFactoryProfileOnline" in line]
require(app_lookup_lines and all(", true" not in line for line in app_lookup_lines),
        "production GUI/inventory cloud calls must use the default network-disabled policy")

# Mutable cache is advisory; reviewed static profile files are the only current trusted profile source.
require("trustedProvenance" in PROFILE_H,
        "profile load result must expose provenance trust")
require("scanDir(root" in PROFILE and "trustedProvenance = true" in PROFILE,
        "reviewed static profile scan must mark trusted provenance explicitly")
require('root / L"cache"' not in PROFILE,
        "normal factory profile loader must not promote mutable cache data")

# Remote identity must be exact and the payload remains unauthenticated advisory evidence.
require("identityMatched" in CLOUD_H and "authenticatedProvenance" in CLOUD_H,
        "cloud result must distinguish identity match from authenticated provenance")
require("IdentityEquals(profile.serviceTag, serviceTag)" in CLOUD,
        "cloud response Service Tag must exactly match the requested identity")
require("profile.serviceTag = serviceTag" not in CLOUD,
        "missing cloud identity must never be filled from the request")
require("authenticatedProvenance = false" in CLOUD,
        "unsigned cloud/cache profiles must not be promoted to trusted factory provenance")

# Identifiers and network input/output are bounded and encoded.
for token in ["PercentEncodeQueryComponent", "kMaxCloudResponseBytes", "kMaxCloudUrlChars", "INTERNET_SCHEME_HTTPS"]:
    require(token in CLOUD, f"missing cloud privacy/bounding primitive: {token}")
require("responseData.size()" in CLOUD and "kMaxCloudResponseBytes" in CLOUD,
        "cloud response must be bounded before allocation/append")
require("WINHTTP_OPTION_REDIRECT_POLICY_NEVER" in CLOUD,
        "OEM lookup must not redirect device identifiers to another host")
require('res.source = L"OEM Cloud Lookup (advisory; unauthenticated profile provenance)"' in CLOUD,
        "report provenance must not echo the Service Tag query URL")

require("Cloud/profile privacy and provenance" in SECURITY,
        "security policy must document cloud privacy/provenance behavior")
require("disabled by default" in SECURITY.lower() and "pre-cache" in SECURITY.lower(),
        "security policy must document explicit cloud opt-in")

print("Round 5 cloud privacy/provenance contract: PASS")
