/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Userspace check for #18816: a non-permanent reject must be lifted
 * when the same protocol address is installed again without REJECT.
 *
 * Unpatched: ARP_SET_ENTRY ignores the new flags on an existing entry,
 * so REJECT sticks and GET_ENTRY (which requires VALID) fails.
 * Patched: REJECT is cleared, VALID is set, GET_ENTRY succeeds.
 *
 * Build on Haiku:
 *		make
 * Run (needs the ARP generic syscall, i.e. an IPv4 ethernet interface):
 *		./arp_reject_learn
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __HAIKU__
#	include <generic_syscall_defs.h>
#	include <syscalls.h>
#else
#	include <stdint.h>
	typedef int32_t status_t;
#	define B_OK 0
#endif

#define ARP_SYSCALLS			"network/arp"
#define ARP_SET_ENTRY			1
#define ARP_GET_ENTRY			2
#define ARP_DELETE_ENTRY		4

#define ARP_FLAG_REJECT			0x02
#define ARP_FLAG_VALID			0x10
#define ETHER_ADDRESS_LENGTH	6

struct arp_control {
	in_addr_t	address;
	uint8_t		ethernet_address[ETHER_ADDRESS_LENGTH];
	uint32_t	flags;
	uint32_t	cookie;
};

static const char* kAddress = "203.0.113.99";
static const uint8_t kMac[ETHER_ADDRESS_LENGTH] = {
	0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};


static status_t
ArpSyscall(uint32_t function, struct arp_control* control)
{
#ifdef __HAIKU__
	return _kern_generic_syscall(ARP_SYSCALLS, function, control,
		sizeof(*control));
#else
	(void)function;
	(void)control;
	fprintf(stderr, "arp_reject_learn: run on Haiku\n");
	return -1;
#endif
}


static void
FillControl(struct arp_control* control, uint32_t flags)
{
	memset(control, 0, sizeof(*control));
	control->address = inet_addr(kAddress);
	memcpy(control->ethernet_address, kMac, ETHER_ADDRESS_LENGTH);
	control->flags = flags;
}


int
main()
{
	struct arp_control control;
	status_t status;

#ifdef __HAIKU__
	uint32_t version = 0;
	status = _kern_generic_syscall(ARP_SYSCALLS, B_SYSCALL_INFO, &version,
		sizeof(version));
	if (status != B_OK) {
		fprintf(stderr, "ARP syscall is not available\n");
		return 1;
	}
#endif

	FillControl(&control, 0);
	ArpSyscall(ARP_DELETE_ENTRY, &control);

	FillControl(&control, ARP_FLAG_REJECT);
	status = ArpSyscall(ARP_SET_ENTRY, &control);
	if (status != B_OK) {
		fprintf(stderr, "ARP_SET_ENTRY reject failed: %s\n", strerror(status));
		return 1;
	}

	FillControl(&control, 0);
	status = ArpSyscall(ARP_SET_ENTRY, &control);
	if (status != B_OK) {
		fprintf(stderr, "ARP_SET_ENTRY learn failed: %s\n", strerror(status));
		return 1;
	}

	FillControl(&control, 0);
	status = ArpSyscall(ARP_GET_ENTRY, &control);
	if (status != B_OK) {
		fprintf(stderr, "ARP_GET_ENTRY after learn failed: %s\n"
			"(unpatched stack leaves REJECT set and VALID clear)\n",
			strerror(status));
		ArpSyscall(ARP_DELETE_ENTRY, &control);
		return 1;
	}

	if ((control.flags & ARP_FLAG_REJECT) != 0) {
		fprintf(stderr, "REJECT still set after learn, flags=0x%x\n",
			control.flags);
		ArpSyscall(ARP_DELETE_ENTRY, &control);
		return 1;
	}
	if ((control.flags & ARP_FLAG_VALID) == 0) {
		fprintf(stderr, "VALID not set after learn, flags=0x%x\n",
			control.flags);
		ArpSyscall(ARP_DELETE_ENTRY, &control);
		return 1;
	}

	printf("reject lifted, flags=0x%x\n", control.flags);
	ArpSyscall(ARP_DELETE_ENTRY, &control);
	return 0;
}
