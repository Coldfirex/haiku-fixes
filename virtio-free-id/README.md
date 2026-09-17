# virtio_net: free device id if publish_device fails

This is the **virtio_net** id-leak fix (`virtio-free-id`). Status: **local**.

It is **not** virtio_gpu patch 4. The numbered GPU series is only:

- GPU 1: `virtio-gpu-detach-backing` — Gerrit 11771 **MERGED**
- GPU 2: `virtio-gpu-mutex-uninit` — Gerrit 11776 **MERGED**
- GPU 3: `virtio-gpu-clone-fd` — Gerrit 11783 **NEW**
- GPU next (unnumbered leftover): `virtio-gpu-open-shared-area` — still local

Sibling virtio_net change, do not bundle: `virtio-tx-freelist`.

Do not amend 11771, 11776, or 11783. Do not upload the GitHub `.patch`
as-is. Commit inside the Haiku tree so the hook adds a **new** Change-Id.

---

## Lessons from the GPU uploads (still apply)

Same guest, same helpers, same Gerrit account. Different driver.

1. Trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`, not `$HOME/haiku`.
2. Always `export HAIKU_SRC`. Helpers do not search Desktop.
3. `chmod +x work.sh gerrit.sh on-haiku.sh` after pull.
4. Pull haiku-fixes first. Older clones of this folder only had the `.patch`.
5. `gerrit.sh` is often missing from the guest clone. Commit/push by hand
   if `ls gerrit.sh` fails.
6. Start from `origin/master`. Do not stack on a leftover GPU commit.
7. New commit, new Change-Id. Never paste the GitHub `.patch` into the
   Gerrit UI. Never `git push origin master`.
8. Identity: Alan Shearer / sakison@gmail.com, user `Coldfirex`.
   SSH: `/boot/home/config/settings/ssh/`.
9. Kernel C++98. This hunk is two lines.

### Overlay is *not* the GPU path

10. GPU lesson was: `on-haiku.sh go` installs
    `drivers/bin` + `dev/graphics`, and **graphics/virtio_gpu** does not
    load from there.
11. This module is `virtio_net`. The published node is `/dev/net/virtio/N`.
    The overlay that matches that class is the one `on-haiku.sh` already
    uses for `virtio_net`:
    - `~/config/non-packaged/add-ons/kernel/drivers/bin/virtio_net`
    - `~/config/non-packaged/add-ons/kernel/drivers/dev/net/virtio_net`
      → symlink to `../../bin/virtio_net`
12. Do **not** copy `virtio_net` into `drivers/graphics/` or
    `add-ons/accelerants/`.
13. Still skip `on-haiku.sh go` for this upload. Copy by hand so a GPU
    leftover path does not get mixed in.
14. `listimage | grep virtio_net` must show the non-packaged bin path,
    or you tested stock.
15. Network driver reload still needs a reboot (or a clean module
    unload, which we have not been doing).

---

## What the hunk does

`register_child_devices()`:

1. `create_id(VIRTIO_NET_DEVICE_ID_GENERATOR)` — one bit, max 64
2. builds `net/virtio/%lu` from that id
3. `publish_device(...)`
4. returns `status`

Unpatched: a failed `publish_device` leaves the bit reserved.
Repeated failed probes can exhaust the generator, after which new
NICs never get a `/dev/net/virtio/N` name.

Patched: `free_id` on the non-`B_OK` path, then return the original
`publish_device` status. Do not check or return `free_id()`.

Rebuild-only. You cannot force `publish_device` to fail from
ifconfig. Smoke-test that the NIC still publishes and gets an IP.

---

## What a test can prove

| Path | How | Proves |
|---|---|---|
| probe success | boot with virtio-net | `/dev/net/virtio/0` exists; overlay loaded |
| traffic | ping / DHCP | driver still works |
| publish fail | fail-injection only | `free_id` ran; not this smoke test |

---

## Guest paths

```
HAIKU_SRC=/boot/home/Desktop/sources/haiku
HAIKU_OUTPUT=/boot/home/Desktop/sources/haiku/generated
FIXES=/boot/home/Desktop/sources/haiku-fixes
NET_BIN=$HOME/config/non-packaged/add-ons/kernel/drivers/bin
NET_DEV=$HOME/config/non-packaged/add-ons/kernel/drivers/dev/net
```

---

## 0. One-time (skip if a GPU patch already did this)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
export GERRIT_USER=Coldfirex
export GIT_AUTHOR_NAME='Alan Shearer'
export GIT_AUTHOR_EMAIL='sakison@gmail.com'

cd /boot/home/Desktop/sources/haiku-fixes
chmod +x work.sh on-haiku.sh
chmod +x gerrit.sh 2>/dev/null || true

hook=$HAIKU_SRC/.git/hooks/commit-msg
if [ ! -x "$hook" ]; then
	curl -Lo "$hook" https://review.haiku-os.org/tools/hooks/commit-msg
	chmod +x "$hook"
fi

git -C "$HAIKU_SRC" config user.name "$GIT_AUTHOR_NAME"
git -C "$HAIKU_SRC" config user.email "$GIT_AUTHOR_EMAIL"
```

