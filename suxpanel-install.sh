#!/bin/sh
#
# SuxPanel user installer script
# Version 0.1
# Copyright (c) Leandro Pereira <leandro@linuxmag.com.br>
#

function createAppDir {
	icon="$1"
	app="$2"
	path="$3"

	mkdir -p "$path"					2>/dev/null
	rm -f "$path/AppRun" 					2>/dev/null
	echo -e "#!/bin/sh\n$app" > "$path/AppRun"		2>/dev/null
	chmod +x "$path/AppRun" 				2>/dev/null
	ln -s "/usr/share/pixmaps/$icon" "$path/.DirIcon"	2>/dev/null
}

DIR="${HOME}/.suxpanel"

if [ -e $DIR ]; then
	echo "$DIR exists, aborting."
	exit 1
fi

if [ ! -e /usr/share/suxpanel ]; then
	echo "\"make install\" first :)"
	exit 1
fi

echo -e "--- Installing SuxPanel..."

APPS="$DIR/apps"
APPBAR="$DIR/appbar"
MODULE="$DIR/modules.ini"

mkdir -p $APPS
mkdir -p $APPBAR
touch $MODULE

echo -e "--- Searching modules..."
P=`pwd`
cd /usr/share/suxpanel/plugins
for i in $( find *.so ); do
	case $i in
		clock.so)
			ALIGN="r" ;;
		apps.so)
			ALIGN="f" ;;
		appbar.so)
			ALIGN="l" ;;
		mixer.so)
			ALIGN="r" ;;
		weather.so)
			ALIGN="r" ;;
		sux-ws.so)
			ALIGN="r" ;;
		sux-tasklist-applet.so)
			ALIGN="r" ;;		
		clipman.so)
			ALIGN="r" ;;
		mount.so)
			ALIGN="r" ;;
		*)
			ALIGN="l" ;;
	esac

	if [ "$ALIGN" != "f" ]; then
		echo " - $i (align=$ALIGN)"
		echo "$ALIGN=`pwd`/$i" >> $MODULE
	fi
done

echo "l=`pwd`/apps.so" > "$MODULE.tmp"
cat "$MODULE" >> "$MODULE.tmp"
rm -f "$MODULE" 
mv "$MODULE.tmp" "$MODULE"

cd $P

echo ""

FILEMAN="rox rox-filer nautilus konqueror gmc"

for i in $FILEMAN ; do
	echo -e "--- Searching for $i file manager..."
	for j in `which $i`; do
		if [ -e "$j" ]; then
			echo "Found in $j."
			FILEMGR="$j"
			break;
		fi
	done

	if [ -e "$FILEMGR" ]; then break; fi
done

echo -e "--- Creating menu entries..."

createAppDir "gnome-home.png" "$FILEMGR \"$HOME\"" "$APPS/Home Directory"
createAppDir "gnome-terminal.png" "xterm" "$APPS/Terminal"

echo -e "--- Importing GNOME menu entries"
P=`pwd`
cd $APPS
GNOMEAPPDIR="/usr/share/applications /opt/gnome/share/applications"
for j in $GNOMEAPPDIR; do
  echo "--- Searching in $j."
  [ ! -e "$j" ] && break
  for i in $( find "$j" -name \*desktop ); do
          APPICON=`cat $i|grep "^Icon="|cut -d= -f2|head -n 1`
          APPNAME=`cat $i|grep "^Name="|cut -d= -f2|head -n 1`
          APPEXEC=`cat $i|grep "^Exec="|cut -d= -f2|head -n 1`

          CATEGORY=`cat $i|grep "^Categories="|cut -d= -f2|head -n 1`
          CATEGORY=$(echo $CATEGORY | sed s/\;/\\//g | sed s/GNOME//g | sed s/Application//g)
          CATEGORY=$(echo $CATEGORY | sed s/X\-\.*//g | sed s/AudioVideo/Multimedia/g);
          CATEGORY=$(echo $CATEGORY | sed s/Core/Utility/g )
          
          APPNAME="$APPS$CATEGORY/$APPNAME"

          if [ ! -e "$APPNAME" ]; then
                  echo -n "." 

                  createAppDir "$APPICON" "$APPEXEC" "$APPNAME"
          fi
  done
  echo ""
done
cd $P

echo ""

echo "Done installing."

echo ""

echo "Your menu is saved in $APPS."
echo " - Groups are directories."
echo " - Change the directory icon with a .DirIcon file."
echo " - Put some AppDir inside the directories."
echo "More information about ''AppDir'' may be found at:"
echo " - http://rox.sourceforge.net/appdirs.html"
echo ""

echo -n "Open $APPS with $FILEMGR? (Y/n) "
read foo

if [ "$foo" == "n" ]; then exit; fi

$FILEMGR "$APPS"

