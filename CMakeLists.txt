cmake_minimum_required(VERSION 3.0)
project(klookandfeel)
# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_lookandfeel\")

configure_file (config-kcm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kcm.h )

set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.54.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(CheckIncludeFiles)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    QuickWidgets
    DBus
    Widgets
    X11Extras
    Svg
    Concurrent
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Auth
    Plasma
    PlasmaQuick
    DocTools
    I18n
    KCMUtils
    NewStuff
    KDELibs4Support
    Notifications
    NotifyConfig
    Attica
    Wallet
    Runner
    GlobalAccel
    Declarative
    DBusAddons
    Config
)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)


set(kcm_lookandfeel_SRCS
  kcmmain.cpp
  kcm.cpp
  #../krdb/krdb.cpp
)

set(klauncher_xml ${KINIT_DBUS_INTERFACES_DIR}/kf5_org.kde.KLauncher.xml)
qt5_add_dbus_interface(kcm_lookandfeel_SRCS ${klauncher_xml} klauncher_iface)

add_library(kcm_lookandfeel MODULE ${kcm_lookandfeel_SRCS})

target_link_libraries(kcm_lookandfeel
  KF5::KIOWidgets
  KF5::CoreAddons
  KF5::KCMUtils
  KF5::I18n
  KF5::Plasma
  KF5::PlasmaQuick
  KF5::KDELibs4Support
  KF5::Declarative
  KF5::QuickAddons
  KF5::NewStuff

  Qt5::DBus
  Qt5::Widgets
  Qt5::QuickWidgets
  Qt5::X11Extras
)

if(X11_FOUND)
    target_link_libraries(kcm_lookandfeel Qt5::X11Extras)
endif()
if (X11_Xfixes_FOUND)
   target_link_libraries(kcm_lookandfeel ${X11_Xfixes_LIB})
endif ()

kcoreaddons_desktop_to_json(kcm_lookandfeel "kcm_lookandfeel.desktop" SERVICE_TYPES kcmodule.desktop)

#this desktop file is installed only for retrocompatibility with sycoca
install(FILES kcm_lookandfeel.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS kcm_lookandfeel DESTINATION ${KDE_INSTALL_PLUGINDIR}/kcms)
install(FILES lookandfeel.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})

###### Command line tool

set(lookandfeeltool_SRCS
    lnftool.cpp
    # TODO: load kcm plugin instead of using code copy
    kcm.cpp
    #../krdb/krdb.cpp
)

qt5_add_dbus_interface(lookandfeeltool_SRCS ${klauncher_xml} klauncher_iface)

add_executable(lookandfeeltool ${lookandfeeltool_SRCS})

target_link_libraries(lookandfeeltool
  KF5::KIOWidgets
  KF5::CoreAddons
  KF5::KCMUtils
  KF5::I18n
  #TODO:kpackage
  KF5::Plasma
  KF5::PlasmaQuick
  KF5::KDELibs4Support
  KF5::Declarative
  KF5::QuickAddons
  KF5::NewStuff

  Qt5::DBus
  Qt5::Widgets
  Qt5::QuickWidgets
  ${X11_LIBRARIES}
)
kpackage_install_package(package kcm_lookandfeel kcms)


if(X11_FOUND)
    target_link_libraries(lookandfeeltool Qt5::X11Extras)
endif()
if (X11_Xfixes_FOUND)
   target_link_libraries(lookandfeeltool ${X11_Xfixes_LIB})
endif ()

install( TARGETS lookandfeeltool ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

