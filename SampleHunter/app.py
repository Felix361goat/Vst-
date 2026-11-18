#!/usr/bin/env python3
"""SampleHunter as a desktop app: its own window, and samples you can drag
straight into FL Studio (or any DAW, or Explorer/Finder).

    python app.py

A browser page cannot hand a real file to another program; this window can.
It shows the same library as serve.py, and when you drag a sample out of it,
it starts a native file drag, exactly as if the file came from Explorer.

Needs PySide6 (pip install -r requirements-app.txt). Without it, start
serve.py instead: everything works there except dragging into the DAW.
"""

from __future__ import annotations

import os
import socket
import sys
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from PySide6.QtCore import QFile, QIODevice, QMimeData, QObject, QUrl, Qt, Slot  # noqa: E402
from PySide6.QtGui import QColor, QDrag, QIcon, QPainter, QPixmap  # noqa: E402
from PySide6.QtWebChannel import QWebChannel  # noqa: E402
from PySide6.QtWebEngineCore import QWebEnginePage, QWebEngineScript  # noqa: E402
from PySide6.QtWebEngineWidgets import QWebEngineView  # noqa: E402
from PySide6.QtWidgets import QApplication, QMainWindow  # noqa: E402

import serve  # noqa: E402


def free_port(preferred: int = 8765) -> int:
    for port in (preferred, 0):
        with socket.socket() as s:
            try:
                s.bind(("127.0.0.1", port))
                return s.getsockname()[1]
            except OSError:
                continue
    raise RuntimeError("kein freier Port")


class Bridge(QObject):
    """What the page may ask the app to do."""

    def __init__(self, view: QWebEngineView):
        super().__init__()
        self.view = view

    @Slot(str, str)
    def drag(self, sample_id: str, fmt: str) -> None:
        path = serve.export_path(sample_id, fmt)
        if not path:
            return
        data = QMimeData()
        data.setUrls([QUrl.fromLocalFile(path)])
        drag = QDrag(self.view)
        drag.setMimeData(data)
        drag.setPixmap(_drag_pixmap(os.path.basename(path)))
        if os.environ.get("SH_TEST_DRAG"):      # headless test: report instead of dragging
            print("DRAG", path, flush=True)
            return
        drag.exec(Qt.CopyAction)

    @Slot(result=bool)
    def native(self) -> bool:
        return True


def _drag_pixmap(name: str) -> QPixmap:
    pm = QPixmap(260, 34)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setBrush(QColor("#ff4d6d"))
    p.setPen(Qt.NoPen)
    p.drawRoundedRect(0, 0, 260, 34, 8, 8)
    p.setPen(QColor("white"))
    p.drawText(pm.rect().adjusted(10, 0, -10, 0), Qt.AlignVCenter | Qt.AlignLeft,
               p.fontMetrics().elidedText(name, Qt.ElideMiddle, 240))
    p.end()
    return pm


def _icon() -> QIcon:
    pm = QPixmap(64, 64)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setBrush(QColor("#ff4d6d"))
    p.setPen(Qt.NoPen)
    p.drawRoundedRect(0, 0, 64, 64, 14, 14)
    pen = p.pen()
    pen.setColor(QColor("white"))
    pen.setWidth(5)
    pen.setStyle(Qt.SolidLine)
    pen.setCapStyle(Qt.RoundCap)
    p.setPen(pen)
    pts = [(12, 32), (18, 32), (22, 20), (28, 44), (34, 26), (38, 36), (42, 32), (52, 32)]
    for a, b in zip(pts, pts[1:]):
        p.drawLine(*a, *b)
    p.end()
    return QIcon(pm)


def _channel_script() -> QWebEngineScript:
    f = QFile(":/qtwebchannel/qwebchannel.js")
    f.open(QIODevice.ReadOnly)
    js = bytes(f.readAll()).decode()
    f.close()
    js += """
    new QWebChannel(qt.webChannelTransport, function (ch) {
      window.SH_NATIVE = ch.objects.bridge;
      document.documentElement.classList.add('native');
      window.dispatchEvent(new Event('sh-native'));
    });
    """
    s = QWebEngineScript()
    s.setName("samplehunter-bridge")
    s.setSourceCode(js)
    s.setInjectionPoint(QWebEngineScript.DocumentReady)
    s.setWorldId(QWebEngineScript.MainWorld)
    s.setRunsOnSubFrames(False)
    return s


class Window(QMainWindow):
    def __init__(self, url: str):
        super().__init__()
        self.setWindowTitle("SampleHunter")
        self.setWindowIcon(_icon())
        self.resize(1440, 900)
        self.view = QWebEngineView(self)
        page = QWebEnginePage(self.view)
        self.view.setPage(page)
        self.bridge = Bridge(self.view)
        self.channel = QWebChannel(page)
        self.channel.registerObject("bridge", self.bridge)
        page.setWebChannel(self.channel)
        page.scripts().insert(_channel_script())
        page.profile().downloadRequested.connect(self._download)
        self.view.setUrl(QUrl(url))
        self.setCentralWidget(self.view)

    def _download(self, item) -> None:
        # Download buttons and ZIPs go to the normal Downloads folder.
        folder = os.path.join(os.path.expanduser("~"), "Downloads")
        os.makedirs(folder, exist_ok=True)
        item.setDownloadDirectory(folder)
        item.accept()
        self.statusBar().showMessage("Gespeichert in %s: %s" % (folder, item.downloadFileName()), 6000)


def main() -> int:
    port = free_port(int(os.environ.get("PORT", "8765")))
    server = serve.ThreadingHTTPServer(("127.0.0.1", port), serve.Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    threading.Thread(target=serve.autopilot, daemon=True).start()

    app = QApplication(sys.argv)
    app.setApplicationName("SampleHunter")
    win = Window("http://localhost:%d" % port)
    win.show()
    code = app.exec()
    serve._hunt["auto"] = None
    if serve.hunting():
        serve._hunt["proc"].terminate()
    server.shutdown()
    return code


if __name__ == "__main__":
    sys.exit(main())
