# virtio_gpu accelerant: do not close clone fd twice

Patch 3 of the virtio_gpu series. Status: **merged**.

- Patch 1: Gerrit 11771 **MERGED** https://review.haiku-os.org/c/haiku/+/11771
  haiku.git `0438319c127429a416086d1220f79ff94d71f2d0`
- Patch 2: Gerrit 11776 **MERGED** https://review.haiku-os.org/c/haiku/+/11776
  haiku.git `768d4e6cba315d55c9469c85d9219243408152d7`
- Patch 3: this change — Gerrit 11783 **MERGED**
  https://review.haiku-os.org/c/haiku/+/11783
  haiku.git `55d56e03a0834af356f883d3e148284cd9d286e9`
  Change-Id `I42d3007704c57958050eba997ba4537718e5f619`
- Leftover: `virtio-gpu-open-shared-area` (kernel driver; still local)

Do not re-push 11771, 11776, or 11783.
