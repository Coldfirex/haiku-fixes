#!/bin/sh
# All-in-one helper meant to run *inside Haiku* (the guest).
# Grok can write/update this file; it cannot log into your VM or Gerrit.
#
# First boot of a new tree:
#   export GERRIT_USER=Coldfirex
#   export GIT_AUTHOR_EMAIL='the-email-verified-on-gerrit'
#   ./on-haiku.sh bootstrap
#   ./on-haiku.sh configure
#
# Test someone else's Gerrit change (no commit):
#   ./on-haiku.sh try 11584
#   reboot
#   screenmode 1920 1080 32
#   ./on-haiku.sh test-log
#   ./on-haiku.sh untry          # drop the overlay + reverse the apply
#
# Our own issue folders:
#   ./on-haiku.sh go virtio-gpu-detach-backing

set -e

FIXES_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
HAIKU_SRC=${HAIKU_SRC:-$HOME/haiku}
HAIKU_OUTPUT=${HAIKU_OUTPUT:-$HAIKU_SRC/generated}
GERRIT_USER=${GERRIT_USER:-Coldfirex}
GERRIT_HTTP=${GERRIT_HTTP:-https://review.haiku-os.org}
FIXES_GIT=${FIXES_GIT:-https://github.com/Coldfirex/haiku-fixes.git}
TRY_STATE=$HAIKU_SRC/.on-haiku-try
TRY_PATCHDIR=$FIXES_DIR/.tmp

need_haiku() {
	if [ "$(uname)" != "Haiku" ]; then
		echo "on-haiku.sh is for the Haiku guest. uname=$(uname)" >&2
		echo "On Linux use work.sh / gerrit.sh instead." >&2
		exit 1
	fi
}

need_src() {
	if [ ! -d "$HAIKU_SRC/.git" ]; then
		echo "no haiku tree at $HAIKU_SRC; run $0 bootstrap" >&2
		exit 1
	fi
}

usage() {
	cat <<EOF
usage: $0 <command> [arg]

Haiku-guest commands:
  bootstrap              clone haiku, hook, pkgs
  pkgs                   pkgman install git / openssh / curl if missing
  configure              ./configure in HAIKU_SRC (once per tree)
  pull                   git fetch/rebase haiku master; pull haiku-fixes
  try <change> [ps]      fetch a Gerrit change, apply, jam, overlay
  untry                  reverse the last try (apply -R + drop overlay)
  go <name>              check + apply + jam + overlay-install (local issue)
  install <name>         copy built add-on into non-packaged
  overlay <target...>    copy named jam outputs into non-packaged
  test-log               listimage + syslog virtio_gpu
  screen                 print current graphics driver if listimage exists
  commit <name>          gerrit.sh commit
  push <name>            gerrit.sh push
  preflight              gerrit.sh preflight
  status                 trees + last commit

env:
  HAIKU_SRC      default \$HOME/haiku
  HAIKU_OUTPUT   default \$HAIKU_SRC/generated
  GERRIT_USER    default Coldfirex
  GIT_AUTHOR_EMAIL   used by bootstrap for git config
EOF
}

have() {
	command -v "$1" >/dev/null 2>&1
}

cmd_pkgs() {
	need_haiku
	missing=
	have git || missing="$missing git"
	have ssh || missing="$missing openssh"
	have curl || missing="$missing curl"
	have jam || missing="$missing jam"
	if [ -n "$missing" ]; then
		echo "pkgman install$missing"
		pkgman install $missing
	else
		echo "git ssh curl jam already present"
	fi
}

cmd_bootstrap() {
	need_haiku
	cmd_pkgs

	if [ ! -d "$HAIKU_SRC/.git" ]; then
		echo "clone haiku -> $HAIKU_SRC"
		git clone "ssh://${GERRIT_USER}@git.haiku-os.org/haiku" "$HAIKU_SRC"
	else
		echo "haiku tree exists: $HAIKU_SRC"
	fi

	hook=$HAIKU_SRC/.git/hooks/commit-msg
	if [ ! -x "$hook" ]; then
		curl -Lo "$hook" "$GERRIT_HTTP/tools/hooks/commit-msg"
		chmod +x "$hook"
	fi

	if [ -n "$GIT_AUTHOR_EMAIL" ]; then
		git -C "$HAIKU_SRC" config user.name "${GIT_AUTHOR_NAME:-Alan Shearer}"
		git -C "$HAIKU_SRC" config user.email "$GIT_AUTHOR_EMAIL"
	fi

	if [ ! -d "$FIXES_DIR/.git" ]; then
		echo "haiku-fixes dir is $FIXES_DIR (not a clone; pull skipped)"
	else
		git -C "$FIXES_DIR" pull --ff-only || true
	fi

	echo
	echo "Next:"
	echo "  $0 configure"
	echo "  $0 try 11584          # Gerrit change, then reboot"
}

cmd_configure() {
	need_haiku
	need_src
	cd "$HAIKU_SRC"
	if [ -d "$HAIKU_OUTPUT" ]; then
		echo "already configured: $HAIKU_OUTPUT"
		return 0
	fi
	./configure --use-gcc-pipe
}

cmd_pull() {
	need_haiku
	need_src
	git -C "$HAIKU_SRC" fetch origin
	git -C "$HAIKU_SRC" checkout master
	git -C "$HAIKU_SRC" pull --rebase origin master
	if [ -d "$FIXES_DIR/.git" ]; then
		git -C "$FIXES_DIR" pull --ff-only || true
	fi
}

built_addon() {
	target=$1
	if [ -z "$target" ]; then
		return 1
	fi
	# Prefer release/ over debug/ if both exist.
	find "$HAIKU_OUTPUT/objects" -type f -path '*/release/*' -name "$target" 2>/dev/null | head -1
}

copy_one() {
	src=$1
	dst=$2
	mkdir -p "$(dirname "$dst")"
	echo "install $src -> $dst"
	if have copyattr; then
		copyattr --data "$src" "$dst"
	else
		cp -f "$src" "$dst"
	fi
	chmod +x "$dst"
}

overlay_target() {
	target=$1
	bin=$(built_addon "$target")
	if [ -z "$bin" ] || [ ! -f "$bin" ]; then
		echo "built $target not found under $HAIKU_OUTPUT/objects" >&2
		return 1
	fi
	case $target in
	*.accelerant)
		copy_one "$bin" "$HOME/config/non-packaged/add-ons/accelerants/$target"
		;;
	virtio_gpu|vesa|framebuffer|intel_extreme|radeon_hd|s3)
		base=$HOME/config/non-packaged/add-ons/kernel/drivers
		mkdir -p "$base/bin" "$base/dev/graphics"
		copy_one "$bin" "$base/bin/$target"
		ln -sfn "../../bin/$target" "$base/dev/graphics/$target"
		;;
	virtio_net)
		base=$HOME/config/non-packaged/add-ons/kernel/drivers
		mkdir -p "$base/bin" "$base/dev/net"
		copy_one "$bin" "$base/bin/$target"
		ln -sfn "../../bin/$target" "$base/dev/net/$target"
		;;
	*)
		base=$HOME/config/non-packaged/add-ons/kernel/drivers
		mkdir -p "$base/bin"
		copy_one "$bin" "$base/bin/$target"
		echo "copied $target to $base/bin (no device symlink; unknown class)"
		;;
	esac
}

