from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
APP = read_app_source(ROOT)
MAIN = (ROOT / "src/main_round5.cpp").read_text(encoding="utf-8")
CLOUD_H = (ROOT / "include/lap/cloud_lookup.h").read_text(encoding="utf-8")
CLOUD = (ROOT / "src/cloud_lookup.cpp").read_text(encoding="utf-8")
PROFILE_H = (ROOT / "include/lap/profile.h").read_text(encoding="utf-8")
PROFILE = (ROOT / "src/profile.cpp").read_text(encoding="utf-8")
CHASSIS_H = (ROOT / "include/lap/chassis_profile.h").read_text(encoding="utf-8")
CHASSIS = (ROOT / "src/chassis_profile.cpp").read_text(encoding="utf-8")
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

# Portable factory/chassis metadata is mutable and therefore advisory only.
require("trustedProvenance" in PROFILE_H and "TrustedExact()" in PROFILE_H,
        "profile load result must expose an explicit provenance trust contract")
require("LoadDecisionFactoryProfile" in PROFILE_H,
        "factory profile decision boundary must discard untrusted advisory data")
require("scanDir(root" in PROFILE,
        "static top-level profile parser must remain bounded to the intended directory")
require("out.loaded = false" in PROFILE and "out.exact = false" in PROFILE and
        "out.trustedProvenance = false" in PROFILE,
        "unsigned portable static profiles must remain advisory rather than factory truth")
require("trustedProvenance = true" not in PROFILE,
        "portable static profile loader must not self-promote provenance trust")
require('root / L"cache"' not in PROFILE,
        "normal factory profile loader must not promote mutable cache data")
require(
    "LoadDecisionChassisProfile" in CHASSIS_H and
    'raw.validationStatus==L"physical-verified"' in CHASSIS and
    'raw.validationStatus=L"static-unverified"' in CHASSIS and
    "ProtectedPrecisionPilotBaseline" in CHASSIS and
    "advisory.required=false" in CHASSIS,
    "mutable chassis metadata must remain advisory at the decision boundary and cannot shrink the protected denominator",
)

# Production fragments are routed through decision-safe wrappers while technician
# batch pre-cache remains implemented separately in cloud_lookup.cpp.
for raw_name, safe_name in [
    ("LoadFactoryProfile", "LoadDecisionFactoryProfile"),
    ("LookupFactoryProfileOnline", "LookupFactoryProfileForDecision"),
    ("LoadChassisProfile", "LoadDecisionChassisProfile"),
]:
    require(f"#define {raw_name} {safe_name}" in MAIN,
            f"production app must route {raw_name} through {safe_name}")

# Remote identity must be exact and the payload remains unauthenticated advisory evidence.
require("identityMatched" in CLOUD_H and "authenticatedProvenance" in CLOUD_H,
        "cloud result must distinguish identity match from authenticated provenance")
require("LookupFactoryProfileForDecision" in CLOUD_H and
        "result.identityMatched && result.authenticatedProvenance" in CLOUD_H,
        "decision cloud boundary must require exact identity plus authenticated provenance")
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
require("portable" in SECURITY.lower() and "physical-verified" in SECURITY.lower(),
        "security policy must document mutable portable profile limitations")

print("Round 5 cloud/profile privacy and provenance contract: PASS")