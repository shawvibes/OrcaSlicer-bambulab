#ifndef slic3r_wgtFilaManagerPrintCheck_h_
#define slic3r_wgtFilaManagerPrintCheck_h_

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/ProjectTask.hpp"
#include "wgtFilaManagerStore.h"

namespace Slic3r { namespace GUI {

struct FilamentShortage {
    int         plate_index   = -1; // 0-based; -1 when not tied to a plate
    int         filament_slot = 0;  // 0-based project filament index
    std::string spool_id;
    std::string display_label;
    double      required_g  = 0.0;
    double      remaining_g = 0.0;
    double      short_by_g  = 0.0;
};

struct FilamentInventoryCheckResult {
    std::vector<FilamentShortage> shortages;
    int                           unmapped_slot_count       = 0;
    int                           unknown_weight_slot_count = 0;

    bool has_shortage() const { return !shortages.empty(); }
    bool has_unmapped_slots() const { return unmapped_slot_count > 0; }

    void merge(const FilamentInventoryCheckResult& other);
};

struct FilamentSlotMapping {
    int         filament_slot = 0;
    std::string setting_id;
    std::string color_code;
    std::string tag_uid;
};

using TrayTagUidLookup = std::function<std::string(const std::string& ams_id, int slot_id)>;

// Remaining net filament on a spool, in grams. Returns < 0 when unknown.
double spool_remaining_grams(const FilamentSpool& spool);

std::string format_spool_display_label(const FilamentSpool& spool);

// Required grams per 0-based filament slot from post-slice gcode statistics
// (total_volumes_per_extruder, includes flush / wipe tower / support).
std::map<size_t, double> compute_required_grams_per_filament(const GCodeProcessorResult& result);

const FilamentSpool* resolve_spool_for_slot(
    const wgtFilaManagerStore& store, const FilamentSlotMapping& mapping);

std::vector<FilamentSlotMapping> build_filament_slot_mappings(
    const std::map<size_t, double>& required_g_per_slot,
    const std::vector<std::string>& filament_setting_ids,
    const std::vector<std::string>& filament_colours,
    const std::vector<FilamentInfo>* ams_mapping           = nullptr,
    const TrayTagUidLookup&           tray_tag_uid_lookup   = {});

FilamentInventoryCheckResult check_filament_inventory(
    const std::map<size_t, double>&     required_g_per_slot,
    const wgtFilaManagerStore&          store,
    const std::vector<FilamentSlotMapping>& slot_mappings);

FilamentInventoryCheckResult check_slice_result_inventory(
    const GCodeProcessorResult&         result,
    const wgtFilaManagerStore&          store,
    const std::vector<std::string>&     filament_setting_ids,
    const std::vector<std::string>&     filament_colours,
    const std::vector<FilamentInfo>*    ams_mapping         = nullptr,
    const TrayTagUidLookup&             tray_tag_uid_lookup = {});

}} // namespace Slic3r::GUI

#endif