`ssh -o BatchMode=yes Coldfirex@git.haiku-os.org` must print a
Gerrit banner.

---

## 1. Pull latest sources

```
cd /boot/home/Desktop/sources/haiku-fixes
git pull --ff-only
ls virtio-free-id
# expect: README.md STATUS virtio_net-free-id-on-publish-fail.patch

cd /boot/home/Desktop/sources/haiku
git fetch origin
git checkout master
git pull --rebase origin master
```

If guest `master` still has a local 11783 / open-shared-area commit:

```
git branch keep/local-gpu
git reset --hard origin/master
```

```
git status
git log -1 --oneline
git -C "$HAIKU_SRC" diff --stat
```

Clean tree. Diff must not include `virtio_gpu.cpp` or `accelerant.cpp`.

```
git -C "$HAIKU_SRC" grep -n 'free_id(VIRTIO_NET_DEVICE_ID_GENERATOR' -- \
	src/add-ons/kernel/drivers/network/ether/virtio/virtio_net.cpp
# expect no matches on clean master (the leak)
```

---

## 2. Apply (no commit yet)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
cd /boot/home/Desktop/sources/haiku-fixes

./work.sh check virtio-free-id
./work.sh apply virtio-free-id
./work.sh files virtio-free-id
```

`files` must print only:

```
src/add-ons/kernel/drivers/network/ether/virtio/virtio_net.cpp
```

```
git -C "$HAIKU_SRC" diff -- \
	src/add-ons/kernel/drivers/network/ether/virtio/virtio_net.cpp
```

Exactly two added lines after `publish_device`:

```
	if (status != B_OK)
		sDeviceManager->free_id(VIRTIO_NET_DEVICE_ID_GENERATOR, id);
```

No TX freelist hunk. No `err7`. If `apply --check` fails, stop.

---

## 3. Jam and overlay (manual copy)

```
cd /boot/home/Desktop/sources/haiku-fixes
./work.sh jam virtio-free-id
```

```
BIN=$(find "$HAIKU_OUTPUT/objects" -type f -path '*/release/*' -name virtio_net | head -1)
echo "$BIN"
ls -l "$BIN"

mkdir -p ~/config/non-packaged/add-ons/kernel/drivers/bin
mkdir -p ~/config/non-packaged/add-ons/kernel/drivers/dev/net
copyattr --data "$BIN" ~/config/non-packaged/add-ons/kernel/drivers/bin/virtio_net
chmod +x ~/config/non-packaged/add-ons/kernel/drivers/bin/virtio_net
ln -sfn ../../bin/virtio_net \
	~/config/non-packaged/add-ons/kernel/drivers/dev/net/virtio_net
