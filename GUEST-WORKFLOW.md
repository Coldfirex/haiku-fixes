# Guest workflow (what actually works)

Do not invent remotes or re-apply a hunk that is already in the tree.
This is the flow that uploaded Gerrit 11824.

## Trees

- Haiku: `/boot/home/Desktop/sources/haiku`
- Fixes: `/boot/home/Desktop/sources/haiku-fixes`
- Identity: Alan Shearer / sakison@gmail.com
- Gerrit user: Coldfirex
- SSH key: `/boot/home/config/settings/ssh/id_ed25519`
- Gerrit SSH: `ssh://Coldfirex@git.haiku-os.org/haiku`

## Remotes on the Haiku tree (locked)

```
origin  https://review.haiku-os.org/haiku (fetch)
origin  ssh://Coldfirex@git.haiku-os.org/haiku (push)
```

Set push once:

```
git remote set-url --push origin ssh://Coldfirex@git.haiku-os.org/haiku
```

Never `git push` HTTPS. That prompts for a username. There is no
remote named `gerrit`. There is no `id_rsa`; the key is `id_ed25519`.
`GIT_SSH_COMMAND` is not required once the default key works.

SSH check:

```
ssh -o BatchMode=yes Coldfirex@git.haiku-os.org
```

Success is the Gerrit banner, then the connection closes.

## Clear old overlays (both trees)

`listimage` can show `/boot/system/non-packaged/...` even after
`~/config/non-packaged` is empty. Remove both:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu
rm -f ~/config/non-packaged/add-ons/accelerants/virtio_gpu.accelerant
rm -f ~/config/non-packaged/add-ons/kernel/drivers/network/virtio_net
rm -f ~/config/non-packaged/add-ons/kernel/network/datalink_protocols/arp
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/ipv4
rm -f ~/config/non-packaged/add-ons/kernel/network/protocols/udp
rm -f /boot/system/non-packaged/add-ons/kernel/network/protocols/udp
find ~/config/non-packaged/add-ons /boot/system/non-packaged/add-ons -type f
shutdown -r
```

After reboot, `listimage` paths must be `/boot/system/add-ons/...`.

## Apply once, then jam, then overlay

```
cd /boot/home/Desktop/sources/haiku-fixes && git pull
cd /boot/home/Desktop/sources/haiku
git checkout -- <touched files>
git apply /boot/home/Desktop/sources/haiku-fixes/<folder>/<name>.patch
cd generated && jam -q <target>
```

GPU kernel overlay that loads:

`~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`

Copy the jam output there, reboot, `listimage | grep virtio_gpu`.
Do not `open()` `/dev/graphics/` from a tester on a live desktop.

## Upload — do not apply a second time

If `git status` already shows the hunk (`4 ++++` on the one file),
**skip `git apply`**. That apply already ran for the jam/overlay.
A second apply fails with `patch does not apply`.

```
cd /boot/home/Desktop/sources/haiku
git diff --stat
git add <the one file>
git commit -m "<Subject from the .patch plus the body up to --->"
git log -1 --format=%B    # must contain Change-Id: I...
git push origin HEAD:refs/for/master%topic=<topic>
git reset --hard origin/master
```

New Change-Id every issue. Do not amend 11771 11776 11783 11784
11789 11791 11792 11824.

Gerrit commit text is the bug and the fix only. Review asides
(second open, future refcount) stay in the GitHub README.
