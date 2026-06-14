#include "wgtFilaManagerSync.h"
#include "wgtFilaManagerStore.h"

#include "slic3r/GUI/DeviceCore/DevFilaSystem.h"
#include "slic3r/GUI/DeviceManager.hpp"

#include <boost/log/trivial.hpp>

#include <cmath>

namespace Slic3r { namespace GUI {

wgtFilaManagerSync::wgtFilaManagerSync(wgtFilaManagerStore* store)
    : m_store(store)
{}

void wgtFilaManagerSync::on_device_update(MachineObject* obj)
{
    if (!obj || !m_store) return;
    sync_all_trays(obj);
}

void wgtFilaManagerSync::sync_all_trays(MachineObject* obj)
{
    if (!obj || !m_store) return;

    auto fila_sys = obj->GetFilaSystem();
    if (!fila_sys) return;

    const std::string dev_id = obj->get_dev_id();
    bool any_changed         = false;

    auto handle_tray = [&](const DevAmsTray& tray, const std::string& ams_id) {
        if (tray.setting_id.empty() && tray.tag_uid.empty()) return;

        const FilamentSpool* matched = match_tray(tray);
        if (!matched) {
            BOOST_LOG_TRIVIAL(trace)
                << "[ams-sync] unmatched tray, skip auto-add"
                << " setting_id=" << tray.setting_id
                << " tag_uid="    << tray.tag_uid;
            return;
        }

        const double total_nw = matched->effective_total_net_weight();
        if (total_nw <= 0.0) {
            BOOST_LOG_TRIVIAL(trace)
                << "[ams-sync] frozen spool, no total_net_weight"
                << " spool_id=" << matched->spool_id;
            return;
        }

        FilamentSpool updated  = *matched;
        const int64_t net_weight_g =
            static_cast<int64_t>(std::round(total_nw * tray.remain / 100.0));
        updated.net_weight     = static_cast<double>(net_weight_g);
        updated.remain_percent = tray.remain;
        updated.status         = (tray.remain == 0)  ? "empty"
                              : (tray.remain < 20)   ? "low" : "active";
        updated.bound_dev_id   = dev_id;
        updated.bound_ams_id   = ams_id;

        if (m_store->update_spool_if_changed(updated))
            any_changed = true;
    };

    for (auto& [ams_id, ams] : fila_sys->GetAmsList()) {
        if (!ams) continue;
        for (auto& [slot_id, tray] : ams->GetTrays()) {
            if (tray) handle_tray(*tray, ams_id);
        }
    }
    for (auto& vt_tray : obj->vt_slot) {
        handle_tray(vt_tray, "ext");
    }

    if (any_changed)
        m_store->set_dirty();

    // Phase 3: notify cloud sync (notify_ams_synced) after local AMS writes.
}

const FilamentSpool* wgtFilaManagerSync::match_tray(const DevAmsTray& tray)
{
    if (!tray.tag_uid.empty()) {
        auto* sp = m_store->find_by_tag_uid(tray.tag_uid);
        if (sp) return sp;
    }
    if (!tray.setting_id.empty()) {
        auto* sp = m_store->find_by_setting_and_color(tray.setting_id, tray.color);
        if (sp) return sp;
    }
    return nullptr;
}

}} // namespace Slic3r::GUI