cmd_overlay() {
	need_haiku
	if [ $# -lt 1 ]; then
		echo "overlay needs jam target names" >&2
		exit 1
	fi
	for t in "$@"; do
		overlay_target "$t"
	done
	echo "overlay ready. Reboot so the kernel reloads drivers."
}

cmd_install() {
	need_haiku
	name=$1
	target=$(awk -F '	' -v n="$name" '/^[^#]/ && $1 == n { print $2; exit }' "$FIXES_DIR/MANIFEST")
	if [ -z "$target" ]; then
		echo "no jam target for $name" >&2
		exit 1
	fi
	# MANIFEST column 2 may list several targets.
	# shellcheck disable=SC2086
	cmd_overlay $target
}

cmd_go() {
	need_haiku
	name=$1
	if [ -z "$name" ]; then
		echo "go needs an issue name" >&2
		exit 1
	fi
	export HAIKU_SRC HAIKU_OUTPUT
	"$FIXES_DIR/work.sh" check "$name"
	"$FIXES_DIR/work.sh" apply "$name" || echo "apply skipped (already applied?)"
	"$FIXES_DIR/work.sh" jam "$name"
	cmd_install "$name"
}

gerrit_latest_ps() {
	change=$1
	mod=$(printf '%02d' $((change % 100)))
	git ls-remote "$GERRIT_HTTP/haiku" "refs/changes/$mod/$change/*" \
		| awk '{print $2}' \
		| sed -n "s|^refs/changes/[0-9][0-9]/$change/||p" \
		| grep -E '^[0-9]+$' \
		| sort -n \
		| tail -1
}

files_to_targets() {
	# Map changed source paths to jam targets. Unknown paths are ignored.
	awk '
		{
			f = $0
			if (f ~ /add-ons\/accelerants\/virtio\//) print "virtio_gpu.accelerant"
			else if (f ~ /drivers\/graphics\/virtio\// || f ~ /headers\/private\/graphics\/virtio\//)
				print "virtio_gpu"
			else if (f ~ /drivers\/network\/ether\/virtio\//) print "virtio_net"
			else if (f ~ /network\/protocols\/udp\//) print "udp"
			else if (f ~ /network\/protocols\/tcp\//) print "tcp"
			else if (f ~ /network\/protocols\/ipv4\//) print "ipv4"
			else if (f ~ /network\/protocols\/icmp\//) print "icmp"
			else if (f ~ /datalink_protocols\/arp\//) print "arp"
		}
	' | sort -u
}

cmd_try() {
	need_haiku
	need_src
	change=$1
	ps=$2
	if [ -z "$change" ]; then
		echo "try needs a Gerrit change number (e.g. 11584)" >&2
		exit 1
	fi
	if [ -f "$TRY_STATE" ]; then
		echo "a previous try is recorded:" >&2
		cat "$TRY_STATE" >&2
		echo "run: $0 untry" >&2
		exit 1
	fi
	if [ ! -d "$HAIKU_OUTPUT" ]; then
		echo "tree not configured; run $0 configure" >&2
		exit 1
	fi

	if [ -z "$ps" ]; then
		echo "looking up latest patch set for $change"
		ps=$(gerrit_latest_ps "$change")
	fi
	if [ -z "$ps" ]; then
		echo "could not find refs/changes/../$change/* on $GERRIT_HTTP" >&2
		exit 1
	fi
	mod=$(printf '%02d' $((change % 100)))
	ref="refs/changes/$mod/$change/$ps"
	echo "fetch $ref"
	git -C "$HAIKU_SRC" fetch "$GERRIT_HTTP/haiku" "$ref"

	mkdir -p "$TRY_PATCHDIR"
	patch=$TRY_PATCHDIR/gerrit-$change-$ps.patch
	git -C "$HAIKU_SRC" format-patch -1 --stdout FETCH_HEAD > "$patch"
	echo "patch $patch"
	git -C "$HAIKU_SRC" apply --check "$patch"
	git -C "$HAIKU_SRC" apply "$patch"

	targets=$(awk '/^diff --git / {
		f=$3
		sub(/^a\//, "", f)
		print f
	}' "$patch" | files_to_targets)
	if [ -z "$targets" ]; then
		echo "applied, but no known jam targets in the diff. Jam by hand." >&2
		echo "CHANGE=$change" > "$TRY_STATE"
		echo "PS=$ps" >> "$TRY_STATE"
		echo "PATCH=$patch" >> "$TRY_STATE"
		echo "TARGETS=" >> "$TRY_STATE"
		exit 1
	fi
	echo "jam $targets"
	# shellcheck disable=SC2086
	( cd "$HAIKU_OUTPUT" && jam -q $targets )
	# shellcheck disable=SC2086
	cmd_overlay $targets

	{
		echo "CHANGE=$change"
		echo "PS=$ps"
		echo "PATCH=$patch"
		echo "TARGETS=$targets"
	} > "$TRY_STATE"

	echo
	echo "Applied Gerrit $change patch set $ps"
	echo "Reboot, then:"
	echo "  screenmode                 # current"
	echo "  screenmode 1920 1080 32    # or another listed size"
	echo "  $0 test-log"
	echo "  $0 untry                   # when done"
}

cmd_untry() {
	need_haiku
	need_src
	if [ ! -f "$TRY_STATE" ]; then
		echo "no try state at $TRY_STATE" >&2
		exit 1
	fi
	# shellcheck disable=SC1090
	. "$TRY_STATE"
	if [ -n "$PATCH" ] && [ -f "$PATCH" ]; then
		echo "reverse $PATCH"
		git -C "$HAIKU_SRC" apply -R "$PATCH" || echo "reverse apply failed (already clean?)"
	fi
	for t in $TARGETS; do
		case $t in
		*.accelerant)
			rm -f "$HOME/config/non-packaged/add-ons/accelerants/$t"
			echo "removed accelerant overlay $t"
			;;
		virtio_gpu|vesa|framebuffer|intel_extreme|radeon_hd|s3)
			rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/bin/$t"
			rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/dev/graphics/$t"
			echo "removed driver overlay $t"
			;;
		*)
			rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/bin/$t"
			echo "removed overlay $t"
			;;
		esac
	done
	rm -f "$TRY_STATE"
	echo "untry done. Reboot to load packaged modules again."
}

cmd_test_log() {
	need_haiku
	echo "== Screen / loaded images =="
	if have listimage; then
		listimage | grep -i virtio || true
	fi
	echo
	echo "== screenmode =="
	if have screenmode; then
		screenmode || true
	fi
	echo
	echo "== syslog virtio_gpu (last 40) =="
	if [ -f /var/log/syslog ]; then
		grep -i virtio_gpu /var/log/syslog | tail -40
	else
		echo "no /var/log/syslog"
	fi
}

cmd_screen() {
	need_haiku
	if have listimage; then
		listimage | grep -iE 'virtio|vesa|radeon|intel|nvidia|framebuffer' || listimage
	else
		echo "listimage not found"
	fi
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

cmd=$1
shift
case $cmd in
bootstrap)  cmd_bootstrap ;;
pkgs)       cmd_pkgs ;;
configure)  cmd_configure ;;
pull)       cmd_pull ;;
try)        cmd_try "${1:?change required}" "${2:-}" ;;
untry)      cmd_untry ;;
go)         cmd_go "${1:?name required}" ;;
install)    cmd_install "${1:?name required}" ;;
overlay)    cmd_overlay "$@" ;;
test-log)   cmd_test_log ;;
screen)     cmd_screen ;;
commit)     export HAIKU_SRC GERRIT_USER; "$FIXES_DIR/gerrit.sh" commit "${1:?name required}" ;;
push)       export HAIKU_SRC GERRIT_USER; "$FIXES_DIR/gerrit.sh" push "${1:-}" ;;
preflight)  export HAIKU_SRC GERRIT_USER; "$FIXES_DIR/gerrit.sh" preflight ;;
status)     export HAIKU_SRC; "$FIXES_DIR/gerrit.sh" status ;;
-h|--help|help) usage ;;
*) usage; exit 1 ;;
esac
