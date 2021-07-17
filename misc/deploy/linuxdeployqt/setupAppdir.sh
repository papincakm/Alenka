mkdir -p appdir/usr/bin
mkdir -p appdir/usr/share/applications
mkdir -p appdir/usr/share/icons/hicolor
touch appdir/usr/share/icons/hicolor/default.png
cp ../misc/deploy/linuxdeployqt/alenka.desktop appdir/usr/share/applications/
cp Alenka appdir/usr/bin/
