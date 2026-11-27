/*
 * ACPI video brightness notify handler.
 * Catches 0x86/0x87 from the EC and publishes them so a userspace helper
 * can call screenmode. Matches any device that has _BCL.
 */

#include <string.h>
#include <stdlib.h>

#include <KernelExport.h>
#include <device_manager.h>
#include <drivers/KernelExport.h>

#include <ACPI.h>


#define ACPI_VIDEO_MODULE_NAME "drivers/power/acpi_video/driver_v1"
#define ACPI_VIDEO_DEVICE_MODULE_NAME "drivers/power/acpi_video/device_v1"
#define ACPI_VIDEO_BASENAME "power/acpi_video/%d"

#define ACPI_VIDEO_NOTIFY_INC	0x86
#define ACPI_VIDEO_NOTIFY_DEC	0x87


struct acpi_video_device {
	device_node*				node;
	acpi_device_module_info*	acpi;
	acpi_device					acpi_cookie;
	uint32						last_notify;
};


static device_manager_info* sDeviceManager;
static acpi_module_info* sAcpi;


static void
acpi_video_notify_handler(acpi_handle device, uint32 value, void* context)
{
	acpi_video_device* info = (acpi_video_device*)context;
	if (value != ACPI_VIDEO_NOTIFY_INC && value != ACPI_VIDEO_NOTIFY_DEC)
		return;

	info->last_notify = value;
	dprintf("acpi_video: notify 0x%lx\n", (unsigned long)value);
	// wake any waiting readers
	sDeviceManager->publish_device(info->node, ACPI_VIDEO_BASENAME, NULL);
}


static status_t
acpi_video_init_device(device_node* node, void** _cookie)
{
	acpi_video_device* info = (acpi_video_device*)calloc(1, sizeof(*info));
	if (info == NULL)
		return B_NO_MEMORY;

	info->node = node;

	device_node* parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&info->acpi,
		(void**)&info->acpi_cookie);
	sDeviceManager->put_node(parent);

	status_t status = info->acpi->install_notify_handler(info->acpi_cookie,
		ACPI_DEVICE_NOTIFY, acpi_video_notify_handler, info);
	if (status != B_OK) {
		dprintf("acpi_video: install_notify_handler failed\n");
		free(info);
		return status;
	}

	*_cookie = info;
	return B_OK;
}


static void
acpi_video_uninit_device(void* _cookie)
{
	acpi_video_device* info = (acpi_video_device*)_cookie;
	info->acpi->remove_notify_handler(info->acpi_cookie,
		ACPI_DEVICE_NOTIFY, acpi_video_notify_handler);
	free(info);
}


static status_t
acpi_video_open(void* _cookie, const char* path, int flags, void** cookie)
{
	*cookie = _cookie;
	return B_OK;
}


static status_t
acpi_video_close(void* cookie)
{
	return B_OK;
}


static status_t
acpi_video_free(void* cookie)
{
	return B_OK;
}


static status_t
acpi_video_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	acpi_video_device* info = (acpi_video_device*)cookie;
	if (*numBytes < sizeof(uint32))
		return B_BUFFER_OVERFLOW;

	uint32 value = info->last_notify;
	info->last_notify = 0;
	memcpy(buffer, &value, sizeof(value));
	*numBytes = sizeof(value);
	return B_OK;
}


static status_t
acpi_video_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


static status_t
acpi_video_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
acpi_video_support(device_node* parent)
{
	const char* bus = NULL;
	sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false);
	if (bus == NULL || strcmp(bus, "acpi") != 0)
		return B_ERROR;

	// only devices that have _BCL
	acpi_device_module_info* acpi = NULL;
	acpi_device cookie;
	sDeviceManager->get_driver(parent, (driver_module_info**)&acpi,
		(void**)&cookie);
	if (acpi == NULL)
		return B_ERROR;

	acpi_data result;
	result.pointer = NULL;
	result.length = ACPI_ALLOCATE_BUFFER;
	status_t status = acpi->evaluate_method(cookie, "_BCL", NULL, &result);
	free(result.pointer);
	return status == B_OK ? B_OK : B_ERROR;
}


static status_t
acpi_video_register_device(device_node* node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "ACPI Video" } },
		{ NULL }
	};
	return sDeviceManager->register_node(node, ACPI_VIDEO_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
acpi_video_init_driver(device_node* node, void** _cookie)
{
	*_cookie = node;
	return B_OK;
}


static void
acpi_video_uninit_driver(void* cookie)
{
}


static float
acpi_video_support_child(device_node* parent)
{
	return 0.0f;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}


static driver_module_info sAcpiVideoDevice = {
	{
		ACPI_VIDEO_DEVICE_MODULE_NAME,
		0,
		std_ops
	},

	NULL,
	NULL,
	acpi_video_init_device,
	acpi_video_uninit_device,

	acpi_video_open,
	acpi_video_close,
	acpi_video_free,

	acpi_video_read,
	acpi_video_write,
	NULL,
	acpi_video_control,

	NULL,
	NULL
};


static driver_module_info sAcpiVideoDriver = {
	{
		ACPI_VIDEO_MODULE_NAME,
		0,
		std_ops
	},

	acpi_video_support,
	acpi_video_register_device,
	acpi_video_init_driver,
	acpi_video_uninit_driver,

	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info**)&sAcpi },
	{}
};


module_info* modules[] = {
	(module_info*)&sAcpiVideoDriver,
	(module_info*)&sAcpiVideoDevice,
	NULL
};
