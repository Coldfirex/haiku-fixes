#!/bin/sh
# Haiku-guest helper. Does not walk the disk (that hangs SSH).
# Set HAIKU_SRC once if the tree is not in a common path.

set -e

FIXES_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
GERRIT_HTTP=${GERRIT_HTTP:-https://review.haiku-os.org}
ENV_FILE=$FIXES_DIR/.on-haiku.env
TRY_PATCHDIR=$FIXES_DIR/.tmp
TRY_STATE=$FIXES_DIR/.on-haiku-try

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
	for d in "$HOME/haiku" "$HOME/src/haiku" "$HOME/source/haiku" /boot/home/haiku /boot/home/src/haiku /boot/home/source/haiku "$FIXES_DIR/../haiku"; do
		if is_haiku_tree "$d"; then
			HAIKU_SRC=$(CDPATH= cd -- "$d" && pwd)
			guess_output
			return 0
		fi
	done
	return 1
}

save_env() {
	[ -n "$HAIKU_SRC" ] || return 0
	printf 'HAIKU_SRC=%s\nHAIKU_OUTPUT=%s\n' "$HAIKU_SRC" "$HAIKU_OUTPUT" > "$ENV_FILE"
	TRY_STATE=$HAIKU_SRC/.on-haiku-try
}

need_haiku() {
	[ "$(uname)" = "Haiku" ] || { echo "run this inside Haiku" >&2; exit 1; }
}

need_src() {
	echo "looking for haiku source tree..."
	if ! find_haiku_src; then
		echo "not found. Set it:" >&2
		echo "  export HAIKU_SRC=/path/to/haiku" >&2
		exit 1
	fi
	save_env
	echo "HAIKU_SRC=$HAIKU_SRC"
	echo "HAIKU_OUTPUT=$HAIKU_OUTPUT"
}

have() { command -v "$1" >/dev/null 2>&1; }

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
		mkdir -p "$HOME/config/non-packaged/add-ons/kernel/drivers/bin"
		copy_one "$bin" "$HOME/config/non-packaged/add-ons/kernel/drivers/bin/$target"
		;;
	esac
}

cmd_try() {
	need_haiku
	need_src
	change=$1
	ps=$2
	[ -n "$change" ] || { echo "usage: $0 try 11584" >&2; exit 1; }
	[ -f "$TRY_STATE" ] && { echo "already trying:"; cat "$TRY_STATE"; echo "run: $0 untry"; exit 1; }
	[ -d "$HAIKU_OUTPUT" ] || { echo "no $HAIKU_OUTPUT — run: $0 configure" >&2; exit 1; }
	echo "asking Gerrit for $change ..."
	if [ -z "$ps" ]; then
		mod=$(printf '%02d' $((change % 100)))
		ps=$(git ls-remote "$GERRIT_HTTP/haiku" "refs/changes/$mod/$change/*" | awk '{print $2}' | sed -n "s|^refs/changes/[0-9][0-9]/$change/||p" | grep -E '^[0-9]+$' | sort -n | tail -1)
	fi
	[ -n "$ps" ] || { echo "Gerrit change $change not found" >&2; exit 1; }
	mod=$(printf '%02d' $((change % 100)))
	ref="refs/changes/$mod/$change/$ps"
	echo "fetch $ref"
	git -C "$HAIKU_SRC" fetch "$GERRIT_HTTP/haiku" "$ref"
	echo "apply"
	mkdir -p "$TRY_PATCHDIR"
	patch=$TRY_PATCHDIR/gerrit-$change-$ps.patch
	git -C "$HAIKU_SRC" format-patch -1 --stdout FETCH_HEAD > "$patch"
	git -C "$HAIKU_SRC" apply --check "$patch"
	git -C "$HAIKU_SRC" apply "$patch"
	targets=$(awk '/^diff --git / { f=$3; sub(/^a\//,"",f); print f }' "$patch" | awk '{ f=$0; if (f ~ /accelerants\/virtio\//) print "virtio_gpu.accelerant"; else if (f ~ /graphics\/virtio\//) print "virtio_gpu"; }' | sort -u)
	[ -n "$targets" ] || { echo "applied, but no virtio_gpu jam targets" >&2; exit 1; }
	echo "jam $targets  (this can take a few minutes with little output)"
	( cd "$HAIKU_OUTPUT" && jam -q $targets )
	echo "overlay $targets"
	for t in $targets; do overlay_target "$t"; done
	printf 'CHANGE=%s\nPS=%s\nPATCH=%s\nTARGETS=%s\n' "$change" "$ps" "$patch" "$targets" > "$TRY_STATE"
	echo "done. Reboot, then: screenmode 1920 1080 32"
}

cmd_untry() {
	need_haiku
	need_src
	[ -f "$TRY_STATE" ] || { echo "no try state"; exit 1; }
	. "$TRY_STATE"
	[ -n "$PATCH" ] && [ -f "$PATCH" ] && git -C "$HAIKU_SRC" apply -R "$PATCH" || true
	for t in $TARGETS; do
		rm -f "$HOME/config/non-packaged/add-ons/accelerants/$t"
		rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/bin/$t"
		rm -f "$HOME/config/non-packaged/add-ons/kernel/drivers/dev/graphics/$t"
	done
	rm -f "$TRY_STATE"
	echo "untry done. Reboot."
}

cmd_configure() {
	need_haiku
	need_src
	cd "$HAIKU_SRC"
	[ -d "$HAIKU_OUTPUT" ] && { echo "already configured: $HAIKU_OUTPUT"; return 0; }
	echo "configure --use-gcc-pipe"
	./configure --use-gcc-pipe
}

cmd=${1:-}
shift || true
case $cmd in
try) cmd_try "${1:?11584}" "${2:-}" ;;
untry) cmd_untry ;;
configure) cmd_configure ;;
*) echo "usage: $0 try 11584 | untry | configure" ;;
esac
