# ipv4-error-received (abandoned)

Do not Gerrit. Do not apply.

ipv4_error_received only returns errors on the paths this patch
freed. The stack loop consumer already frees the buffer when
receive_data fails. Free-then-error is a double-free.

The success path still hands the buffer to the next protocol.
