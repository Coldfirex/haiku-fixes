# virtio_gpu accelerant: do not close clone fd twice

Patch 3 of the virtio_gpu series. Status: **local** (not on Gerrit yet).

- Patch 1: `virtio-gpu-detach-backing` — Gerrit 11771 **MERGED**
  https://review.haiku-os.org/c/haiku/+/11771
  haiku.git `0438319c127429a416086d1220f79ff94d71f2d0`
  Change-Id `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Patch 2: `virtio-gpu-mutex-uninit` — Gerrit 11776 **NEW**
  https://review.haiku-os.org/c/haiku/+/11776
  Change-Id `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- Patch 3: this change (`virtio-gpu-clone-fd`) — **accelerant only**
- Later: `virtio-gpu-open-shared-area` (kernel driver; separate commit)

## Patch notes — still good

The hunk is still the right fix. `virtio_gpu_clone_accelerant()`:

1. `open()` the graphics device
2. `init_common(fd, true)` — sets `is_clone`, stores `fd` in `gInfo->device`
3. `clone_area()` of the mode list
4. On mode-list failure: `uninit_common()` (already `close(gInfo->device)`) then fell through to `close(fd)` again

One added line under `err2:`:

```
err2:
	uninit_common();
	return status;
err1:
	close(fd);
	return status;
```

Do not upload the GitHub `.patch` as-is. Commit inside the Haiku tree so the `commit-msg` hook adds a **new** Change-Id.

## What a test can prove

| Path | How | Proves |
|---|---|---|
| `INIT_ACCELERANT` | login / app_server | desktop is VirtioGpu |
| clone success | `./virtio_gpu_clone` or BDirectWindow | overlay loaded; clone still works |
| clone `err1` | `init_common` failed | `close(fd)` only; not this bug |
| clone `err2` | fail-injection only | the added `return status` |

Screen prefs and a clean desktop never take `err2`.

## Still true from patches 1–2 (do not ignore)

- Guest trees: `/boot/home/Desktop/sources/{haiku,haiku-fixes}`
- Always `export HAIKU_SRC` (helpers do not search Desktop)
- `chmod +x work.sh gerrit.sh on-haiku.sh`
- Do **not** use `on-haiku.sh go` / `overlay` for this series
- Kernel overlay (patches 1–2 only):
  `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`
- **This** overlay:
  `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`
- Jam target is `virtio_gpu.accelerant`, not `virtio_gpu`
- Start from `origin/master` (11771 is already there). Do not amend 11771 or 11776.
- SSH keys: `/boot/home/config/settings/ssh/`

```
HAIKU_SRC=/boot/home/Desktop/sources/haiku
HAIKU_OUTPUT=/boot/home/Desktop/sources/haiku/generated
FIXES=/boot/home/Desktop/sources/haiku-fixes
ACCEL_OVERLAY=$HOME/config/non-packaged/add-ons/accelerants
```

---

## 0. One-time (skip if patch 2 already did this)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
export GERRIT_USER=Coldfirex
export GIT_AUTHOR_NAME='Alan Shearer'
export GIT_AUTHOR_EMAIL='sakison@gmail.com'

cd /boot/home/Desktop/sources/haiku-fixes
chmod +x work.sh gerrit.sh on-haiku.sh

hook=$HAIKU_SRC/.git/hooks/commit-msg
if [ ! -x "$hook" ]; then
	curl -Lo "$hook" https://review.haiku-os.org/tools/hooks/commit-msg
	chmod +x "$hook"
fi

git -C "$HAIKU_SRC" config user.name "$GIT_AUTHOR_NAME"
git -C "$HAIKU_SRC" config user.email "$GIT_AUTHOR_EMAIL"
```

`./gerrit.sh preflight` must print a Gerrit banner, not `Permission denied`.

---

## 1. Pull latest sources

Guest `haiku-fixes` must have this folder's `README.md`, `Makefile`, and
`virtio_gpu_clone.c`. Older clones only had the `.patch`. Pull first:

```
cd /boot/home/Desktop/sources/haiku-fixes
git pull --ff-only
ls virtio-gpu-clone-fd
# expect: README.md STATUS Makefile virtio_gpu_clone.c virtio-gpu-clone-fd.patch

cd /boot/home/Desktop/sources/haiku
git fetch origin
git checkout master
git pull --rebase origin master
```

If guest `master` still has a local 11776 commit:

```
git branch keep/11776-mutex-uninit
git reset --hard origin/master
```

```
git status
git log -1 --oneline
git -C "$HAIKU_SRC" diff --stat
```

Clean tree. Detach-backing is already in master. Do not include
`virtio_gpu.cpp` in this commit. 11776 does not need to merge first.

---

## 2. Apply (no commit yet)

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated
cd /boot/home/Desktop/sources/haiku-fixes

./work.sh check virtio-gpu-clone-fd
./work.sh apply virtio-gpu-clone-fd
./work.sh files virtio-gpu-clone-fd
```

