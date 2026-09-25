# syslog-remote-forward

Optional RFC 3164 UDP forwarder in `syslog_daemon`. Off unless
`syslog_server` is set. Not a syslog server.

Default install still writes only `/boot/system/var/log/syslog`.
This adds a third handler next to the file writer and the
`SYSLOG_ADD_LISTENER` fan-out.

v1 limits (intentional):

- IPv4 or IPv6 literal. No DNS.
- UDP only. Default port 514.
- `MSG_DONTWAIT`. Drop if there is no route or the send would block.
- No boot queue. Early lines are lost until the stack has a route.
- Settings are read once at daemon start.

Settings (`~/config/settings/kernel/drivers/kernel`, same file as
`syslog_time_stamps`):

```
syslog_server 192.168.1.10
syslog_server_port 514
```

Loopback check uses 127.0.0.1 and port 1514 so it does not need a
remote box.

## Build / overlay

This is a userspace server, not a kernel add-on.

```
cd $HAIKU_SRC
git apply /path/to/syslog-remote-forward.patch
cd generated
jam -qj2 syslog_daemon
```

Copy the built binary over the running one and restart the service:

```
cp $HAIKU_OUTPUT/objects/.../syslog_daemon \
	~/config/non-packaged/servers/syslog_daemon
# exact object path depends on the generated tree

launch_roster stop x-vnd.Haiku-SystemLogger
launch_roster start x-vnd.Haiku-SystemLogger
```

Or reboot after the overlay and settings are in place. The daemon
does not reload `syslog_server` on the fly.

If `~/config/non-packaged/servers/` does not shadow
`/boot/system/servers/syslog_daemon` on that nightly, replace the
packaged binary only for a smoke test and restore it after.

## Test

On Haiku, gcc 2.95 or gcc 13:

```
cd syslog-remote-forward
make
```

Terminal 1, after the patched daemon is running with
`syslog_server 127.0.0.1` and `syslog_server_port 1514`:

```
./syslog_remote_sink 1514
```

Terminal 2:

```
./syslog_remote_probe
```

Patched + configured: sink prints an RFC 3164 line that contains
`syslog-remote-forward-probe` and exits 0.

Stock daemon, or `syslog_server` unset: sink times out on recvfrom
and exits 1. The probe line still lands in `/system/var/log/syslog`.

A real collector (rsyslog, syslog-ng) on another host is the same
path. Point `syslog_server` at that host and use port 514.

## Gerrit

Do not submit until it has been run against a collector on the
guest. Topic if submitted: `syslog`. New Change-Id. Do not amend
any existing change.
