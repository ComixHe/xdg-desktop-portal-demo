#pragma once
#include "xdgexporterv2.h"
#include <QDBusUnixFileDescriptor>
#include <QMainWindow>
#include <QPointer>

class PortalDemo : public QMainWindow {
  Q_OBJECT
public:
  PortalDemo();

public Q_SLOTS:
  void requestScreenShot();
  void gotResponseScreenShot(uint32_t response, const QVariantMap &results);

  void requestAccessCamera();
  void gotResponseAccessCamera(uint32_t response, const QVariantMap &results);

  int requestOpenPipeWireRemote();

  // bool requestDeviceAccess(const QStringList &devices);

private:
  [[nodiscard]] QString parentWindowId() const;
  [[nodiscard]] QString getRequestToken() {
    m_requestTokenCounter += 1;
    return QString{"token_%1"}.arg(m_requestTokenCounter);
  }

  QScopedPointer<XDGExporterV2> m_xdgExporter;
  QPointer<XDGExportedV2> m_xdgExported;
  uint m_requestTokenCounter{0};
};
