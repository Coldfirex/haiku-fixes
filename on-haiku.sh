#!/bin/sh
# All-in-one helper meant to run *inside Haiku* (the guest).
# See Coldfirex/haiku-fixes. Auto-detects HAIKU_SRC.

set -e

FIXES_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
GERRIT_USER=${GERRIT_USER:-Coldfirex}
GERRIT_HTTP=${GERRIT_HTTP:-https://review.haiku-os.org}
ENV_FILE=$FIXES_DIR/.on-haiku.env
TRY_PATCHDIR=$FIXES_DIR/.tmp

is_haiku_tree() {
	[ -d "$1/src/add-ons" ] && { [ -d "$1/.git" ] || [ -f "$1/configure" ]; }
}

load_saved_env() {
	[ -f "$ENV_FILE" ] && . "$ENV_FILE"
}

guess_output() {
	[ -n "$HAIKU_OUTPUT" ] && [ -d "$HAIKU_OUTPUT" ] && return
	[ -z "$HAIKU_SRC" ] && return
	if [ -d "$HAIKU_SRC/generated" ]; then
		HAIKU_OUTPUT=$HAIKU_SRC/generated
		return
	fi
	set -- "$HAIKU_SRC"/generated.*
	if [ -d "$1" ]; then
		HAIKU_OUTPUT=$1
	else
		HAIKU_OUTPUT=$HAIKU_SRC/generated
	fi
}

find_haiku_src() {
	if [ -n "$HAIKU_SRC" ] && is_haiku_tree "$HAIKU_SRC"; then
		guess_output
		return 0
	fi
	load_saved_env
	if [ -n "$HAIKU_SRC" ] && is_haiku_tree "$HAIKU_SRC"; then
		guess_output
		return 0
	fi
	for d in "$HOME/haiku" "$HOME/src/haiku" "$HOME/source/haiku" /boot/home/haiku /boot/home/src/haiku /boot/home/source/haiku "$FIXES_DIR/../haiku" "$FIXES_DIR/../../haiku"; do
		if is_haiku_tree "$d"; then
			HAIKU_SRC=$(CDPATH= cd -- "$d" && pwd)
			guess_output
			return 0
		fi
	done
	found=$(find "$HOME" /boot/home -maxdepth 5 -type d -path '*/src/add-ons' 2>/dev/null | head -1)
	if [ -n "$found" ]; then
		HAIKU_SRC=$(CDPATH= cd -- "$found/../.." && pwd)
		if is_haiku_tree "$HAIKU_SRC"; then
			guess_output
			return 0
		fi
	fi
	return 1
}

save_env() {
	[ -n "$HAIKU_SRC" ] && printf 'HAIKU_SRC=%s\nHAIKU_OUTPUT=%s\n' "$HAIKU_SRC" "$HAIKU_OUTPUT" > "$ENV_FILE"
}

find_haiku_src || true
TRY_STATE=${HAIKU_SRC:+$HAIKU_SRC/.on-haiku-try}
TRY_STATE=${TRY_STATE:-$FIXES_DIR/.on-haiku-try}

need_haiku() {
	if [ "$(uname)" != "Haiku" ]; then
		echo "on-haiku.sh is for the Haiku guest" >&2
		exit 1
	fi
}

need_src() {
	if ! find_haiku_src; then
		echo "no haiku source tree found. export HAIKU_SRC=/path/to/haiku" >&2
		exit 1
	fi
	save_env
	echo "using HAIKU_SRC=$HAIKU_SRC"
	echo "using HAIKU_OUTPUT=$HAIKU_OUTPUT"
}

have() { command -v "$1" >/dev/null 2>&1; }

usage() {
	cat <<EOF
usage: $0 try <gerrit-change>
       $0 untry | configure | test-log | screen | bootstrap
EOF
}

copy_one() {
	src=$1; dst=$2
	mkdir -p "$(dirname "$dst")"
	echo "install $src -> $dst"
	if have copyattr; then copyattr --data "$src" "$dst"; else cp -f "$src" "$dst"; fi
	chmod +x "$dst"
}

built_addon() {
	find "$HAIKU_OUTPUT/objects" -type f -path '*/release/*' -name "$1" 2>/dev/null | head -1
}

overlay_target() {
	target=$1
	bin=$(built_addon "$target")
	[ -n "$bin" ] && [ -f "$bin" ] || { echo "built $target not found" >&2; return 1; }
	case $target in
	*.accelerant) copy_one "$bin" "$HOME/config/non-packaged/add-ons/accelerants/$target" ;;
	virtio_gpu|vesa|framebuffer)
		base=$HOME/config/non-packaged/add-ons/kernel/drivers
		mkdir -p "$base/bin" "$base/dev/graphics"
		copy_one "$bin" "$base/bin/$target"
		ln -sfn "../../bin/$target" "$base/dev/graphics/$target"
		;;
	*)
		base=$HOME/config/non-packaged/add-ons/kernel/drivers
		mkdir -p "$base/bin"
		copy_one "$bin" "$base/bin/$target"
		;;
	esac
}

