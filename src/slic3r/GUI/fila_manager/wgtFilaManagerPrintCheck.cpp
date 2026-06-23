#include "wgtFilaManagerPrintCheck.h"

#include <sstream>

namespace Slic3r { namespace GUI {

void FilamentInventoryCheckResult::merge(const FilamentInventoryCheckResult& other)
{
    shortages.insert(shortages.end(), other.shortages.begin(), other.shortages.end());
    unmapped_slot_count       += other.unmapped_slot_count;
    unknown_weight_slot_count += other.unknown_weight_slot_count;
}

double spool_remaining_grams(const FilamentSpool& spool)
{
    if (spool.net_weight > 0.0)
        return spool.net_weight;

    const double total_net = spool.effective_total_net_weight();
    if (total_net > 0.0 && spool.remain_percent >= 0)
        return total_net * static_cast<double>(spool.remain_percent) / 100.0;

    return -1.0;
}

std::string format_spool_display_label(const FilamentSpool& spool)
{
    std::ostringstream label;
    if (!spool.brand.empty())
        label << spool.brand;
    if (!spool.series.empty()) {
        if (label.tellp() > 0)
            label << ' ';
        label << spool.series;
    } else if (!spool.material_type.empty()) {
        if (label.tellp() > 0)
            label << ' ';
        label << spool.material_type;
    }

    const std::string& color = !spool.color_name.empty() ? spool.color_name : spool.color_code;
    if (!color.empty()) {
        if (label.tellp() > 0)
            label << " / ";
        label << color;
    }

    return label.str().empty() ? spool.spool_id : label.str();
}

std::map<size_t, double> compute_required_grams_per_filament(const GCodeProcessorResult& result)
{
    std::map<size_t, double> required_g_per_slot;
    const auto& ps        = result.print_statistics;
    const auto& densities = result.filament_densities;

    for (const auto& item : ps.total_volumes_per_extruder) {
        const size_t slot = item.first;
        if (slot >= densities.size())
            continue;
        required_g_per_slot[slot] = item.second * densities[slot] * 0.001;
    }

    return required_g_per_slot;
}

const FilamentSpool* resolve_spool_for_slot(
    const wgtFilaManagerStore& store, const FilamentSlotMapping& mapping)
{
    if (FilamentSpool::is_valid_tag_uid(mapping.tag_uid)) {
        if (const FilamentSpool* sp = store.find_by_tag_uid(mapping.tag_uid))
            return sp;
    }

    if (!mapping.setting_id.empty() && !mapping.color_code.empty())
        return store.find_by_setting_and_color(mapping.setting_id, mapping.color_code);

    return nullptr;
}

static const FilamentInfo* find_ams_mapping_entry(
    int filament_slot, const std::vector<FilamentInfo>* ams_mapping)
{
    if (!ams_mapping)
        return nullptr;
    for (const FilamentInfo& info : *ams_mapping) {
        if (info.id == filament_slot)
            return &info;
    }
    return nullptr;
}

std::vector<FilamentSlotMapping> build_filament_slot_mappings(
    const std::map<size_t, double>& required_g_per_slot,
    const std::vector<std::string>& filament_setting_ids,
    const std::vector<std::string>& filament_colours,
    const std::vector<FilamentInfo>* ams_mapping,
    const TrayTagUidLookup& tray_tag_uid_lookup)
{
    std::vector<FilamentSlotMapping> mappings;
    mappings.reserve(required_g_per_slot.size());

    for (const auto& item : required_g_per_slot) {
        const int slot = static_cast<int>(item.first);
        FilamentSlotMapping mapping;
        mapping.filament_slot = slot;

        if (slot >= 0 && slot < static_cast<int>(filament_setting_ids.size()))
            mapping.setting_id = filament_setting_ids[slot];
        if (slot >= 0 && slot < static_cast<int>(filament_colours.size()))
            mapping.color_code = filament_colours[slot];

        if (const FilamentInfo* info = find_ams_mapping_entry(slot, ams_mapping)) {
            if (tray_tag_uid_lookup && !info->ams_id.empty() && info->get_slot_id() >= 0)
                mapping.tag_uid = tray_tag_uid_lookup(info->ams_id, info->get_slot_id());
        }

        mappings.push_back(std::move(mapping));
    }

    return mappings;
}

FilamentInventoryCheckResult check_filament_inventory(
    const std::map<size_t, double>& required_g_per_slot,
    const wgtFilaManagerStore& store,
    const std::vector<FilamentSlotMapping>& slot_mappings)
{
    FilamentInventoryCheckResult result;

    for (const auto& item : required_g_per_slot) {
        const int slot       = static_cast<int>(item.first);
        const double required_g = item.second;

        const FilamentSlotMapping* mapping = nullptr;
        for (const FilamentSlotMapping& candidate : slot_mappings) {
            if (candidate.filament_slot == slot) {
                mapping = &candidate;
                break;
            }
        }
        if (!mapping) {
            ++result.unmapped_slot_count;
            continue;
        }

        const FilamentSpool* spool = resolve_spool_for_slot(store, *mapping);
        if (!spool) {
            ++result.unmapped_slot_count;
            continue;
        }

        const double remaining_g = spool_remaining_grams(*spool);
        if (remaining_g < 0.0) {
            ++result.unknown_weight_slot_count;
            continue;
        }

        if (remaining_g < required_g) {
            FilamentShortage shortage;
            shortage.filament_slot = slot;
            shortage.spool_id      = spool->spool_id;
            shortage.display_label = format_spool_display_label(*spool);
            shortage.required_g    = required_g;
            shortage.remaining_g   = remaining_g;
            shortage.short_by_g    = required_g - remaining_g;
            result.shortages.push_back(std::move(shortage));
        }
    }

    return result;
}

FilamentInventoryCheckResult check_slice_result_inventory(
    const GCodeProcessorResult& result,
    const wgtFilaManagerStore& store,
    const std::vector<std::string>& filament_setting_ids,
    const std::vector<std::string>& filament_colours,
    const std::vector<FilamentInfo>* ams_mapping,
    const TrayTagUidLookup& tray_tag_uid_lookup)
{
    const auto required_g_per_slot = compute_required_grams_per_filament(result);
    const auto slot_mappings = build_filament_slot_mappings(
        required_g_per_slot, filament_setting_ids, filament_colours, ams_mapping, tray_tag_uid_lookup);
    return check_filament_inventory(required_g_per_slot, store, slot_mappings);
}

}} // namespace Slic3r::GUI
