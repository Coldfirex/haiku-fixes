# Guest workflow (pull, baseline, apply, test, Gerrit)

This is the general method for any Haiku add-on change in this repo.
Issue folders only hold the patch, tester, and STATUS. Do not copy
this file into those folders.

Do not invent remotes. Do not apply a hunk that is already in the
tree. This is the flow that uploaded Gerrit 11824.

## Trees and remotes

- Haiku: `/boot/home/Desktop/sources/haiku`
- Fixes: `/boot/home/Desktop/sources/haiku-fixes`
- Identity: Alan Shearer / sakison@gmail.com
- Gerrit user: Coldfirex
- SSH key: `/boot/home/config/settings/ssh/id_ed25519`
- Gerrit SSH: `ssh://Coldfirex@git.haiku-os.org/haiku`

On the Haiku tree:

```
origin  https://review.haiku-os.org/haiku (fetch)
origin  ssh://Coldfirex@git.haiku-os.org/haiku (push)
```

Set push once:

```
git remote set-url --push origin ssh://Coldfirex@git.haiku-os.org/haiku
```

Never `git push` HTTPS. There is no remote named `gerrit`. There is
no `id_rsa`. `GIT_SSH_COMMAND` is not required once the default key
works.

```
ssh -o BatchMode=yes Coldfirex@git.haiku-os.org
```

Success is the Gerrit banner, then the connection closes.

## Order (do not skip)

1. Drop every leftover overlay and reboot onto **packaged** modules.
2. Build and run the tester against that packaged module (baseline).
3. Only then apply the patch, jam, copy the overlay, reboot.
4. Confirm `listimage` shows the non-packaged path.
5. Run the same tester again (patched).
6. Commit from the already-applied tree and push Gerrit.
7. `git reset --hard origin/master`. Remove the overlay when done.

If a step needs a packaged baseline, it is written **before** the
overlay copy. Never say “compare stock vs patched” after
`listimage` already shows `~/config/non-packaged/...`.

## 1. Packaged modules only

`listimage` can still show `/boot/system/non-packaged/...` after
`~/config/non-packaged` is empty. Remove both trees:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu
rm -f ~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant
rm -f ~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net
rm -f ~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/ipv4
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/udp
rm -f /boot/system/non-packaged/add-ons/kernel/network/protocols/udp
rm -f ~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block
rm -f ~/config/non-packaged/add-ons/kernel/busses/scsi/virtio
find ~/config/non-packaged/add-ons /boot/system/non-packaged/add-ons -type f
shutdown -r
```

After reboot:

```
listimage | grep add-ons/kernel
```

Paths must be `/boot/system/add-ons/...`. If any line still says
`non-packaged`, stop. Do not run a tester and call it stock.

## 2. Baseline on packaged (required before overlay)

Pull notes, keep the Haiku tree clean, build the userspace tester
only. Do **not** `git apply` yet. Do **not** copy a module into
`non-packaged`.

```
cd /boot/home/Desktop/sources/haiku-fixes && git pull
cd /boot/home/Desktop/sources/haiku
git fetch origin
git checkout master
git reset --hard origin/master
git status
```

Then, from the issue folder (example names only):

```
cd /boot/home/Desktop/sources/haiku-fixes/<folder>
make
./<tester>
```

Write down used pages / syslog / pass-fail. That is the stock line.

### Testers that inject packets

Raw ICMP / UDP flood loops can hard-lock this QEMU guest (SSH dies,
ping dies, Proxmox console mouse frozen). That is not proof of the
bug and it is not “stock vs patched”.

- Loopback only (`127.0.0.1`). Never another host.
- Start with 3–5 seconds, not 20.
- If the tester has no delay between `sendto`s, do not leave it
  running unattended. Reset the VM from Proxmox if the console
  dies; do not wait for ACPI.
- A leak test is “pages climb over a few seconds”, not “line-rate
  until the guest wedges”.

## 3. Apply once, jam, overlay, reboot

One issue at a time. Do not stack ipv4 + udp + tcp patches.

```
cd /boot/home/Desktop/sources/haiku
git checkout -- <touched files>
git apply /boot/home/Desktop/sources/haiku-fixes/<folder>/<name>.patch
git diff --stat
cd generated && jam -q <target>
```

Copy the jam output to the overlay that actually loads, then reboot.

| Kind | jam target | Overlay path |
| --- | --- | --- |
| UDP | `udp` | `~/config/non-packaged/add-ons/kernel/network/protocols/udp` |
| IPv4 | `ipv4` | `~/config/non-packaged/add-ons/kernel/network/protocols/ipv4` |
| ARP | `'<module>arp'` | `~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp` |
| virtio_net | `virtio_net` | `~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net` |
| virtio_gpu kernel | `virtio_gpu` | `~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu` |
| virtio_gpu accelerant | `virtio_gpu.accelerant` | `~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant` |
| virtio_block | `virtio_block` | `~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block` |

Example UDP:

```
mkdir -p ~/config/non-packaged/add-ons/kernel/network/protocols
cp /boot/home/Desktop/sources/haiku/generated/objects/haiku/x86_64/release/add-ons/kernel/network/protocols/udp/udp \
   ~/config/non-packaged/add-ons/kernel/network/protocols/udp
shutdown -r
```

After reboot:

```
listimage | grep network/protocols/udp
```

Must show `/boot/home/config/non-packaged/add-ons/kernel/network/protocols/udp`.
If it still shows `/boot/system/add-ons/...`, the overlay did not
load; do not call the next run “patched”.

Do not `open()` `/dev/graphics/` from a tester on a live desktop.
Do not use `on-haiku.sh go` for overlays.

## 4. Retest on the overlay

Same tester, same arguments as step 2.

```
cd /boot/home/Desktop/sources/haiku-fixes/<folder>
./<tester>
```

Compare to the packaged numbers from step 2. That is the only
stock-vs-patched pair that counts.

## 5. Upload — do not apply a second time

If `git status` already shows the hunk, **skip `git apply`**.
A second apply fails with `patch does not apply`.

```
cd /boot/home/Desktop/sources/haiku
git diff --stat
git add <the one file>
git commit -F /tmp/<issue>.msg
git log -1 --format=%B
git push origin HEAD:refs/for/master%topic=<topic>
git reset --hard origin/master
```

`git log -1` must contain a new `Change-Id: I...`. Do not amend
11771 11776 11783 11784 11789 11791 11792 11807 11824 11827 11843 11851.

Do not run bare `git commit` in the guest; Haiku often leaves
`.git/COMMIT_EDITMSG` empty. Use `git commit -F` from the `.patch` body.

Gerrit body is the bug and the fix only. Testers, overlay paths,
and review asides stay on GitHub.

After upload the Haiku tree is clean again. The overlay file in
`~/config/non-packaged` stays until you delete it and reboot.

## 6. Drop the overlay

```
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/udp
# plus any other overlay you copied
shutdown -r
```

Update `STATUS` in haiku-fixes and `git push origin main`.

## If the guest hard-locks

SSH gone, no ping, Proxmox console mouse dead: Reset the VM from
Proxmox. That is the same class of lock as earlier idle-session
freezes plus an unthrottled packet injector. It is not a Gerrit
result by itself. After reset, start again at step 1 so you are
not guessing which module was loaded.
