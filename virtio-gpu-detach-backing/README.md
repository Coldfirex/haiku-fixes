# virtio_gpu: initialize detach_backing command

Patch 1 of the virtio_gpu series. Status: **submitted**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11771 (NEW, Code-Review +2)
- Change-Id: `I043945302ef10c2a2911faf037ca534759bd3ee0`
- Patch set 1: `4163d8e4b7467e050693cd352082550454cafaff`
- Topic: `virtio-gpu`
- Submitted: 2026-09-16

See `STATUS` in this folder. Flip `state` to `merged` and fill `hrev` /
`haiku-commit` when Gerrit lands it. Do not treat the GitHub `.patch` as
the Gerrit upload; that change already lives on review.haiku-os.org.

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

Already uploaded. Do not push a second change for this hunk, and do not
upload the GitHub `.patch` as-is.

https://review.haiku-os.org/c/haiku/+/11771

If you need a new patch set on the same Change-Id, amend the existing
commit in the Haiku tree (keep `I043945302ef10c2a2911faf037ca534759bd3ee0`)
and push to `refs/for/master`.
