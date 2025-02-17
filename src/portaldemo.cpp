#include "portaldemo.h"
#include <QComboBox>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDesktopServices>
#include <QGridLayout>
#include <QGuiApplication>
#include <QPushButton>
#include <QUrl>
#include <QWidget>
#include <fcntl.h>
#include <gstreamer-1.0/gst/gst.h>
#include <qdbusextratypes.h>
#include <qdbuspendingreply.h>

inline static const char *desktopPortalService() {
  return "org.freedesktop.portal.Desktop";
}
inline static const char *desktopPortalPath() {
  return "/org/freedesktop/portal/desktop";
}
inline static const char *portalRequestInterface() {
  return "org.freedesktop.portal.Request";
}

inline static const char *portalCameraInterface() {
  return "org.freedesktop.portal.Camera";
}

inline static const char *portalResponseSignal() { return "Response"; }

QString PortalDemo::parentWindowId() const {
  auto name = QGuiApplication::platformName();
  if (name == QLatin1String("wayland")) {
    return QString{"wayland:%1"}.arg(*m_xdgExported->handle());
  }

  if (name == QLatin1String("xcb")) {
    return QString{"x11:%1"}.arg(winId());
  }

  return "";
}

PortalDemo::PortalDemo()
    : QMainWindow(nullptr), m_xdgExporter(new XDGExporterV2),
      m_xdgExported(m_xdgExporter->exportWidget(this)) {
  auto *centralWidget = new QWidget(this);
  auto *vert = new QVBoxLayout(centralWidget);

  auto *shotButton = new QPushButton();
  shotButton->setText("Request screenshot");
  connect(shotButton, &QPushButton::clicked, this,
          &PortalDemo::requestScreenShot);
  vert->addWidget(shotButton);

  auto *camButton = new QPushButton();
  camButton->setText("Request Camera");
  connect(camButton, &QPushButton::clicked, this,
          &PortalDemo::requestAccessCamera);
  vert->addWidget(camButton);

  auto *hori = new QHBoxLayout();
  auto *box = new QComboBox();
  box->addItem("camera");
  box->addItem("speaker");
  box->addItem("microphone");
  auto *reqBtn = new QPushButton();
  reqBtn->setText("Request Device Access");
  connect(reqBtn, &QPushButton::clicked, [box, this] {
    auto devices = box->currentText();
    requestDeviceAccess({devices});
  });

  hori->addWidget(box);
  hori->addWidget(reqBtn);
  vert->addLayout(hori);

  setCentralWidget(centralWidget);

  gst_init(nullptr, nullptr);
}

void PortalDemo::requestScreenShot() {
  QDBusMessage message = QDBusMessage::createMethodCall(
      desktopPortalService(), desktopPortalPath(),
      QLatin1String("org.freedesktop.portal.Screenshot"),
      QLatin1String("Screenshot"));

  auto winID = parentWindowId();
  qInfo() << "parent_window:" << winID;
  auto token = getRequestToken();
  message.setArguments({winID, QVariantMap{{QLatin1String("interactive"), true},
                                           {"handle_token", token}}});
  auto ret = QDBusConnection::sessionBus().asyncCall(message);

  auto *watcher = new QDBusPendingCallWatcher(ret);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this, token](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            if (reply.isError()) {
              qCritical() << "Error: " << watcher->error().message();
              return;
            }
            qInfo() << "request path:" << reply.value().path();

            QDBusConnection::sessionBus().connect(
                desktopPortalService(), reply.value().path(),
                portalRequestInterface(), portalResponseSignal(), this,
                SLOT(gotResponseScreenShot(uint32_t, QVariantMap)));
          });
}

void PortalDemo::gotResponseScreenShot(uint32_t response,
                                       const QVariantMap &results) {
  if (response == 1) {
    qInfo() << "user cancelled";
    return;
  }

  if (response == 2) {
    qInfo() << "ended in some other way";
    return;
  }

  auto uri = results["uri"];
  if (!uri.isValid()) {
    qCritical() << "invalid uri";
    return;
  }

  qInfo() << "screenShot under" << uri.toString();
  if (!QDesktopServices::openUrl(uri.toUrl())) {
    qCritical() << "failed to open url:" << uri.toString();
  }
}