cmd_overlay() {
	need_haiku
	for t in "$@"; do overlay_target "$t"; done
	echo "overlay ready. Reboot."
}

cmd_configure() {
	need_haiku; need_src
	cd "$HAIKU_SRC"
	[ -d "$HAIKU_OUTPUT" ] && { echo "already configured: $HAIKU_OUTPUT"; return 0; }
	./configure --use-gcc-pipe
}

cmd_bootstrap() {
	need_haiku
	have git || pkgman install git
	have curl || pkgman install curl
	have jam || pkgman install jam
	if ! find_haiku_src; then
		HAIKU_SRC=$HOME/haiku
		HAIKU_OUTPUT=$HAIKU_SRC/generated
	fi
	if [ ! -d "$HAIKU_SRC/.git" ]; then
		git clone "https://git.haiku-os.org/haiku" "$HAIKU_SRC"
	fi
	save_env
}

gerrit_latest_ps() {
	change=$1
	mod=$(printf '%02d' $((change % 100)))
	git ls-remote "$GERRIT_HTTP/haiku" "refs/changes/$mod/$change/*" | awk '{print $2}' | sed -n "s|^refs/changes/[0-9][0-9]/$change/||p" | grep -E '^[0-9]+$' | sort -n | tail -1
}

files_to_targets() {
	awk '{ f=$0; if (f ~ /accelerants\/virtio\//) print "virtio_gpu.accelerant"; else if (f ~ /drivers\/graphics\/virtio\// || f ~ /graphics\/virtio\//) print "virtio_gpu"; else if (f ~ /ether\/virtio\//) print "virtio_net"; else if (f ~ /protocols\/udp\//) print "udp"; else if (f ~ /protocols\/tcp\//) print "tcp"; else if (f ~ /protocols\/ipv4\//) print "ipv4"; else if (f ~ /protocols\/icmp\//) print "icmp"; else if (f ~ /datalink_protocols\/arp\//) print "arp"; }' | sort -u
}

cmd_try() {
	need_haiku; need_src
	change=$1; ps=$2
	[ -n "$change" ] || { echo "try needs a Gerrit change number" >&2; exit 1; }
	[ -f "$TRY_STATE" ] && { echo "run: $0 untry first"; cat "$TRY_STATE"; exit 1; }
	[ -d "$HAIKU_OUTPUT" ] || { echo "run: $0 configure"; exit 1; }
	[ -z "$ps" ] && ps=$(gerrit_latest_ps "$change")
	[ -n "$ps" ] || { echo "change $change not found on Gerrit" >&2; exit 1; }
	mod=$(printf '%02d' $((change % 100)))
	ref="refs/changes/$mod/$change/$ps"
	echo "fetch $ref"
	git -C "$HAIKU_SRC" fetch "$GERRIT_HTTP/haiku" "$ref"
	mkdir -p "$TRY_PATCHDIR"
	patch=$TRY_PATCHDIR/gerrit-$change-$ps.patch
	git -C "$HAIKU_SRC" format-patch -1 --stdout FETCH_HEAD > "$patch"
	git -C "$HAIKU_SRC" apply --check "$patch"
	git -C "$HAIKU_SRC" apply "$patch"
	targets=$(awk '/^diff --git / { f=$3; sub(/^a\//,"",f); print f }' "$patch" | files_to_targets)
	[ -n "$targets" ] || { echo "applied, no known jam targets"; exit 1; }
	echo "jam $targets"
	( cd "$HAIKU_OUTPUT" && jam -q $targets )
	cmd_overlay $targets
	printf 'CHANGE=%s\nPS=%s\nPATCH=%s\nTARGETS=%s\n' "$change" "$ps" "$patch" "$targets" > "$TRY_STATE"
	echo "Applied $change/$ps. Reboot, then screenmode 1920 1080 32"
}

cmd_untry() {
	need_haiku; need_src
	[ -f "$TRY_STATE" ] || { echo "no try state"; exit 1; }
	. "$TRY_STATE"
	[ -n "$PATCH" ] && [ -f "$PATCH" ] && git -C "$HAIKU_SRC" apply -R "$PATCH" || true
	for t in $TARGETS; do
		case $t in
		*.accelerant) rm -f "$HOME/config/non-packaged/add-ons/accelerants/$t" ;;
		*) rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/bin/$t"; rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/dev/graphics/$t" ;;
		esac
	done
	rm -f "$TRY_STATE"
	echo "untry done. Reboot."
}

cmd_test_log() {
	need_haiku
	have listimage && listimage | grep -i virtio || true
	have screenmode && screenmode || true
	[ -f /var/log/syslog ] && grep -i virtio_gpu /var/log/syslog | tail -40
}

cmd=${1:-help}; shift || true
case $cmd in
bootstrap) cmd_bootstrap ;;
configure) cmd_configure ;;
try) cmd_try "${1:?change}" "${2:-}" ;;
untry) cmd_untry ;;
overlay) cmd_overlay "$@" ;;
test-log) cmd_test_log ;;
*) usage ;;
esac
