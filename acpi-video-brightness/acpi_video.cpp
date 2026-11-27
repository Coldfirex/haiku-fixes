/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * ACPI video brightness notify handler.
 * Catches the standard 0x86 / 0x87 notifies that many laptops
 * (including Dell XPS) generate for the brightness keys and
 * publishes a simple device so a userspace helper can react.
 */

#include <ACPI.h>
#include <KernelExport.h>
#include <fs/select_sync_pool.h>
#include <stdlib.h>
#include <string.h>

#include <device_manager.h>


#define ACPI_VIDEO_MODULE_NAME "drivers/power/acpi_video/driver_v1"
#define ACPI_VIDEO_DEVICE_MODULE_NAME "drivers/power/acpi_video/device_v1"

#define ACPI_VIDEO_NOTIFY_INC_BRIGHTNESS	0x86
#define ACPI_VIDEO_NOTIFY_DEC_BRIGHTNESS	0x87

//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x...) dprintf("acpi_video: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("acpi_video: " x)


static device_manager_info* sDeviceManager;
static acpi_module_info* sAcpi;


typedef struct acpi_video_device_info {
	device_node*			node;
	acpi_device_module_info* acpi;
	acpi_handle				acpi_cookie;
	select_sync_pool*		select_pool;
	uint8					last_event;		// 0x86 or 0x87
} acpi_video_device_info;


static void
acpi_video_notify_handler(acpi_handle _device, uint32 value, void* context)
{
	acpi_video_device_info* device = (acpi_video_device_info*)context;

	if (value == ACPI_VIDEO_NOTIFY_INC_BRIGHTNESS
		|| value == ACPI_VIDEO_NOTIFY_DEC_BRIGHTNESS) {
		TRACE("brightness notify 0x%" B_PRIx32 "\n", value);
		device->last_event = (uint8)value;
		if (device->select_pool != NULL)
			notify_select_event_pool(device->select_pool, B_SELECT_READ);
	} else {
		TRACE("other notify 0x%" B_PRIx32 "\n", value);
	}
}


static float
acpi_video_supports_device(device_node* parent)
{
	const char* bus;
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			!= B_OK || strcmp(bus, "acpi") != 0)
		return 0.0f;

	// Prefer devices that implement _BCL (brightness control levels).
	// Fall back to common video HIDs if the method check is not available.
	acpi_device_module_info* acpi;
	acpi_handle handle;
	if (sDeviceManager->get_driver(parent, (driver_module_info**)&acpi,
			(void**)&handle) != B_OK)
		return 0.0f;

	// Simple check: try to evaluate _BCL. If it succeeds we take the device.
	acpi_object_type result;
	if (acpi->evaluate_method(handle, "_BCL", NULL, &result) == B_OK) {
		if (result.object_type == ACPI_TYPE_PACKAGE)
			return 0.8f;
	}

	const char* hid;
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid,
			false) == B_OK) {
		if (strcmp(hid, "ACPI0002") == 0 || strstr(hid, "VIDEO") != NULL
			|| strstr(hid, "VID") != NULL)
			return 0.6f;
	}

	return 0.0f;
}


static status_t
acpi_video_register_device(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { .string = "ACPI Video" } },
		{ NULL }
	};
	return sDeviceManager->register_node(parent, ACPI_VIDEO_MODULE_NAME,
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


static status_t
acpi_video_register_child_devices(void* cookie)
{
	device_node* node = (device_node*)cookie;
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = "ACPI Video Brightness" } },
		{ NULL }
	};
	return sDeviceManager->register_node(node, ACPI_VIDEO_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
acpi_video_init_device(void* _cookie, void** cookie)
{
	device_node* node = (device_node*)_cookie;
	acpi_video_device_info* device = (acpi_video_device_info*)malloc(
		sizeof(acpi_video_device_info));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));
	device->node = node;

	device_node* parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&device->acpi,
		(void**)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	status_t status = device->acpi->install_notify_handler(
		device->acpi_cookie, ACPI_DEVICE_NOTIFY, acpi_video_notify_handler,
		device);
	if (status != B_OK) {
		ERROR("install_notify_handler failed\n");
		free(device);
		return status;
	}

	TRACE("video device ready\n");
	*cookie = device;
	return B_OK;
}


static void
acpi_video_uninit_device(void* cookie)
{
	acpi_video_device_info* device = (acpi_video_device_info*)cookie;
	device->acpi->remove_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, acpi_video_notify_handler);
	free(device);
}


static status_t
acpi_video_open(void* cookie, const char* path, int flags, void** _cookie)
{
	*_cookie = cookie;
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
	acpi_video_device_info* device = (acpi_video_device_info*)cookie;
	if (*numBytes < 1)
		return B_BAD_VALUE;

	if (device->last_event == 0) {
		*numBytes = 0;
		return B_OK;
	}

	uint8 event = device->last_event;
	device->last_event = 0;
	*(uint8*)buffer = event;
	*numBytes = 1;
	return B_OK;
}


static status_t
acpi_video_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	return B_NOT_ALLOWED;
}


static status_t
acpi_video_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_BAD_VALUE;
}


static status_t
acpi_video_select(void* cookie, uint8 event, selectsync* sync)
{
	acpi_video_device_info* device = (acpi_video_device_info*)cookie;
	if (event != B_SELECT_READ)
		return B_BAD_VALUE;
	return add_select_sync_pool_entry(&device->select_pool, sync, event);
}


static status_t
acpi_video_deselect(void* cookie, uint8 event, selectsync* sync)
{
	acpi_video_device_info* device = (acpi_video_device_info*)cookie;
	return remove_select_sync_pool_entry(&device->select_pool, sync, event);
}


static driver_module_info acpi_video_driver_module = {
	{ ACPI_VIDEO_MODULE_NAME, 0, NULL },

	acpi_video_supports_device,
	acpi_video_register_device,
	acpi_video_init_driver,
	acpi_video_uninit_driver,
	acpi_video_register_child_devices,
	NULL,
	NULL
};


static device_module_info acpi_video_device_module = {
	{ ACPI_VIDEO_DEVICE_MODULE_NAME, 0, NULL },

	acpi_video_init_device,
	acpi_video_uninit_device,
	NULL,

	acpi_video_open,
	acpi_video_close,
	acpi_video_free,
	acpi_video_read,
	acpi_video_write,
	NULL,
	acpi_video_control,
	acpi_video_select,
	acpi_video_deselect
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info**)&sAcpi },
	{}
};


module_info* modules[] = {
	(module_info*)&acpi_video_driver_module,
	(module_info*)&acpi_video_device_module,
	NULL
};
