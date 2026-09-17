# virtio_gpu: initialize detach_backing command

Patch 1 of the virtio_gpu series. Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11771 (MERGED)
- haiku.git: `0438319c127429a416086d1220f79ff94d71f2d0`
- Change-Id: `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Topic: `virtio-gpu`
- Submitted: 2026-09-16
- Merged: 2026-09-17 by korli (Code-Review +2 waddlesplash)

See `STATUS` in this folder. The change is in haiku master; do not
re-push it. The GitHub `.patch` is the originally submitted hunk.

Patch: `virtio-gpu-detach-backing.patch`

`struct virtio_gpu_resource_detach_backing backing` was left uninitialized.
`send_cmd()` overwrites `fence_id` but ORs `flags`, so `hdr.flags`, `ctx_id`,
and padding could be sent to the device as stack garbage. Zero-init with `{}`
like the other commands in the file.

Rebuild-only. No userspace flooder.

## Runtime check (Haiku VM with virtio-gpu)

1. Confirm Screen preferences shows **VirtioGpu Driver**.
2. Change resolution a few times (detach/attach backing).
3. `grep virtio_gpu /var/log/syslog` — no new command errors.
4. Desktop still draws after the mode switch.

## Gerrit

Merged. Do not push this hunk again.

https://review.haiku-os.org/c/haiku/+/11771
https://github.com/haiku/haiku/commit/0438319c127429a416086d1220f79ff94d71f2d0
