# virtio_gpu: destroy commandLock if interrupt setup fails

Patch 2 of the virtio_gpu series. Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11776 (NEW)
- Change-Id: `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- Topic: `virtio-gpu`
- Submitted: 2026-09-17

See `STATUS` in this folder. Do not re-push unless Gerrit asks for a
new patch set. The GitHub `.patch` is the originally submitted hunk.

Patch 1 (detach-backing) is already merged:
https://review.haiku-os.org/c/haiku/+/11771

`mutex_init(&info->commandLock)` runs before `setup_interrupt()` /
`queue_setup_interrupt()`. Those failures jump to `err3`, which only
deleted the command area and left `commandLock` initialized. `err2` is
also used when `get_memory_map()` fails, before the mutex exists, so
destroy it only on `err3`.

Rebuild-only. No userspace flooder. Interrupt-setup failure is not
something you can force from Screen preferences. Smoke-test that the
driver still loads.

Do not upload the GitHub `.patch` file as-is.

## Apply / jam (machine with the Haiku tree)

```
export HAIKU_SRC=$HOME/haiku
export HAIKU_OUTPUT=$HAIKU_SRC/generated

cd /path/to/haiku-fixes
./work.sh check virtio-gpu-mutex-uninit
./work.sh apply virtio-gpu-mutex-uninit
./work.sh jam virtio-gpu-mutex-uninit
```

On the Haiku guest the overlay path that loads is:

```
~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu
```

Not `drivers/bin` + `dev/graphics`. Reboot after copy.

## Runtime check (Haiku VM with virtio-gpu)

1. Confirm Screen preferences shows **VirtioGpu Driver**.
2. `listimage | grep virtio_gpu` shows the non-packaged graphics path.
3. Change resolution a few times.
4. Desktop still draws; syslog has no new virtio_gpu command errors.
