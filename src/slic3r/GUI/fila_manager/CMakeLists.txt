# GUI/fila_manager
# Filament Manager — local spool inventory (Phase 1: store only)

list(APPEND SLIC3R_GUI_SOURCES
    GUI/fila_manager/wgtFilaManagerStore.h
    GUI/fila_manager/wgtFilaManagerStore.cpp
)
set(SLIC3R_GUI_SOURCES ${SLIC3R_GUI_SOURCES} PARENT_SCOPE)
