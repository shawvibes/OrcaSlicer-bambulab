#ifndef slic3r_wgtFilaManagerPrintCheckUI_h_
#define slic3r_wgtFilaManagerPrintCheckUI_h_

#include <vector>

#include "libslic3r/PrintConfig.hpp"
#include "wgtFilaManagerPrintCheck.h"

class wxString;
class wxWindow;

namespace Slic3r {
class MachineObject;
struct FilamentInfo;

namespace GUI {

class NotificationManager;
class PartPlate;
class Plater;

TrayTagUidLookup make_tray_tag_uid_lookup(MachineObject* obj);

void extract_filament_identity_from_config(
    const DynamicPrintConfig&       config,
    std::vector<std::string>&       filament_setting_ids,
    std::vector<std::string>&       filament_colours);

FilamentInventoryCheckResult check_part_plate_inventory(
    PartPlate*                      plate,
    const DynamicPrintConfig&       config,
    const wgtFilaManagerStore&      store,
    const std::vector<FilamentInfo>* ams_mapping = nullptr,
    MachineObject*                  obj         = nullptr);

FilamentInventoryCheckResult check_print_plates_inventory(
    Plater*                         plater,
    int                             print_plate_idx,
    const DynamicPrintConfig&       config,
    const wgtFilaManagerStore&      store,
    const std::vector<FilamentInfo>* ams_mapping = nullptr,
    MachineObject*                  obj         = nullptr);

wxString build_low_filament_warning_message(
    const FilamentInventoryCheckResult& result,
    bool                                is_estimate,
    const wxString&                     context_label);

// Returns true when the user chooses to continue.
bool show_low_filament_warning_dialog(
    wxWindow*                           parent,
    const FilamentInventoryCheckResult& result,
    bool                                is_estimate,
    const wxString&                     context_label);

void push_low_filament_slice_notification(
    NotificationManager*                notification_manager,
    const FilamentInventoryCheckResult& result,
    int                                 plate_index);

bool confirm_print_filament_inventory(
    wxWindow*                           parent,
    Plater*                             plater,
    int                                 print_plate_idx,
    const DynamicPrintConfig&           config,
    const std::vector<FilamentInfo>*    ams_mapping,
    MachineObject*                      obj);

bool confirm_pre_slice_filament_inventory(
    wxWindow*                 parent,
    Plater*                   plater,
    bool                      slice_all,
    const DynamicPrintConfig& config);

void notify_post_slice_filament_inventory(
    NotificationManager*      notification_manager,
    PartPlate*                plate,
    const DynamicPrintConfig& config);

}} // namespace Slic3r::GUI

#endif
