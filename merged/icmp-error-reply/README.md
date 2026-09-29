# icmp-error-reply

Status: **merged** [Gerrit 11869](https://review.haiku-os.org/c/haiku/+/11869) / `a4aa8679`.
Do not re-push. Do not apply on current master.

- Change-Id: `I669c618158f6b60a9d8f9188a95c3eb550176093`
- Topic: `icmp`
- Code-Review: +2 Philippe Houdoin
- Merged: 2026-09-28

icmp_error_reply() creates a reply buffer, then returns B_ERROR if
get_domain() is NULL without freeing it. The same function wrote the
ICMP header through NetBufferPrepend without checking Status(). The
prepend can fail, so do not access the header until its status has
been checked.

Declare status_t once. On a NULL domain set status and fall through
to the existing free at the end of the function.

The cause buffer is still owned by the caller.
