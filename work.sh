#!/bin/sh
# Local apply / jam / test helper for haiku-fixes.
# Run on the machine that has the Haiku git tree (guest or Linux host).
#
#   export HAIKU_SRC=$HOME/haiku
#   export HAIKU_OUTPUT=$HOME/haiku/generated   # after configure
#   ./work.sh check
#   ./work.sh apply udp-unicast-enqueue
#   ./work.sh jam udp-unicast-enqueue
#   ./work.sh test udp-unicast-enqueue
#   ./work.sh all udp-unicast-enqueue
#
# Default FIXES_DIR is the directory containing this script.

set -e

FIXES_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
HAIKU_SRC=${HAIKU_SRC:-${HAIKU_TOP:-$HOME/haiku}}
HAIKU_OUTPUT=${HAIKU_OUTPUT:-$HAIKU_SRC/generated}
MANIFEST=$FIXES_DIR/MANIFEST

usage() {
	cat <<EOF
usage: $0 <command> [name|all]

commands:
  list                 list issues from MANIFEST
  check [name|all]     git apply --check (default: all)
  apply [name|all]     git apply
  reverse [name|all]   git apply -R
  files [name]         print paths a patch touches
  jam [name|all]       jam the mapped add-on targets
  test-build [name|all] make userspace tests
  test [name]          run that issue's test binary
  all [name]           check + apply + jam + test-build + test

env:
  HAIKU_SRC      Haiku git tree   (default: \$HOME/haiku)
  HAIKU_OUTPUT   jam generated/   (default: \$HAIKU_SRC/generated)
  JAM            jam binary       (default: jam)
EOF
}

need_src() {
	if [ ! -d "$HAIKU_SRC/.git" ] && [ ! -d "$HAIKU_SRC/src" ]; then
		echo "HAIKU_SRC is not a Haiku tree: $HAIKU_SRC" >&2
		echo "export HAIKU_SRC=/path/to/haiku" >&2
		exit 1
	fi
}

issues() {
	if [ -n "$1" ] && [ "$1" != "all" ]; then
		echo "$1"
		return
	fi
	awk -F '\t' '/^[^#]/ && NF { print $1 }' "$MANIFEST"
}

manifest_line() {
	awk -F '\t' -v n="$1" '/^[^#]/ && $1 == n { print; exit }' "$MANIFEST"
}

jam_targets_for() {
	line=$(manifest_line "$1") || true
	if [ -z "$line" ]; then
		return
	fi
	echo "$line" | awk -F '\t' '{ print $2 }'
}

test_bin_for() {
	echo "$(manifest_line "$1")" | awk -F '\t' '{ print $3 }'
}

test_args_for() {
	echo "$(manifest_line "$1")" | awk -F '\t' '{ print $4 }'
}

patch_file() {
	name=$1
	dir=$FIXES_DIR/$name
	if [ ! -d "$dir" ]; then
		echo "unknown issue: $name" >&2
		exit 1
	fi
	set -- "$dir"/*.patch
	if [ ! -f "$1" ]; then
		echo "no .patch in $dir" >&2
		exit 1
	fi
	if [ $# -ne 1 ]; then
		echo "expected one .patch in $dir, found $#" >&2
		exit 1
	fi
	echo "$1"
}

cmd_list() {
	awk -F '\t' '/^[^#]/ && NF {
		printf "%-28s jam=%-12s test=%-24s %s\n", $1, $2, $3, $5
	}' "$MANIFEST"
}

cmd_files() {
	need_src
	p=$(patch_file "$1")
	awk '/^diff --git / {
		f=$3
		sub(/^a\//, "", f)
		print f
	}' "$p"
}

apply_one() {
	mode=$1
	name=$2
	p=$(patch_file "$name")
	need_src
	case $mode in
	check)   git -C "$HAIKU_SRC" apply --check "$p" ;;
	apply)   git -C "$HAIKU_SRC" apply "$p" ;;
	reverse) git -C "$HAIKU_SRC" apply -R "$p" ;;
	esac
	echo "$mode $name"
}

cmd_apply_loop() {
	mode=$1
	shift
	failed=0
	for name in $(issues "$1"); do
		if apply_one "$mode" "$name"; then
			:
		else
			echo "FAILED $mode $name" >&2
			failed=1
		fi
	done
	return $failed
}

cmd_jam() {
	need_src
	if [ ! -d "$HAIKU_OUTPUT" ]; then
		echo "HAIKU_OUTPUT missing: $HAIKU_OUTPUT" >&2
		echo "configure the tree first, or point HAIKU_OUTPUT at generated/" >&2
		exit 1
	fi
	JAM=${JAM:-jam}
	for name in $(issues "$1"); do
		targets=$(jam_targets_for "$name")
		if [ -z "$targets" ]; then
			echo "no jam target for $name, skip" >&2
			continue
		fi
		echo "jam $name -> $targets"
		( cd "$HAIKU_OUTPUT" && $JAM -q $targets )
	done
}

cmd_test_build() {
	for name in $(issues "$1"); do
		bin=$(test_bin_for "$name")
		if [ -z "$bin" ]; then
			echo "no userspace test for $name"
			continue
		fi
		if [ ! -f "$FIXES_DIR/$name/Makefile" ]; then
			echo "no Makefile in $name" >&2
			continue
		fi
		echo "make $name"
		make -C "$FIXES_DIR/$name"
	done
}

cmd_test() {
	if [ -z "$1" ] || [ "$1" = "all" ]; then
		echo "test needs a single issue name" >&2
		exit 1
	fi
	name=$1
	bin=$(test_bin_for "$name")
	if [ -z "$bin" ]; then
		echo "$name has no userspace test (rebuild-only path)" >&2
		exit 1
	fi
	if [ ! -x "$FIXES_DIR/$name/$bin" ]; then
		make -C "$FIXES_DIR/$name"
	fi
	args=$(test_args_for "$name")
	echo "run $name/$bin $args"
	( cd "$FIXES_DIR/$name" && ./"$bin" $args )
}

cmd_all() {
	if [ -z "$1" ] || [ "$1" = "all" ]; then
		echo "all needs a single issue name" >&2
		exit 1
	fi
	name=$1
	cmd_apply_loop check "$name"
	cmd_apply_loop apply "$name"
	if [ -d "$HAIKU_OUTPUT" ]; then
		cmd_jam "$name" || echo "jam failed; add-on not rebuilt" >&2
	else
		echo "skip jam (set HAIKU_OUTPUT after configure)"
	fi
	bin=$(test_bin_for "$name")
	if [ -n "$bin" ]; then
		cmd_test_build "$name"
		cmd_test "$name"
	else
		echo "no userspace test; load the new add-on and hit the fail path"
	fi
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

cmd=$1
shift
case $cmd in
list)       cmd_list ;;
check)      cmd_apply_loop check "${1:-all}" ;;
apply)      cmd_apply_loop apply "${1:-all}" ;;
reverse)    cmd_apply_loop reverse "${1:-all}" ;;
files)      cmd_files "${1:?name required}" ;;
jam)        cmd_jam "${1:-all}" ;;
test-build) cmd_test_build "${1:-all}" ;;
test)       cmd_test "${1:?name required}" ;;
all)        cmd_all "${1:?name required}" ;;
-h|--help|help) usage ;;
*)          usage; exit 1 ;;
esac
