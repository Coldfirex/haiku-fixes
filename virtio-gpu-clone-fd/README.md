# virtio_gpu accelerant: do not close clone fd twice

Patch 3 of the virtio_gpu series. Status: **submitted**.

- Patch 1: Gerrit 11771 **MERGED** https://review.haiku-os.org/c/haiku/+/11771
  haiku.git `0438319c127429a416086d1220f79ff94d71f2d0`
  Change-Id `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Patch 2: Gerrit 11776 **MERGED** https://review.haiku-os.org/c/haiku/+/11776
  haiku.git `768d4e6cba315d55c9469c85d9219243408152d7`
  Change-Id `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- Patch 3: this change — Gerrit 11783 **NEW**
  https://review.haiku-os.org/c/haiku/+/11783
  Change-Id `I42d3007704c57958050eba997ba4537718e5f619`
- Later: `virtio-gpu-open-shared-area` (kernel driver; separate commit)

Do not amend 11771 or 11776. Do not re-push 11783 unless Gerrit asks for a new patch set.

## Patch notes

`virtio_gpu_clone_accelerant()` opens the device, `init_common(fd, true)`,
then `clone_area()` of the mode list. On mode-list failure it called
`uninit_common()` (already closes the fd) and fell through to `close(fd)`.

```
err2:
	uninit_common();
	return status;
err1:
	close(fd);
	return status;
```

## Test

Device node is `/dev/graphics/virtio/0` (`/dev/graphics/virtio` is a directory).

Do **not** run `./virtio_gpu_clone` on a live desktop. `virtio_gpu_open()`
re-inits the GPU on every open (new shared area, scanout, update thread).
A second open races app_server.

Safe smoke test:

```
listimage | grep accelerant
screenmode
screenmode 1920 1080 32
grep -a virtio_gpu /var/log/syslog | tail -40
```

Need `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant`,
Screen = VirtioGpu, `set_display_mode` with no command errors.
`err2` still needs a temporary fail-injection; do not commit that.

Jam target is `virtio_gpu.accelerant`. Overlay is accelerants/, not kernel/drivers/.
Do not use `on-haiku.sh go`.
