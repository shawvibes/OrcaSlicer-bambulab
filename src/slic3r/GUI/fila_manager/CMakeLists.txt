# GUI/fila_manager
# Filament Manager — local spool inventory and AMS sync

list(APPEND SLIC3R_GUI_SOURCES
    GUI/fila_manager/wgtFilaManagerStore.h
    GUI/fila_manager/wgtFilaManagerStore.cpp
    GUI/fila_manager/wgtFilaManagerSync.h
    GUI/fila_manager/wgtFilaManagerSync.cpp
)
set(SLIC3R_GUI_SOURCES ${SLIC3R_GUI_SOURCES} PARENT_SCOPE)
