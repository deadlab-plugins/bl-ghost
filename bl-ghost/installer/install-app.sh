#!/bin/sh

# release
#src_dir="./bl-ghost-app"

# debug
src_dir="../build-linux/bl-ghost-app"

cd "$(dirname "$0")"

make_desktop_item()
{
    echo "[Desktop Entry]"
    echo "Encoding=UTF-8"
    echo "Type=Application"
    while [ $# -gt 0 ]; do echo "$1"; shift; done
}

install_desktop()
{
    echo -n "$3"
    tmpdir=$(mktemp -d -t ghost-installXXXX)

    if [ "$1" = "uninstall" ]; then
	xdg-desktop-menu uninstall --mode user bluelab-ghost.directory "bluelab-ghost.desktop"
	xdg-icon-resource uninstall --mode user --size 256 bluelab-ghost.directory
	xdg-desktop-menu uninstall --mode user bluelab-ghost.desktop
    else
	make_desktop_item \
	    "Name=Ghost" \
	    "Comment=Ghost" \
	    "Categories=Audio;Video;AudioVideo;AudioVideoEditing;Recorder;" \
	    "Exec=\"$(pwd)/$src_dir/bl-ghost\" %F" \
	    "Icon=bluelab-ghost" \
	    "MimeType=audio/x-wav;audio/x-aiff;application/x-flac;audio/flac;audio/x-flac;audio/x-flac+ogg" \
	    "StartupWMClass=Ghost" \
	    > "$tmpdir/bluelab-ghost.desktop"

	xdg-icon-resource install --mode user --size 256 "$(pwd)/$src_dir/resources/img/bl-ghost.png" bluelab-ghost

	xdg-desktop-menu install $2 "$tmpdir/bluelab-ghost.desktop" 
	rm -f -- "$tmpdir/bluelab-ghost.desktop"
    fi
}

install_desktop $1
