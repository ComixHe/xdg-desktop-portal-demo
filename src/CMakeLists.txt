set(portal-demo_SRCS main.cpp xdgexporterv2.h xdgexporterv2.cpp portaldemo.h
                     portaldemo.cpp)

pkg_get_variable(portal_interfaces_dir xdg-desktop-portal interfaces_dir)
qt_add_dbus_interface(
  portal-demo_SRCS ${portal_interfaces_dir}/org.freedesktop.portal.Request.xml
  org_freedesktop_portal_Request)
add_executable(portal-demo ${portal-demo_SRCS})

pkg_get_variable(protocols_dir wayland-protocols pkgdatadir)
qt_generate_wayland_protocol_client_sources(
  portal-demo FILES
  ${protocols_dir}/unstable/xdg-foreign/xdg-foreign-unstable-v2.xml)

target_link_libraries(
  portal-demo
  Qt::Core
  Qt::DBus
  Qt::Widgets
  Qt::GuiPrivate
  Qt::WaylandClient
  Wayland::Client
  PkgConfig::GSTREAMER)

include(GNUInstallDirs)
install(TARGETS portal-demo DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