`files` must print only:

```
src/add-ons/accelerants/virtio/accelerant.cpp
```

```
git -C "$HAIKU_SRC" diff -- src/add-ons/accelerants/virtio/accelerant.cpp
```

Exactly one added `return status;` under `err2:`. If `apply --check`
fails, stop.

---

## 3. Jam and overlay (manual copy)

```
cd /boot/home/Desktop/sources/haiku-fixes
./work.sh jam virtio-gpu-clone-fd
```

```
BIN=$(find "$HAIKU_OUTPUT/objects" -type f -path '*/release/*' -name virtio_gpu.accelerant | head -1)
echo "$BIN"
ls -l "$BIN"

mkdir -p ~/config/non-packaged/add-ons/accelerants
copyattr --data "$BIN" ~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant
chmod +x ~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant
```

`cp -f` if `copyattr` is missing.

Drop the **wrong** kernel overlay if it is still there from `on-haiku.sh go`:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/bin/virtio_gpu
rm -f ~/config/non-packaged/add-ons/kernel/drivers/dev/graphics/virtio_gpu
```

Do not copy this build into `kernel/drivers/graphics/`.

Reload the accelerant (userspace; kernel module not required):

```
reboot
```

---

## 4. Test

### 4a. Overlay is the one loaded

```
listimage | grep -i accelerant
screenmode
```

Need `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`
and Screen = VirtioGpu. Packaged-only path means you tested the stock
binary.

### 4b. Clone success path (do this)

```
cd /boot/home/Desktop/sources/haiku-fixes/virtio-gpu-clone-fd
make
./virtio_gpu_clone 50
```

Expect `ok clone success path`, fd count flat. The program prints the
add-on path it loaded — that must be the non-packaged overlay.

`TRACE_ACCELERANT` is on, so syslog should show:

```
grep virtio_gpu_clone_accelerant /var/log/syslog | tail
```

Optional extra clone team: GLTeapot / Game Kit `BDirectWindow`.

Desktop smoke (does not prove `err2`): change resolution a couple of
times, desktop still draws, no new `virtio_gpu` command errors:

```
grep -i virtio_gpu /var/log/syslog | tail -40
```

### 4c. The actual bug (optional, do not commit)

After `init_common` succeeds in `virtio_gpu_clone_accelerant()`:

```
	status = B_NO_MEMORY;
	goto err2;
```

Jam + overlay again, then `./virtio_gpu_clone 1`.

- Unpatched: returns `B_NO_MEMORY` and `close(fd)` twice. Usually
  `EBADF` on the second close, not a KDL. Risk is fd reuse.
- Patched: same `B_NO_MEMORY`, one close inside `uninit_common()`.

Delete those two lines before step 5.

---

## 5. Gerrit (new change, new Change-Id)

Working tree must contain only the clone-fd hunk. No mutex-uninit,
no open-shared-area, no fail-injection.

```
export HAIKU_SRC=/boot/home/Desktop/sources/haiku
export GERRIT_USER=Coldfirex
cd /boot/home/Desktop/sources/haiku-fixes

./gerrit.sh preflight
./gerrit.sh commit virtio-gpu-clone-fd
```

Subject from the patch:

```
virtio_gpu accelerant: do not close clone fd twice
```

```
git -C "$HAIKU_SRC" log -1 --format='%B'
git -C "$HAIKU_SRC" show --stat
```

- New `Change-Id: I...`
- Not `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Not `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- `--stat` is only `accelerant.cpp`

```
./gerrit.sh push virtio-gpu-clone-fd
```

```
git push ssh://Coldfirex@git.haiku-os.org/haiku HEAD:refs/for/master -o topic=virtio-gpu
```

Save the printed URL in `STATUS` (`state: submitted`).

Do not amend 11771 or 11776. Do not `git push origin master`.

---

## 6. Reset after push

```
cd /boot/home/Desktop/sources/haiku
git fetch origin
git checkout master
git reset --hard origin/master
```

```
rm -f ~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant
# reboot
```

---

## Do not

- Amend 11771 or 11776.
- Paste the GitHub `.patch` into the Gerrit web UI.
- `on-haiku.sh try 11771` / `try 11776` as the base for this commit.
- `on-haiku.sh go virtio-gpu-clone-fd`
- Overlay the accelerant under `kernel/drivers/`
- Jam / overlay a kernel `virtio_gpu` you did not rebuild for this patch
- Commit fail-injection or `open-shared-area` / mutex-uninit in this change
