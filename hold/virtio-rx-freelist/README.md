# virtio_net: keep RX BufInfo if enqueue fails (hold)

Do not Gerrit. Never uploaded.

queue_request_v returns B_BUSY when the ring is full. RX is built not
to hit that: rxSizes is queue_size/2, each buffer uses two descriptors,
and receive() only re-queues after a dequeue. drain_queues runs from
free(); the BufInfos are still in rxBufInfos[] and deleted on uninit.

Same lesson as abandoned 11833. Keep merged TX freelist (11807).