void PortalDemo::requestAccessCamera() {
  // read properties

  QDBusMessage message = QDBusMessage::createMethodCall(
      desktopPortalService(), desktopPortalPath(),
      "org.freedesktop.DBus.Properties", "Get");
  message.setArguments({portalCameraInterface(), "IsCameraPresent"});
  QDBusPendingReply<QVariant> reply =
      QDBusConnection::sessionBus().asyncCall(message);
  reply.waitForFinished();
  if (reply.isError()) {
    qCritical() << "failed to get property IsCameraPresent";
    return;
  }

  if (!reply.value().toBool()) {
    qCritical() << "no camera.";
    return;
  }

  qInfo() << "camera check pass";
  // access
  auto msg = QDBusMessage::createMethodCall(
      desktopPortalService(), desktopPortalPath(), portalCameraInterface(),
      "AccessCamera");
  msg.setArguments({QVariantMap{{"handle_token", getRequestToken()}}});
  auto ret = QDBusConnection::sessionBus().asyncCall(msg);
  auto *watcher = new QDBusPendingCallWatcher(ret);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            if (reply.isError()) {
              qCritical() << "Error: " << watcher->error().message();
              return;
            }
            qInfo() << "request path:" << reply.value().path();

            QDBusConnection::sessionBus().connect(
                desktopPortalService(), reply.value().path(),
                portalRequestInterface(), portalResponseSignal(), this,
                SLOT(gotResponseAccessCamera(uint32_t, QVariantMap)));
          });
}

void PortalDemo::gotResponseAccessCamera(
    uint32_t response, [[maybe_unused]] const QVariantMap &results) {
  if (response == 1) {
    qInfo() << "user cancelled";
    return;
  }

  if (response == 2) {
    qInfo() << "ended in some other way";
    return;
  }

  // permission granted
  auto pipeWireFD = requestOpenPipeWireRemote();
  if (pipeWireFD == -1) {
    return;
  }

  qInfo() << "open pipewire remote successfully";
  g_autoptr(GError) err{nullptr};
  QString gstLaunch =
      QString("pipewiresrc fd=%1 ! videoconvert ! xvimagesink").arg(pipeWireFD);
  auto *element = gst_parse_launch(gstLaunch.toUtf8(), &err);
  if (element == nullptr) {
    qCritical() << "failed to launch:" << err->message;
    ::close(pipeWireFD);
    return;
  }

  auto ret = gst_element_set_state(element, GST_STATE_PLAYING);
  switch (ret) {
  case GST_STATE_CHANGE_FAILURE:
    qCritical() << "failed to play";
    ::close(pipeWireFD);
    return;
  case GST_STATE_CHANGE_SUCCESS:
    qInfo() << "play success";
    break;
  case GST_STATE_CHANGE_ASYNC:
    qInfo() << "play async";
    break;
  case GST_STATE_CHANGE_NO_PREROLL:
    qInfo() << "play no preroll";
    break;
  }
}

void PortalDemo::requestDeviceAccess(const QStringList &devices) {
  QDBusMessage message = QDBusMessage::createMethodCall(
      desktopPortalService(), desktopPortalPath(),
      "org.freedesktop.portal.Device", "AccessDevice");
  auto token = getRequestToken();
  message.setArguments({static_cast<uint32_t>(::getpid()), devices,
                        QVariantMap{{"handle_token", token}}});
  auto ret = QDBusConnection::sessionBus().asyncCall(message);
  auto *watcher = new QDBusPendingCallWatcher(ret);
  connect(watcher, &QDBusPendingCallWatcher::finished, this,
          [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            if (reply.isError()) {
              qCritical() << "Error: " << watcher->error().message();
              return;
            }
            qInfo() << "request path:" << reply.value().path();

            QDBusConnection::sessionBus().connect(
                desktopPortalService(), reply.value().path(),
                portalRequestInterface(), portalResponseSignal(), this,
                SLOT(gotResponseDeviceAccess(uint32_t, QVariantMap)));
          });
}

void PortalDemo::gotResponseDeviceAccess(uint32_t response,
                                         [[maybe_unused]] QVariantMap result) {
  if (response == 1) {
    qInfo() << "user cancelled";
    return;
  }

  if (response == 2) {
    qInfo() << "ended in some other way";
    return;
  }

  qInfo() << "device access granted";
}

int PortalDemo::requestOpenPipeWireRemote() {
  auto message = QDBusMessage::createMethodCall(
      desktopPortalService(), desktopPortalPath(), portalCameraInterface(),
      "OpenPipeWireRemote");
  message.setArguments({QVariantMap{}});
  QDBusPendingReply<QDBusUnixFileDescriptor> reply =
      QDBusConnection::sessionBus().call(message);
  if (reply.isError()) {
    qCritical() << "failed to open pipewire remote";
    return -1;
  }

  auto fd = ::dup(reply.value().fileDescriptor());
  if (fd == -1) {
    qCritical() << "dup failed:" << ::strerror(errno);
    return -1;
  }

  return fd;
}
