#include "lap/chassis_profile.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>
namespace lap { namespace {
std::wstring L(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
std::wstring T(std::wstring s){while(!s.empty()&&iswspace(s.front()))s.erase(s.begin());while(!s.empty()&&iswspace(s.back()))s.pop_back();return s;}

ChassisProfile SynthesizeDellChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_dell";
    p.source = L"Dell Universal Architecture Engine";
    p.displayName = model.empty() ? L"Dell Laptop" : model;
    
    if (lower.find(L"xps 13") != std::wstring::npos || lower.find(L"93") != std::wstring::npos) {
        p.profileId = L"dell_xps_13_universal";
        p.modelContains = L"XPS 13";
        p.ports.push_back({L"left_tb", L"Left Thunderbolt / USB-C", L"Left", L"USB-C", L"Thunderbolt / USB-C with Power Delivery", true, false, L""});
        p.ports.push_back({L"right_tb", L"Right Thunderbolt / USB-C", L"Right", L"USB-C", L"Thunderbolt / USB-C with Power Delivery", true, false, L""});
        p.ports.push_back({L"microsd", L"MicroSD Card Slot", L"Left", L"MicroSD", L"MicroSD card reader", false, false, L""});
    } else if (lower.find(L"xps 15") != std::wstring::npos || lower.find(L"precision 55") != std::wstring::npos || lower.find(L"precision 56") != std::wstring::npos) {
        p.profileId = L"dell_xps_15_precision_universal";
        p.modelContains = L"XPS 15 / Precision 5500";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt #1", L"Left", L"USB-C", L"Thunderbolt / USB4 / Power Delivery", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt #2", L"Left", L"USB-C", L"Thunderbolt / USB4 / Power Delivery", true, false, L""});
        p.ports.push_back({L"right_usbc", L"Right USB-C (DP / Power Delivery)", L"Right", L"USB-C", L"USB-C with DisplayPort and Power Delivery", true, false, L""});
        p.ports.push_back({L"right_sd", L"Right SD Card Reader", L"Right", L"SD", L"Full-size SD card slot", true, false, L""});
    } else if (lower.find(L"precision 7") != std::wstring::npos || lower.find(L"75") != std::wstring::npos || lower.find(L"77") != std::wstring::npos) {
        p.profileId = L"dell_precision_7000_universal";
        p.modelContains = L"Precision 7000";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt #1", L"Left", L"USB-C", L"Thunderbolt / DP / Power Delivery", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt #2", L"Left", L"USB-C", L"Thunderbolt / DP / Power Delivery", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 (PowerShare)", L"Right", L"USB-A", L"USB 3.2 with PowerShare", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Back", L"HDMI", L"HDMI 2.0 / 2.1", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Back", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"sd", L"SD Card Reader", L"Right", L"SD", L"Full-size SD Card Slot", true, false, L""});
    } else if (lower.find(L"g15") != std::wstring::npos || lower.find(L"alienware") != std::wstring::npos || lower.find(L"g3") != std::wstring::npos || lower.find(L"g5") != std::wstring::npos || lower.find(L"g7") != std::wstring::npos) {
        p.profileId = L"dell_gaming_universal";
        p.modelContains = L"Dell Gaming / Alienware";
        p.ports.push_back({L"usbc", L"Thunderbolt / USB-C with DisplayPort", L"Back", L"USB-C", L"Thunderbolt / DisplayPort alt mode", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Back", L"HDMI", L"HDMI 2.1 Video Output", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Left", L"RJ45", L"High-speed Ethernet LAN", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 #1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    } else {
        // Universal Dell Latitude / Inspiron / Vostro standard layout
        p.profileId = L"dell_business_universal";
        p.modelContains = L"Dell Business/Consumer";
        p.ports.push_back({L"usbc", L"USB-C / Thunderbolt with Power Delivery", L"Left", L"USB-C", L"USB-C / Thunderbolt / DisplayPort / Power Delivery", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Left", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 (PowerShare)", L"Right", L"USB-A", L"USB 3.2 Gen 1 with PowerShare", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Right", L"RJ45", L"Gigabit Ethernet LAN", false, false, L""});
        p.ports.push_back({L"sd", L"SD / MicroSD Card Slot", L"Right", L"MicroSD", L"SD or MicroSD card reader", false, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeLenovoChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_lenovo";
    p.source = L"Lenovo Universal Architecture Engine";
    p.displayName = model.empty() ? L"Lenovo Laptop" : model;

    if (lower.find(L"legion") != std::wstring::npos) {
        p.profileId = L"lenovo_legion_universal";
        p.modelContains = L"Lenovo Legion";
        p.ports.push_back({L"left_usbc", L"Left USB-C / DP", L"Left", L"USB-C", L"USB-C 3.2 Gen 2 / DP", true, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"back_usb_a1", L"Back USB 3.2 Gen 1 #1", L"Back", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"back_usb_a2", L"Back USB 3.2 Gen 1 #2 (Always On)", L"Back", L"USB-A", L"USB 3.2 Gen 1 Always On", true, false, L""});
        p.ports.push_back({L"back_usbc", L"Back USB-C (PD / DP)", L"Back", L"USB-C", L"USB-C with Power Delivery & DP", true, false, L""});
        p.ports.push_back({L"back_hdmi", L"Back HDMI 2.1", L"Back", L"HDMI", L"HDMI 2.1 Video Output", true, false, L""});
        p.ports.push_back({L"back_rj45", L"Back RJ-45 Ethernet", L"Back", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
    } else if (lower.find(L"x1 carbon") != std::wstring::npos || lower.find(L"x13") != std::wstring::npos || lower.find(L"t14s") != std::wstring::npos || lower.find(L"yoga slim") != std::wstring::npos) {
        p.profileId = L"lenovo_thinkpad_ultrabook_universal";
        p.modelContains = L"ThinkPad Ultrabook";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt / USB-C #1 (PD)", L"Left", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt / USB-C #2", L"Left", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1 (Always On)", L"Right", L"USB-A", L"USB 3.2 Gen 1 Always On", true, false, L""});
        p.ports.push_back({L"right_hdmi", L"Right HDMI Video Output", L"Right", L"HDMI", L"HDMI 2.0 Video Output", true, false, L""});
    } else {
        // Universal ThinkPad T/P/E/L & IdeaPad standard layout
        p.profileId = L"lenovo_standard_universal";
        p.modelContains = L"Lenovo ThinkPad/IdeaPad";
        p.ports.push_back({L"left_usbc1", L"Left USB-C / TB #1 (PD)", L"Left", L"USB-C", L"USB-C / Thunderbolt / Power Delivery", true, false, L""});
        p.ports.push_back({L"left_usbc2", L"Left USB-C / TB #2", L"Left", L"USB-C", L"USB-C / Thunderbolt / Power Delivery", false, false, L""});
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"left_hdmi", L"Left HDMI Video Output", L"Left", L"HDMI", L"HDMI 1.4 / 2.0 Video Output", true, false, L""});
        p.ports.push_back({L"right_rj45", L"Right RJ-45 Ethernet", L"Right", L"RJ45", L"Gigabit Ethernet LAN", false, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1 (Always On)", L"Right", L"USB-A", L"USB 3.2 Gen 1 Always On", true, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeHpChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_hp";
    p.source = L"HP Universal Architecture Engine";
    p.displayName = model.empty() ? L"HP Laptop" : model;

    if (lower.find(L"victus") != std::wstring::npos || lower.find(L"omen") != std::wstring::npos || lower.find(L"pavilion gaming") != std::wstring::npos) {
        p.profileId = L"hp_gaming_universal";
        p.modelContains = L"HP Gaming / Victus / Omen";
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Left", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 (Sleep & Charge)", L"Left", L"USB-A", L"USB 3.2 Gen 1 Sleep and Charge", true, false, L""});
        p.ports.push_back({L"usbc", L"USB-C / Thunderbolt (PD / DP)", L"Right", L"USB-C", L"USB-C with Power Delivery & DisplayPort", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI 2.1 Video Output", L"Right", L"HDMI", L"HDMI 2.1 Video Output", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"sd", L"SD Card Reader", L"Left", L"SD", L"Multi-format SD Card Reader", false, false, L""});
    } else if (lower.find(L"elitebook") != std::wstring::npos || lower.find(L"zbook") != std::wstring::npos || lower.find(L"spectre") != std::wstring::npos || lower.find(L"envy") != std::wstring::npos) {
        p.profileId = L"hp_premium_business_universal";
        p.modelContains = L"HP EliteBook / ZBook / Spectre";
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1 (Charging)", L"Left", L"USB-A", L"USB 3.2 Gen 1 with Charging", true, false, L""});
        p.ports.push_back({L"right_tb1", L"Right Thunderbolt / USB-C #1 (PD/DP)", L"Right", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"right_tb2", L"Right Thunderbolt / USB-C #2", L"Right", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"right_hdmi", L"Right HDMI Video Output", L"Right", L"HDMI", L"HDMI 2.0 Video Output", true, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1", L"Right", L"USB-A", L"USB 3.2 Gen 1", false, false, L""});
    } else {
        // Universal HP ProBook & Pavilion standard layout
        p.profileId = L"hp_mainstream_universal";
        p.modelContains = L"HP ProBook / Pavilion";
        p.ports.push_back({L"left_rj45", L"Left RJ-45 Ethernet", L"Left", L"RJ45", L"Gigabit Ethernet LAN", false, false, L""});
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_usbc", L"Right USB-C (PD / DP)", L"Right", L"USB-C", L"USB-C with Power Delivery & DP", true, false, L""});
        p.ports.push_back({L"right_usb_a1", L"Right USB 3.2 Gen 1 #1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_hdmi", L"Right HDMI Video Output", L"Right", L"HDMI", L"HDMI Video Output", true, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeAsusChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_asus";
    p.source = L"ASUS Universal Architecture Engine";
    p.displayName = model.empty() ? L"ASUS Laptop" : model;

    if (lower.find(L"tuf") != std::wstring::npos || lower.find(L"rog") != std::wstring::npos || lower.find(L"strix") != std::wstring::npos || lower.find(L"zephyrus") != std::wstring::npos) {
        p.profileId = L"asus_gaming_universal";
        p.modelContains = L"ASUS TUF / ROG Gaming";
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Left", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI 2.0 / 2.1 Video Output", L"Left", L"HDMI", L"HDMI 2.0 / 2.1 Video Output", true, false, L""});
        p.ports.push_back({L"usbc", L"Thunderbolt / USB-C (DP / G-Sync)", L"Left", L"USB-C", L"Thunderbolt / USB-C with DisplayPort", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 #1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    } else if (lower.find(L"zenbook") != std::wstring::npos) {
        p.profileId = L"asus_zenbook_universal";
        p.modelContains = L"ASUS ZenBook";
        p.ports.push_back({L"left_hdmi", L"Left HDMI Video Output", L"Left", L"HDMI", L"HDMI 2.0b / 2.1", true, false, L""});
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt / USB-C #1 (PD/DP)", L"Left", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt / USB-C #2", L"Left", L"USB-C", L"Thunderbolt / USB-C / PD / DP", true, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1 / 2", L"Right", L"USB-A", L"USB 3.2 Gen 1 / 2", true, false, L""});
        p.ports.push_back({L"right_microsd", L"Right MicroSD Card Reader", L"Right", L"MicroSD", L"MicroSD Card Reader", false, false, L""});
    } else {
        // Universal ASUS VivoBook / ExpertBook
        p.profileId = L"asus_mainstream_universal";
        p.modelContains = L"ASUS VivoBook / ExpertBook";
        p.ports.push_back({L"left_hdmi", L"Left HDMI Video Output", L"Left", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"left_usbc", L"Left USB-C 3.2", L"Left", L"USB-C", L"USB-C 3.2 Data / PD", true, false, L""});
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_usb_a1", L"Right USB 2.0 #1", L"Right", L"USB-A", L"USB 2.0", true, false, L""});
        p.ports.push_back({L"right_usb_a2", L"Right USB 2.0 #2", L"Right", L"USB-A", L"USB 2.0", false, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeAppleChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_apple";
    p.source = L"Apple Universal Architecture Engine";
    p.displayName = model.empty() ? L"Apple MacBook" : model;

    if (lower.find(L"pro 14") != std::wstring::npos || lower.find(L"pro 16") != std::wstring::npos || lower.find(L"macbookpro18") != std::wstring::npos) {
        p.profileId = L"apple_macbook_pro_universal";
        p.modelContains = L"MacBook Pro";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt 4 #1", L"Left", L"USB-C", L"Thunderbolt 4 / USB4 / PD / DP", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt 4 #2", L"Left", L"USB-C", L"Thunderbolt 4 / USB4 / PD / DP", true, false, L""});
        p.ports.push_back({L"right_tb3", L"Right Thunderbolt 4", L"Right", L"USB-C", L"Thunderbolt 4 / USB4 / PD / DP", true, false, L""});
        p.ports.push_back({L"right_hdmi", L"Right HDMI Video Output", L"Right", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"right_sd", L"Right SDXC Card Reader", L"Right", L"SD", L"SDXC Card Slot", true, false, L""});
    } else {
        // Universal MacBook Air & 13-inch Pro (Dual USB-C)
        p.profileId = L"apple_macbook_compact_universal";
        p.modelContains = L"MacBook Air / Pro 13";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt / USB4 #1", L"Left", L"USB-C", L"Thunderbolt / USB4 / PD / DP", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt / USB4 #2", L"Left", L"USB-C", L"Thunderbolt / USB4 / PD / DP", true, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeAcerChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_acer";
    p.source = L"Acer Universal Architecture Engine";
    p.displayName = model.empty() ? L"Acer Laptop" : model;

    if (lower.find(L"nitro") != std::wstring::npos || lower.find(L"predator") != std::wstring::npos || lower.find(L"helios") != std::wstring::npos) {
        p.profileId = L"acer_gaming_universal";
        p.modelContains = L"Acer Nitro / Predator";
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Left", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 #1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI 2.0 / 2.1 Video Output", L"Right", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"usbc", L"Thunderbolt / USB-C (PD / DP)", L"Right", L"USB-C", L"Thunderbolt / USB-C with DP", true, false, L""});
        p.ports.push_back({L"usb_a3", L"USB 3.2 Gen 2 (Power-off Charging)", L"Right", L"USB-A", L"USB 3.2 Gen 2 with Power-off Charging", true, false, L""});
    } else {
        // Universal Acer Swift / Aspire
        p.profileId = L"acer_mainstream_universal";
        p.modelContains = L"Acer Swift / Aspire";
        p.ports.push_back({L"left_usbc", L"Left USB-C / TB (PD / DP)", L"Left", L"USB-C", L"USB-C / Thunderbolt / PD / DP", true, false, L""});
        p.ports.push_back({L"left_hdmi", L"Left HDMI Video Output", L"Left", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"left_usb_a", L"Left USB 3.2 Gen 1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_usb_a", L"Right USB 3.2 Gen 1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeMsiChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_msi";
    p.source = L"MSI Universal Architecture Engine";
    p.displayName = model.empty() ? L"MSI Laptop" : model;

    if (lower.find(L"gf63") != std::wstring::npos || lower.find(L"katana") != std::wstring::npos || lower.find(L"bravo") != std::wstring::npos || lower.find(L"stealth") != std::wstring::npos || lower.find(L"raider") != std::wstring::npos) {
        p.profileId = L"msi_gaming_universal";
        p.modelContains = L"MSI Gaming";
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Right", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Right", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"usbc", L"USB-C 3.2 (DP / Data)", L"Right", L"USB-C", L"USB-C 3.2 with DP or Data", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 #1", L"Left", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    } else {
        // Universal MSI Modern / Prestige
        p.profileId = L"msi_modern_universal";
        p.modelContains = L"MSI Modern / Prestige";
        p.ports.push_back({L"left_hdmi", L"Left HDMI Video Output", L"Left", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"left_usbc", L"Left USB-C (PD / DP)", L"Left", L"USB-C", L"USB-C with Power Delivery", true, false, L""});
        p.ports.push_back({L"left_microsd", L"Left MicroSD Card Reader", L"Left", L"MicroSD", L"MicroSD Card Reader", false, false, L""});
        p.ports.push_back({L"right_usb_a1", L"Right USB 3.2 Gen 1 #1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"right_usb_a2", L"Right USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    }
    return p;
}

ChassisProfile SynthesizeUniversalChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    p.confidence = Confidence::Low;
    p.validationStatus = L"heuristic_universal";
    p.source = L"Universal PC Architecture Engine";
    p.displayName = model.empty() ? L"Windows Laptop" : model;
    p.profileId = L"universal_pc_standard";
    p.modelContains = L"Universal";
    p.ports.push_back({L"left_usbc", L"USB-C / Power Delivery Port", L"Left", L"USB-C", L"USB-C Data / Video / Power Delivery", true, false, L""});
    p.ports.push_back({L"left_usb_a", L"USB-A Data Port #1", L"Left", L"USB-A", L"USB 3.0 / 3.2 Standard Port", true, false, L""});
    p.ports.push_back({L"right_usb_a", L"USB-A Data Port #2", L"Right", L"USB-A", L"USB 3.0 / 3.2 Standard Port", true, false, L""});
    p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Left", L"HDMI", L"HDMI Display Output", true, false, L""});
    return p;
}

ChassisProfile ProtectedPrecisionPilotBaseline(const std::wstring& model) {
    const auto lower = L(model);
    const bool protectedModel =
        lower.find(L"precision 5560") != std::wstring::npos ||
        lower.find(L"precision 5570") != std::wstring::npos ||
        lower.find(L"precision 7670") != std::wstring::npos;
    if (!protectedModel) return {};

    auto baseline = SynthesizeDellChassisProfile(model);
    baseline.validationStatus = L"embedded-advisory-baseline";
    baseline.source = L"LapSure embedded Precision pilot baseline";
    baseline.reference = L"Release-defined expected-port denominator; portable metadata is advisory overlay only.";
    if (lower.find(L"precision 7670") != std::wstring::npos) {
        baseline.profileId = L"lapsure_precision_7670_expected_ports";
        baseline.modelContains = L"Precision 7670";
    } else if (lower.find(L"precision 5570") != std::wstring::npos) {
        baseline.profileId = L"lapsure_precision_5570_expected_ports";
        baseline.modelContains = L"Precision 5570";
    } else {
        baseline.profileId = L"lapsure_precision_5560_expected_ports";
        baseline.modelContains = L"Precision 5560";
    }
    return baseline;
}
}

ChassisProfile LoadChassisProfile(const std::wstring&a,const std::wstring&m){
 ChassisProfile best{};std::error_code ec;auto d=std::filesystem::path(a)/L"profiles"/L"chassis";
 if(!std::filesystem::exists(d,ec)){
  auto cur=std::filesystem::path(a);
  for(int depth=0;depth<5&&cur.has_parent_path();++depth){
   cur=cur.parent_path();
   auto alt=cur/L"profiles"/L"chassis";
   if(std::filesystem::exists(alt,ec)){d=alt;break;}
  }
 }
 if(std::filesystem::exists(d,ec)){
  for(auto&e:std::filesystem::directory_iterator(d,ec)){
   if(ec||!e.is_regular_file())continue;
   std::wifstream f(e.path());ChassisProfile p{};p.source=e.path().filename().wstring();p.confidence=Confidence::Medium;std::wstring line;
   while(std::getline(f,line)){
    line=T(line);if(line.empty()||line[0]==L'#')continue;auto q=line.find(L'=');if(q==std::wstring::npos)continue;auto k=T(line.substr(0,q)),v=T(line.substr(q+1));
    if(k==L"profileId")p.profileId=v;else if(k==L"modelContains")p.modelContains=v;else if(k==L"displayName")p.displayName=v;else if(k==L"validationStatus")p.validationStatus=L(v);else if(k==L"reference")p.reference=v;
    else if(k==L"port"){std::wstringstream ss(v);std::wstring x;std::vector<std::wstring>z;while(std::getline(ss,x,L'|'))z.push_back(T(x));if(z.size()>=6){ChassisPortDefinition c{};c.id=z[0];c.label=z[1];c.side=z[2];c.connector=z[3];c.capability=z[4];c.required=L(z[5])!=L"false";p.ports.push_back(c);}}
   }
   if(!p.modelContains.empty()&&L(m).find(L(p.modelContains))!=std::wstring::npos&&p.modelContains.size()>best.modelContains.size())best=p;
  }
 }
 if(best.profileId.empty() && !m.empty()){
  std::wstring lm = L(m);
  if(lm.find(L"dell")!=std::wstring::npos||lm.find(L"latitude")!=std::wstring::npos||lm.find(L"precision")!=std::wstring::npos||lm.find(L"xps")!=std::wstring::npos||lm.find(L"inspiron")!=std::wstring::npos||lm.find(L"vostro")!=std::wstring::npos||lm.find(L"alienware")!=std::wstring::npos||lm.find(L"g15")!=std::wstring::npos||lm.find(L"g16")!=std::wstring::npos||lm.find(L"g3")!=std::wstring::npos||lm.find(L"g5")!=std::wstring::npos){
   best = SynthesizeDellChassisProfile(m);
  } else if(lm.find(L"lenovo")!=std::wstring::npos||lm.find(L"thinkpad")!=std::wstring::npos||lm.find(L"legion")!=std::wstring::npos||lm.find(L"ideapad")!=std::wstring::npos||lm.find(L"yoga")!=std::wstring::npos){
   best = SynthesizeLenovoChassisProfile(m);
  } else if(lm.find(L"hp")!=std::wstring::npos||lm.find(L"hewlett")!=std::wstring::npos||lm.find(L"elitebook")!=std::wstring::npos||lm.find(L"probook")!=std::wstring::npos||lm.find(L"zbook")!=std::wstring::npos||lm.find(L"victus")!=std::wstring::npos||lm.find(L"omen")!=std::wstring::npos||lm.find(L"pavilion")!=std::wstring::npos||lm.find(L"envy")!=std::wstring::npos||lm.find(L"spectre")!=std::wstring::npos){
   best = SynthesizeHpChassisProfile(m);
  } else if(lm.find(L"asus")!=std::wstring::npos||lm.find(L"zenbook")!=std::wstring::npos||lm.find(L"vivobook")!=std::wstring::npos||lm.find(L"rog")!=std::wstring::npos||lm.find(L"tuf")!=std::wstring::npos||lm.find(L"expertbook")!=std::wstring::npos){
   best = SynthesizeAsusChassisProfile(m);
  } else if(lm.find(L"apple")!=std::wstring::npos||lm.find(L"macbook")!=std::wstring::npos){
   best = SynthesizeAppleChassisProfile(m);
  } else if(lm.find(L"acer")!=std::wstring::npos||lm.find(L"nitro")!=std::wstring::npos||lm.find(L"predator")!=std::wstring::npos||lm.find(L"swift")!=std::wstring::npos||lm.find(L"aspire")!=std::wstring::npos){
   best = SynthesizeAcerChassisProfile(m);
  } else if(lm.find(L"msi")!=std::wstring::npos||lm.find(L"katana")!=std::wstring::npos||lm.find(L"bravo")!=std::wstring::npos||lm.find(L"stealth")!=std::wstring::npos){
   best = SynthesizeMsiChassisProfile(m);
  } else {
   best = SynthesizeUniversalChassisProfile(m);
  }
 }
 return best;
}

ChassisProfile LoadDecisionChassisProfile(const std::wstring& appDir,const std::wstring& model){
 auto raw=LoadChassisProfile(appDir,model);
 if(raw.validationStatus==L"physical-verified"){
  raw.validationStatus=L"static-unverified";
  if(!raw.reference.empty())raw.reference+=L" | ";
  raw.reference+=L"Portable chassis metadata is not authenticated physical-verification evidence.";
 }

 auto decision=ProtectedPrecisionPilotBaseline(model);
 if(decision.profileId.empty())return raw;

 for(auto& expected:decision.ports){
  expected.tested=false;
  expected.verdict=L"NOT TESTED";
  const auto it=std::find_if(raw.ports.begin(),raw.ports.end(),[&](const auto& advisory){return advisory.id==expected.id;});
  if(it==raw.ports.end())continue;
  if(!it->label.empty())expected.label=it->label;
  if(!it->side.empty())expected.side=it->side;
  if(!it->connector.empty())expected.connector=it->connector;
  if(!it->capability.empty())expected.capability=it->capability;
 }

 for(auto advisory:raw.ports){
  const auto alreadyProtected=std::any_of(decision.ports.begin(),decision.ports.end(),[&](const auto& expected){return expected.id==advisory.id;});
  if(alreadyProtected)continue;
  advisory.required=false;
  advisory.tested=false;
  advisory.verdict=L"NOT TESTED";
  decision.ports.push_back(std::move(advisory));
 }

 if(!raw.source.empty())decision.reference+=L" Advisory overlay: "+raw.source+L".";
 return decision;
}

void ApplyPortResultToChassisProfile(ChassisProfile&p,const PortProbeResult&r){
 for(auto&x:p.ports){
  const bool stableMatch=!r.expectedPortId.empty()&&x.id==r.expectedPortId;
  const bool legacyFallback=r.expectedPortId.empty()&&(x.label==r.portLabel||x.id==r.portLabel);
  if(stableMatch||legacyFallback){x.tested=true;x.verdict=r.verdict;return;}
 }
}
unsigned RequiredPortsRemaining(const ChassisProfile&p){unsigned n=0;for(auto&x:p.ports)if(x.required&&!x.tested)n++;return n;}
}