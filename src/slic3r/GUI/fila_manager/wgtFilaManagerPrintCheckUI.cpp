#include "wgtFilaManagerPrintCheckUI.h"

#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/MsgDialog.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/format.hpp"

#include "slic3r/GUI/DeviceCore/DevDefs.h"
#include "slic3r/GUI/DeviceManager.hpp"

namespace Slic3r { namespace GUI {

TrayTagUidLookup make_tray_tag_uid_lookup(MachineObject* obj)
{
    return [obj](const std::string& ams_id, int slot_id) -> std::string {
        if (!obj)
            return {};

        const std::string tray_id = std::to_string(slot_id);
        if (obj->contains_tray(ams_id, tray_id))
            return obj->get_tray(ams_id, tray_id).tag_uid;

        if (ams_id == VIRTUAL_AMS_MAIN_ID_STR || ams_id == VIRTUAL_AMS_DEPUTY_ID_STR) {
            for (const auto& tray : obj->vt_slot) {
                if (tray.id == ams_id)
                    return tray.tag_uid;
            }
        }

        return {};
    };
}

void extract_filament_identity_from_config(
    const DynamicPrintConfig& config,
    std::vector<std::string>& filament_setting_ids,
    std::vector<std::string>& filament_colours)
{
    if (const auto* ids = config.option<ConfigOptionStrings>("filament_ids"))
        filament_setting_ids = ids->values;
    if (const auto* colours = config.option<ConfigOptionStrings>("filament_colour"))
        filament_colours = colours->values;
}

static FilamentInventoryCheckResult check_slice_result_for_plate(
    const GCodeProcessorResult&       result,
    int                               plate_index,
    const DynamicPrintConfig&         config,
    const wgtFilaManagerStore&        store,
    const std::vector<FilamentInfo>*  ams_mapping,
    MachineObject*                    obj)
{
    std::vector<std::string> setting_ids;
    std::vector<std::string> colours;
    extract_filament_identity_from_config(config, setting_ids, colours);

    FilamentInventoryCheckResult check = check_slice_result_inventory(
        result, store, setting_ids, colours, ams_mapping, make_tray_tag_uid_lookup(obj));

    if (plate_index >= 0) {
        for (FilamentShortage& shortage : check.shortages)
            shortage.plate_index = plate_index;
    }

    return check;
}

FilamentInventoryCheckResult check_part_plate_inventory(
    PartPlate*                      plate,
    const DynamicPrintConfig&       config,
    const wgtFilaManagerStore&      store,
    const std::vector<FilamentInfo>* ams_mapping,
    MachineObject*                  obj)
{
    if (!plate || !plate->is_slice_result_valid())
        return {};

    const GCodeProcessorResult* slice = plate->get_slice_result();
    if (!slice)
        return {};

    return check_slice_result_for_plate(*slice, plate->get_index(), config, store, ams_mapping, obj);
}

FilamentInventoryCheckResult check_print_plates_inventory(
    Plater*                         plater,
    int                             print_plate_idx,
    const DynamicPrintConfig&       config,
    const wgtFilaManagerStore&      store,
    const std::vector<FilamentInfo>* ams_mapping,
    MachineObject*                  obj)
{
    FilamentInventoryCheckResult aggregated;
    if (!plater)
        return aggregated;

    PartPlateList& plate_list = plater->get_partplate_list();
    if (print_plate_idx == PLATE_ALL_IDX) {
        for (int i = 0; i < plate_list.get_plate_count(); ++i) {
            PartPlate* plate = plate_list.get_plate(i);
            if (!plate || !plate->is_slice_result_valid())
                continue;
            aggregated.merge(check_part_plate_inventory(plate, config, store, ams_mapping, obj));
        }
    } else {
        PartPlate* plate = (print_plate_idx >= 0)
            ? plate_list.get_plate(print_plate_idx)
            : plate_list.get_curr_plate();
        aggregated = check_part_plate_inventory(plate, config, store, ams_mapping, obj);
    }

    return aggregated;
}

static wxString format_grams(double grams)
{
    return wxString::Format("%.1f g", grams);
}

wxString build_low_filament_warning_message(
    const FilamentInventoryCheckResult& result,
    bool                                is_estimate,
    const wxString&                     context_label)
{
    wxString message;
    if (is_estimate)
        message += _L("Based on the previous slice; usage may change after re-slicing.") + "\n\n";

    message += wxString::Format(
        _L("The following spool(s) may not have enough filament for %s:") + "\n\n",
        context_label);

    for (const FilamentShortage& shortage : result.shortages) {
        wxString header;
        if (shortage.plate_index >= 0)
            header = wxString::Format(
                _L("• Plate %d, Filament %d — %s"),
                shortage.plate_index + 1,
                shortage.filament_slot + 1,
                wxString::FromUTF8(shortage.display_label.c_str()));
        else
            header = wxString::Format(
                _L("• Filament %d — %s"),
                shortage.filament_slot + 1,
                wxString::FromUTF8(shortage.display_label.c_str()));

        message += header + "\n  "
            + wxString::Format(
                  _L("Needs %s · Spool has %s · Short %s"),
                  format_grams(shortage.required_g),
                  format_grams(shortage.remaining_g),
                  format_grams(shortage.short_by_g))
            + "\n\n";
    }

    if (result.unmapped_slot_count > 0) {
        message += wxString::Format(
            _L_PLURAL("%d filament slot could not be matched to inventory and was not checked.",
                      "%d filament slots could not be matched to inventory and were not checked.",
                      result.unmapped_slot_count),
            result.unmapped_slot_count);
    }

    return message;
}

bool show_low_filament_warning_dialog(
    wxWindow*                           parent,
    const FilamentInventoryCheckResult& result,
    bool                                is_estimate,
    const wxString&                     context_label)
{
    struct LowFilamentWarningDialog : MessageDialog
    {
        LowFilamentWarningDialog(wxWindow* dialog_parent, const wxString& text)
            : MessageDialog(dialog_parent, text, _L("Low Filament Warning"), 0)
        {
            add_button(wxID_NO, false, _L("Cancel"));
            add_button(wxID_YES, true, _L("Continue anyway"));
        }
    };

    LowFilamentWarningDialog dialog(parent, build_low_filament_warning_message(result, is_estimate, context_label));
    dialog.Fit();
    return dialog.ShowModal() == wxID_YES;
}

void push_low_filament_slice_notification(
    NotificationManager*                notification_manager,
    const FilamentInventoryCheckResult& result,
    int                                 plate_index)
{
    if (!notification_manager || !result.has_shortage())
        return;

    const wxString context = wxString::Format(_L("Plate %d"), plate_index + 1);
    const wxString text    = build_low_filament_warning_message(result, false, context);

    notification_manager->push_notification(
        NotificationType::CustomNotification,
        NotificationManager::NotificationLevel::WarningNotificationLevel,
        into_u8(text));
}

bool confirm_print_filament_inventory(
    wxWindow*                        parent,
    Plater*                          plater,
    int                              print_plate_idx,
    const DynamicPrintConfig&        config,
    const std::vector<FilamentInfo>* ams_mapping,
    MachineObject*                   obj)
{
    wgtFilaManagerStore* store = wxGetApp().fila_manager_store();
    if (!store || !plater)
        return true;

    const FilamentInventoryCheckResult result =
        check_print_plates_inventory(plater, print_plate_idx, config, *store, ams_mapping, obj);

    if (!result.has_shortage())
        return true;

    const wxString context = (print_plate_idx == PLATE_ALL_IDX)
        ? _L("this project")
        : wxString::Format(_L("Plate %d"),
                           (print_plate_idx >= 0 ? print_plate_idx : plater->get_partplate_list().get_curr_plate_index()) + 1);

    return show_low_filament_warning_dialog(parent, result, false, context);
}

bool confirm_pre_slice_filament_inventory(
    wxWindow*                 parent,
    Plater*                   plater,
    bool                      slice_all,
    const DynamicPrintConfig& config)
{
    wgtFilaManagerStore* store = wxGetApp().fila_manager_store();
    if (!store || !plater)
        return true;

    PartPlateList& plate_list = plater->get_partplate_list();
    FilamentInventoryCheckResult aggregated;

    if (slice_all) {
        for (int i = 0; i < plate_list.get_plate_count(); ++i) {
            PartPlate* plate = plate_list.get_plate(i);
            if (!plate || !plate->is_slice_result_valid())
                continue;
            aggregated.merge(check_part_plate_inventory(plate, config, *store, nullptr, nullptr));
        }
    } else {
        PartPlate* plate = plate_list.get_curr_plate();
        if (!plate || !plate->is_slice_result_valid())
            return true;
        aggregated = check_part_plate_inventory(plate, config, *store, nullptr, nullptr);
    }

    if (!aggregated.has_shortage())
        return true;

    const wxString context = slice_all ? _L("this project") : wxString::Format(_L("Plate %d"), plate_list.get_curr_plate_index() + 1);
    return show_low_filament_warning_dialog(parent, aggregated, true, context);
}

void notify_post_slice_filament_inventory(
    NotificationManager*      notification_manager,
    PartPlate*                plate,
    const DynamicPrintConfig& config)
{
    wgtFilaManagerStore* store = wxGetApp().fila_manager_store();
    if (!store || !notification_manager || !plate || !plate->is_slice_result_valid())
        return;

    const FilamentInventoryCheckResult result = check_part_plate_inventory(plate, config, *store, nullptr, nullptr);
    if (!result.has_shortage())
        return;

    push_low_filament_slice_notification(notification_manager, result, plate->get_index());
}

}} // namespace Slic3r::GUI
