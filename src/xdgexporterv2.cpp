#include "xdgexporterv2.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPointer>
#include <QWidget>
#include <qpa/qplatformnativeinterface.h>

class WidgetWatcher : public QObject {
public:
  WidgetWatcher(XDGExporterV2 *exporter, XDGExportedV2 *toExport,
                QWidget *theWidget)
      : QObject(theWidget), m_toExport(toExport), m_widget(theWidget),
        m_exporter(exporter) {
    theWidget->installEventFilter(this);
  }

  ~WidgetWatcher() { delete m_toExport; }

  bool eventFilter(QObject *watched, QEvent *event) override {
    Q_ASSERT(watched == parent());
    if (event->type() == QEvent::PlatformSurface) {
      m_toExport->setWindow(m_widget->windowHandle());
    }
    return false;
  }

  QPointer<XDGExportedV2> m_toExport;
  QWidget *const m_widget;
  XDGExporterV2 *const m_exporter;
};

XDGExportedV2 *XDGExporterV2::exportWindow(QWindow *window) {
  if (window == nullptr) {
    qCritical() << "no window!";
    return nullptr;
  }

  auto *exported = new XDGExportedV2(this);
  exported->setWindow(window);
  return exported;
}

XDGExportedV2 *XDGExporterV2::exportWidget(QWidget *widget) {
  if (widget->windowHandle() != nullptr) {
    return exportWindow(widget->windowHandle());
  }

  auto *nakedExporter = new XDGExportedV2(this);
  new WidgetWatcher(this, nakedExporter, widget);
  return nakedExporter;
}

void XDGExportedV2::setWindow(QWindow *window) {
  Q_ASSERT(window);

  useWindow(window);
  connect(window, &QWindow::visibilityChanged, this,
          [this, window](QWindow::Visibility visibility) {
            if (visibility != QWindow::Hidden) {
              useWindow(window);
            }
          });
  connect(window, &QWindow::destroyed, this, &QObject::deleteLater);
}

void XDGExportedV2::useWindow(QWindow *window) {
  QPlatformNativeInterface *nativeInterface =
      qGuiApp->platformNativeInterface();
  auto surface = static_cast<wl_surface *>(
      nativeInterface->nativeResourceForWindow("surface", window));
  if (surface) {
    auto tl = m_exporter->export_toplevel(surface);
    if (tl) {
      init(tl);
    } else {
      qDebug() << "could not export top level";
    }
  } else {
    qDebug() << "could not get surface";
  }
}

std::optional<QString> XDGExportedV2::handle() const { return m_handle; }
