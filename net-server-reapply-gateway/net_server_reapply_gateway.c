/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * After ifconfig down / up the kernel restores the host and subnet
 * routes but not the saved default gateway. net_server should put
 * that route back for a static profile.
 *
 * Build on Haiku (gcc 2.95 or gcc 13):
 *		make
 * Run (needs a static IPv4 iface with a gateway in interfaces):
 *		./net_server_reapply_gateway
 *		./net_server_reapply_gateway /dev/net/virtio/0 192.168.250.254
 *
 * Unpatched: after up, no default route (exit 2).
 * Patched net_server: default route matches the saved gateway.
 *
 * This bounces IFF_UP on the chosen interface.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static const char* kInterfacesPath
	= "/boot/system/settings/network/interfaces";


static void
Trim(char* line)
{
	char* end;
	char* start = line;

	while (*start != '\0' && isspace((unsigned char)*start))
		start++;
	if (start != line)
		memmove(line, start, strlen(start) + 1);

	end = line + strlen(line);
	while (end > line && isspace((unsigned char)end[-1])) {
		end--;
		*end = '\0';
	}
}


static int
HasDefaultVia(const char* gateway)
{
	FILE* pipe;
	char line[256];
	int found = 0;

	pipe = popen("route", "r");
	if (pipe == NULL)
		return 0;

	while (fgets(line, (int)sizeof(line), pipe) != NULL) {
		if (strstr(line, "default") == NULL)
			continue;
		if (strstr(line, gateway) != NULL) {
			found = 1;
			break;
		}
	}

	pclose(pipe);
	return found;
}


static int
RunIfconfig(const char* iface, const char* how)
{
	char command[256];

	snprintf(command, sizeof(command), "ifconfig %s %s", iface, how);
	if (system(command) != 0) {
		fprintf(stderr, "net_server_reapply_gateway: %s failed\n",
			command);
		return 1;
	}

	return 0;
}


static int
LoadFromSettings(char* iface, size_t ifaceSize, char* gateway,
	size_t gatewaySize)
{
	FILE* file;
	char line[256];
	char currentIface[128];
	int inInet = 0;
	int haveIface = 0;
	int haveGateway = 0;

	file = fopen(kInterfacesPath, "r");
	if (file == NULL) {
		fprintf(stderr, "net_server_reapply_gateway: open %s: %s\n",
			kInterfacesPath, strerror(errno));
		return 1;
	}

	currentIface[0] = '\0';

	while (fgets(line, (int)sizeof(line), file) != NULL) {
		Trim(line);
		if (line[0] == '\0' || line[0] == '#')
			continue;

		if (strncmp(line, "interface", 9) == 0
			&& isspace((unsigned char)line[9])) {
			char* name = line + 9;
			char* brace;

			while (*name != '\0' && isspace((unsigned char)*name))
				name++;
			brace = strchr(name, '{');
			if (brace != NULL)
				*brace = '\0';
			Trim(name);
			snprintf(currentIface, sizeof(currentIface), "%s", name);
			inInet = 0;
			continue;
		}

		if (strstr(line, "address inet6") != NULL) {
			inInet = 0;
			continue;
		}

		if (strstr(line, "address inet") != NULL) {
			inInet = 1;
			continue;
		}

		if (strncmp(line, "gateway", 7) == 0
			&& isspace((unsigned char)line[7]) && inInet
			&& currentIface[0] != '\0'
			&& strcmp(currentIface, "loop") != 0) {
			char* value = line + 7;
			while (*value != '\0' && isspace((unsigned char)*value))
				value++;
			snprintf(iface, ifaceSize, "%s", currentIface);
			snprintf(gateway, gatewaySize, "%s", value);
			haveIface = 1;
			haveGateway = 1;
			break;
		}
	}

	fclose(file);

	if (!haveIface || !haveGateway) {
		fprintf(stderr, "net_server_reapply_gateway: no static "
			"gateway in %s\n", kInterfacesPath);
		return 1;
	}

	return 0;
}


int
main(int argc, char** argv)
{
	char iface[128];
	char gateway[64];

	if (argc >= 3) {
		snprintf(iface, sizeof(iface), "%s", argv[1]);
		snprintf(gateway, sizeof(gateway), "%s", argv[2]);
	} else if (LoadFromSettings(iface, sizeof(iface), gateway,
			sizeof(gateway)) != 0) {
		return 1;
	}

	printf("interface %s gateway %s\n", iface, gateway);

	if (!HasDefaultVia(gateway)) {
		fprintf(stderr, "net_server_reapply_gateway: no default "
			"route via %s before down; set a static gateway first\n",
			gateway);
		return 1;
	}

	if (RunIfconfig(iface, "down") != 0)
		return 1;
	sleep(1);
	if (RunIfconfig(iface, "up") != 0) {
		RunIfconfig(iface, "up");
		return 1;
	}

	/* net_server handles B_NETWORK_INTERFACE_CHANGED asynchronously. */
	sleep(2);

	if (!HasDefaultVia(gateway)) {
		fprintf(stderr, "net_server_reapply_gateway: default route "
			"via %s missing after up\n", gateway);
		return 2;
	}

	printf("default route via %s present after down/up\n", gateway);
	return 0;
}
