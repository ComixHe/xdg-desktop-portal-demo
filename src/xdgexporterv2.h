#pragma once

#include "qwayland-xdg-foreign-unstable-v2.h"
#include <QDebug>
#include <QWaylandClientExtensionTemplate>
#include <QWindow>

class XDGExportedV2;

class XDGExporterV2 : public QWaylandClientExtensionTemplate<XDGExporterV2>,
                      public QtWayland::zxdg_exporter_v2 {
  Q_OBJECT
public:
  XDGExporterV2()
      : QWaylandClientExtensionTemplate(
            ZXDG_EXPORTER_V2_DESTROY_SINCE_VERSION) {
    initialize();
    if (!isInitialized()) {
      qWarning() << "Failed to initialize XDGExporterV2";
    }

    Q_ASSERT(isInitialized());
  }

  ~XDGExporterV2() override { destroy(); }

  XDGExportedV2 *exportWindow(QWindow *window);
  XDGExportedV2 *exportWidget(QWidget *widget);
};

class XDGExportedV2 : public QObject, public QtWayland::zxdg_exported_v2 {
  Q_OBJECT
public:
  explicit XDGExportedV2(XDGExporterV2 *exporter) : m_exporter(exporter) {}

  ~XDGExportedV2() override { destroy(); }

  [[nodiscard]] std::optional<QString> handle() const;
  void setWindow(QWindow *window);

private:
  void zxdg_exported_v2_handle(const QString &handle) override {
    m_handle = handle;
  }

  void useWindow(QWindow *window);
  std::optional<QString> m_handle;
  XDGExporterV2 *m_exporter;
};
