#!/usr/bin/env python3
"""
ArmPilot - gomulu web arayuzunu tarayicida onizle (ESP32 gerekmez).

src/web_ui.cpp icindeki R"PAGE( ... )PAGE" ham dizesini cikarip
.pio/preview/index.html olarak yazar ve yerel bir sunucuda acar.

  python tools/preview.py              # cikar, sun, tarayicida ac
  python tools/preview.py --no-open    # sun ama tarayiciyi acma
  python tools/preview.py --no-serve   # sadece dosyayi yaz
  python tools/preview.py --port 8080

Not: cihaz olmadigi icin WebSocket baglanmaz -> rozet "KOPUK" kalir ve
arayuz config.cpp yerine JS'teki varsayilan geometri/limitlerle cizilir.
Yerel sunucu uzerinden acmak file:// yerine tercih edilir: file:// altinda
location.hostname bos olur ve WebSocket kurucusu hata firlatir.
"""
import argparse
import http.server
import io
import os
import socketserver
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src", "web_ui.cpp")
OUT_DIR = os.path.join(ROOT, ".pio", "preview")
OUT = os.path.join(OUT_DIR, "index.html")

OPEN_TOKEN = 'R"PAGE('
CLOSE_TOKEN = ')PAGE";'


def extract():
    with io.open(SRC, encoding="utf-8") as f:
        src = f.read()
    i = src.find(OPEN_TOKEN)
    if i < 0:
        sys.exit("HATA: %s icinde %s bulunamadi." % (SRC, OPEN_TOKEN))
    i += len(OPEN_TOKEN)
    j = src.find(CLOSE_TOKEN, i)
    if j < 0:
        sys.exit("HATA: ham dizenin sonu ()PAGE\";) bulunamadi.")
    return src[i:j]


def open_in_browser(url):
    """URL'yi isletim sisteminin varsayilan tarayicisinda YENI SEKMEDE acar.

    Windows'ta bilerek os.startfile kullaniliyor: kabuk yoneticisi URL'yi
    zaten calisan tarayiciya iletir, yeni sekme acar ve acik sekmelere
    dokunmaz. webbrowser modulu ise Chrome'u dogrudan alt surec olarak
    baslatabiliyor; bu da mevcut oturumu yeniden baslatip acik sekmelerin
    kapanmasina yol acabiliyor.
    """
    try:
        if os.name == "nt":
            os.startfile(url)  # noqa: S606 - kasitli: kabuk varsayilani
            return
    except OSError as e:
        print("tarayici acilamadi (%s); adresi elle acin: %s" % (e, url))
        return
    import webbrowser

    if not webbrowser.open_new_tab(url):
        print("tarayici acilamadi; adresi elle acin: %s" % url)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--no-serve", action="store_true")
    ap.add_argument("--no-open", action="store_true",
                    help="sunucuyu baslat ama tarayiciyi acma")
    args = ap.parse_args()

    html = extract()
    os.makedirs(OUT_DIR, exist_ok=True)
    with io.open(OUT, "w", encoding="utf-8", newline="") as f:
        f.write(html)
    print("yazildi: %s  (%.1f KB)" % (OUT, len(html.encode("utf-8")) / 1024.0))

    if args.no_serve:
        return

    os.chdir(OUT_DIR)

    class Quiet(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *a):
            pass

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("127.0.0.1", args.port), Quiet) as httpd:
        url = "http://localhost:%d/" % args.port
        print("sunuluyor: %s   (durdurmak icin Ctrl+C)" % url)
        # Soket __init__ icinde bind+listen edildi; istekler backlog'da
        # bekleyecegi icin acmadan once gecikmeye gerek yok.
        if not args.no_open:
            open_in_browser(url)
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nkapatildi.")


if __name__ == "__main__":
    main()