```

`cp -f` if `copyattr` is missing.

Do not touch:

- `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`
- `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`

```
reboot
```

---

## 4. Test

### 4a. Overlay is the one loaded

```
listimage | grep -i virtio_net
ls /dev/net/virtio
ifconfig
```

Need the non-packaged `drivers/bin/virtio_net` path and
`/dev/net/virtio/0` (or `/1`, …). Packaged-only path means stock.

### 4b. Success-path smoke (does not prove free_id ran)

```
ping -c 3 1.1.1.1
grep -a virtio_net /var/log/syslog | tail -40
```

Link up, no new probe/publish errors. A working NIC does not exercise
the `publish_device` failure path.

### 4c. The actual bug (optional, do not commit)

In `register_child_devices`, after `publish_device` assign a fake
failure before the new `if`:

```
	status = B_ERROR;
	if (status != B_OK)
		sDeviceManager->free_id(VIRTIO_NET_DEVICE_ID_GENERATOR, id);
```

That hides the real NIC. Only use it to confirm the two new lines
compile and that a failed publish does not leave you unable to probe
again after you revert the injection.

Delete the `status = B_ERROR` line before step 5.

---

## 5. Gerrit (new change, new Change-Id, topic virtio-net)

Working tree must contain only the two `free_id` lines. No
tx-freelist, no GPU hunks, no fail-injection.

Subject from the patch:

```
virtio_net: free device id if publish_device fails
```

### 5a. If guest `haiku-fixes` has `gerrit.sh`

`gerrit.sh push` derives a topic from the folder name and would
leave `virtio-free-id`. Override it:

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export GERRIT_USER=Coldfirex
export TOPIC=virtio-net
cd /boot/home/Desktop/sources/haiku-fixes

./gerrit.sh preflight
./gerrit.sh commit virtio-free-id
./gerrit.sh push virtio-free-id
```

### 5b. If `gerrit.sh` is missing (same as GPU patch 3)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
cd "$HAIKU_SRC"

git add -- src/add-ons/kernel/drivers/network/ether/virtio/virtio_net.cpp
git diff --cached --stat
# must be only virtio_net.cpp, two lines

git commit -F - <<'EOF'
virtio_net: free device id if publish_device fails

create_id() reserves a bit in a per-generator bitmap, limited to 64
IDs. The ID is used only as the /dev/net/virtio/N suffix. If
publish_device() fails, the bit was not released, so repeated failed
probes could exhaust the generator.

Do not check or return free_id(); keep the original publish_device
status.
EOF
```

The hook must append `Change-Id: I...`. Do not type one by hand.

```
git -C "$HAIKU_SRC" log -1 --format='%B'
git -C "$HAIKU_SRC" show --stat
```

- New `Change-Id: I...`
- Not `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Not `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- Not `I42d3007704c57958050eba997ba4537718e5f619`
- `--stat` is only `virtio_net.cpp`

```
git -C "$HAIKU_SRC" push ssh://Coldfirex@git.haiku-os.org/haiku \
	HEAD:refs/for/master -o topic=virtio-net
```

Save the printed URL in `STATUS` (`state: submitted`).

Do not amend the GPU changes. Do not `git push origin master`.

---

## 6. Reset after push

```
cd /boot/home/Desktop/sources/haiku
git fetch origin
git checkout master
git reset --hard origin/master
```

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/bin/virtio_net
rm -f ~/config/non-packaged/add-ons/kernel/drivers/dev/net/virtio_net
# reboot
```

Leave any GPU overlays alone.

---

## Do not

- Treat this as virtio_gpu patch 4 or put it on topic `virtio-gpu`.
- Bundle `virtio-tx-freelist` in the same commit.
- Amend 11771, 11776, or 11783.
- Paste the GitHub `.patch` into the Gerrit web UI.
- Copy this binary into `drivers/graphics/` or `accelerants/`.
- `on-haiku.sh go virtio-free-id`
- Commit fail-injection.
