#!/bin/sh
# Prepare one haiku-fixes issue as a Gerrit change.
# Run on the machine that has the Haiku git tree and your Gerrit SSH key.
#
#   export HAIKU_SRC=$HOME/haiku
#   export GERRIT_USER=Coldfirex          # review.haiku-os.org username
#   ./gerrit.sh preflight
#   ./gerrit.sh commit virtio-gpu-detach-backing
#   ./gerrit.sh push virtio-gpu-detach-backing
#
# This never force-pushes to master. Push target is refs/for/master.

set -e

FIXES_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
HAIKU_SRC=${HAIKU_SRC:-${HAIKU_TOP:-$HOME/haiku}}
GERRIT_USER=${GERRIT_USER:-Coldfirex}
GERRIT_HOST=${GERRIT_HOST:-git.haiku-os.org}
GERRIT_PROJECT=${GERRIT_PROJECT:-haiku}
TOPIC_DEFAULT=virtio-gpu

usage() {
	cat <<EOF
usage: $0 <command> [name]

commands:
  preflight              check tree, hook, identity, SSH
  files <name>           list paths the patch touches
  apply <name>           git apply the issue patch (no commit)
  commit <name>          apply if needed, commit with the patch Subject
  push <name>            git push HEAD to refs/for/master
  status                 git status / log -1 in HAIKU_SRC

env:
  HAIKU_SRC      Haiku git tree     (default: \$HOME/haiku)
  GERRIT_USER    Gerrit username    (default: Coldfirex)
  GERRIT_HOST    git.haiku-os.org
  TOPIC          Gerrit topic       (default: virtio-gpu)
EOF
}

need_src() {
	if [ ! -d "$HAIKU_SRC/.git" ]; then
		echo "HAIKU_SRC is not a git tree: $HAIKU_SRC" >&2
		echo "export HAIKU_SRC=/path/to/haiku" >&2
		exit 1
	fi
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

subject_of() {
	awk '
		BEGIN { IGNORECASE = 1 }
		/^Subject:/ {
			sub(/^Subject:[[:space:]]*/, "")
			print
			exit
		}
	' "$1"
}

body_of() {
	awk '
		BEGIN { IGNORECASE = 1 }
		/^Subject:/ { grab = 1; next }
		grab && /^---$/ { exit }
		grab { print }
	' "$1"
}

touched_files() {
	awk '/^diff --git / {
		f = $3
		sub(/^a\//, "", f)
		print f
	}' "$1"
}

cmd_preflight() {
	need_src
	echo "== tree =="
	echo "HAIKU_SRC=$HAIKU_SRC"
	git -C "$HAIKU_SRC" rev-parse --abbrev-ref HEAD
	git -C "$HAIKU_SRC" log -1 --oneline

	echo
	echo "== identity (must match Gerrit) =="
	echo "user.name  = $(git -C "$HAIKU_SRC" config user.name)"
	echo "user.email = $(git -C "$HAIKU_SRC" config user.email)"
	echo "Check https://review.haiku-os.org/settings/#EmailAddresses"

	echo
	echo "== commit-msg hook =="
	hook=$HAIKU_SRC/.git/hooks/commit-msg
	if [ -x "$hook" ]; then
		echo "ok $hook"
	else
		echo "MISSING $hook"
		echo "Install with:"
		echo "  curl -Lo \"$hook\" https://review.haiku-os.org/tools/hooks/commit-msg"
		echo "  chmod +x \"$hook\""
	fi

	echo
	echo "== remote =="
	git -C "$HAIKU_SRC" remote -v
	echo "Expected origin push URL:"
	echo "  ssh://${GERRIT_USER}@${GERRIT_HOST}/${GERRIT_PROJECT}"

	echo
	echo "== SSH to Gerrit (no shell; a banner is success) =="
	ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new \
		"${GERRIT_USER}@${GERRIT_HOST}" gerrit version 2>/dev/null \
		|| ssh -o BatchMode=yes "${GERRIT_USER}@${GERRIT_HOST}" \
			true 2>&1 || true
}

cmd_files() {
	p=$(patch_file "$1")
	touched_files "$p"
}

cmd_apply() {
	need_src
	p=$(patch_file "$1")
	if git -C "$HAIKU_SRC" apply --check "$p"; then
		git -C "$HAIKU_SRC" apply "$p"
		echo "applied $1"
	else
		echo "apply --check failed for $1 (already applied, or tree drifted)" >&2
		exit 1
	fi
}

cmd_commit() {
	need_src
	name=$1
	p=$(patch_file "$name")
	hook=$HAIKU_SRC/.git/hooks/commit-msg
	if [ ! -x "$hook" ]; then
		echo "install the commit-msg hook first (./gerrit.sh preflight)" >&2
		exit 1
	fi

	if git -C "$HAIKU_SRC" apply --check "$p" 2>/dev/null; then
		git -C "$HAIKU_SRC" apply "$p"
		echo "applied $name"
	else
		echo "patch already applied or not applicable; committing current tree files"
	fi

	subject=$(subject_of "$p")
	if [ -z "$subject" ]; then
		echo "patch has no Subject: line" >&2
		exit 1
	fi

	tmp=$(mktemp)
	{
		printf '%s\n' "$subject"
		body_of "$p"
	} > "$tmp"

	for f in $(touched_files "$p"); do
		if [ ! -f "$HAIKU_SRC/$f" ]; then
			echo "missing $f" >&2
			exit 1
		fi
		git -C "$HAIKU_SRC" add -- "$f"
	done

	git -C "$HAIKU_SRC" diff --cached --stat
	git -C "$HAIKU_SRC" commit -F "$tmp"
	rm -f "$tmp"

	echo
	echo "commit created. Confirm Change-Id is present:"
	git -C "$HAIKU_SRC" log -1 --format='%B'
}

cmd_push() {
	need_src
	name=${1:-}
	topic=${TOPIC:-}
	if [ -z "$topic" ] && [ -n "$name" ]; then
		topic=$(printf '%s\n' "$name" | sed 's/-detach-backing//;s/-mutex-uninit//;s/-clone-fd//;s/-open-shared-area//')
		if [ -z "$topic" ]; then
			topic=$TOPIC_DEFAULT
		fi
	fi
	if [ -z "$topic" ]; then
		topic=$TOPIC_DEFAULT
	fi

	if ! git -C "$HAIKU_SRC" log -1 --format='%B' | grep -q '^Change-Id: I'; then
		echo "HEAD has no Change-Id. Re-run commit with the hook installed." >&2
		exit 1
	fi

	url="ssh://${GERRIT_USER}@${GERRIT_HOST}/${GERRIT_PROJECT}"
	echo "push $url HEAD:refs/for/master topic=$topic"
	git -C "$HAIKU_SRC" push "$url" HEAD:refs/for/master -o "topic=$topic"
}

cmd_status() {
	need_src
	git -C "$HAIKU_SRC" status
	echo
	git -C "$HAIKU_SRC" log -1
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

cmd=$1
shift
case $cmd in
preflight) cmd_preflight ;;
files)     cmd_files "${1:?name required}" ;;
apply)     cmd_apply "${1:?name required}" ;;
commit)    cmd_commit "${1:?name required}" ;;
push)      cmd_push "${1:-}" ;;
status)    cmd_status ;;
-h|--help|help) usage ;;
*) usage; exit 1 ;;
esac
