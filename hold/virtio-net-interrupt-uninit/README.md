# virtio_net: free interrupts if queue interrupt setup fails (hold)

Do not Gerrit. Never uploaded.

`VirtioQueue::SetupInterrupt()` stores the callback and always returns
`B_OK`. The `err8` / `free_interrupts` path does not run on current
Haiku. Same lesson as abandoned 11833.

The mutex patch is separate: `setup_interrupt()` can still fail.
