#include <linux/dmi.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>

//#include "hid-ids.h"

#define MSI_CLAW_FEATURE_GAMEPAD_REPORT_ID	0x0f

#define MSI_CLAW_GAME_CONTROL_DESC		0x05
#define MSI_CLAW_DEVICE_CONTROL_DESC		0x06

enum msi_claw_gamepad_mode {
	MSI_CLAW_GAMEPAD_MODE_OFFLINE,
	MSI_CLAW_GAMEPAD_MODE_XINPUT,
	MSI_CLAW_GAMEPAD_MODE_DINPUT,
	MSI_CLAW_GAMEPAD_MODE_MSI,
	MSI_CLAW_GAMEPAD_MODE_DESKTOP,
	MSI_CLAW_GAMEPAD_MODE_BIOS,
	MSI_CLAW_GAMEPAD_MODE_TESTING,
	MSI_CLAW_GAMEPAD_MODE_MAX
};

enum msi_claw_mkeys_function {
	MSI_CLAW_MKEY_FUNCTION_MACRO,
	MSI_CLAW_MKEY_FUNCTION_COMBINATION,
};

enum msi_claw_command_type {
	MSI_CLAW_COMMAND_TYPE_ENTER_PROFILE_CONFIG = 1,
	MSI_CLAW_COMMAND_TYPE_EXIT_PROFILE_CONFIG,
	MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE,
	MSI_CLAW_COMMAND_TYPE_READ_PROFILE,
	MSI_CLAW_COMMAND_TYPE_READ_PROFILE_ACK,
	MSI_CLAW_COMMAND_TYPE_ACK,
	MSI_CLAW_COMMAND_TYPE_SWITCH_PROFILE,
	MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE_TO_EEPROM,
	MSI_CLAW_COMMAND_TYPE_READ_FIRMWARE_VERSION,
	MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS_ACK,
	MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE,
	MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE_ACK,
	MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS,
	MSI_CLAW_COMMAND_TYPE_SYNC_TO_ROM = 34,
	MSI_CLAW_COMMAND_TYPE_RESTORE_FROM_ROM,
	MSI_CLAW_COMMAND_TYPE_SWITCH_MODE,
	MSI_CLAW_COMMAND_TYPE_READ_GAMEPAD_MODE = 38,
	MSI_CLAW_COMMAND_TYPE_GAMEPAD_MODE_ACK,
	MSI_CLAW_COMMAND_TYPE_RESET_DEVICE,
	MSI_CLAW_COMMAND_TYPE_RGB_CONTROL = 224,
	MSI_CLAW_COMMAND_TYPE_CALIBRATION_CONTROL = 253,
	MSI_CLAW_COMMAND_TYPE_CALIBRATION_ACK,
};

struct msi_claw_drvdata {
	struct hid_device *hdev;
	struct input_dev *input;
	struct input_dev *tp_kbd_input;
};

static int msi_claw_write_cmd(struct hid_device *hdev, enum msi_claw_command_type,
        u8 b1, u8 b2, u8 b3)
{
	int ret;
	const unsigned char buf[] = {
		MSI_CLAW_FEATURE_GAMEPAD_REPORT_ID, 0, 0, 60,
		MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, b1, b2, b3
	};
	unsigned char *dmabuf = kmemdup(buf, sizeof(buf), GFP_KERNEL);
	if (!dmabuf) {
		ret = -ENOMEM;
		hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
		return ret;
	}

	ret = hid_hw_output_report(hdev, dmabuf, sizeof(buf));

	kfree(dmabuf);

	if (ret != sizeof(buf)) {
		hid_err(hdev, "msi-claw failed to switch controller mode: %d\n", ret);
		return ret;
	}

	return 0;
}

static int msi_claw_read(struct hid_device *hdev, u8 *const buffer)
{
	int ret;

	unsigned char *dmabuf = kmemdup(buffer, 8, GFP_KERNEL);
	if (!dmabuf) {
		ret = -ENOMEM;
		hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
		return ret;
	}

	ret = hid_hw_raw_request(hdev, MSI_CLAW_FEATURE_GAMEPAD_REPORT_ID, dmabuf, 8, HID_INPUT_REPORT, HID_REQ_GET_REPORT);

	if (ret >= 8) {
		hid_err(hdev, "msi-claw read %d bytes: %02x %02x %02x %02x %02x %02x %02x %02x \n", ret,
			dmabuf[0], dmabuf[1], dmabuf[2], dmabuf[3], dmabuf[4], dmabuf[5], dmabuf[6], dmabuf[7]);
		memcpy((void*)buffer, dmabuf, 8);
		ret = 0;
	} else if (ret < 0) {
		hid_err(hdev, "msi-claw failed to read: %d\n", ret);
		goto msi_claw_read_err;
	} else {
		hid_err(hdev, "msi-claw read %d bytes\n", ret);
		ret = -EINVAL;
		goto msi_claw_read_err;
	}

msi_claw_read_err:
	kfree(dmabuf);

	return ret;
}

static int msi_claw_read2(struct hid_device *hdev, u8 *const buffer)
{
	int ret;

	unsigned char *dmabuf = kmemdup(buffer, 8, GFP_KERNEL);
	if (!dmabuf) {
		ret = -ENOMEM;
		hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
		return ret;
	}

	ret = hid_hw_raw_request(hdev, 0x06, dmabuf, 8, HID_FEATURE_REPORT, HID_REQ_GET_REPORT);

	if (ret >= 8) {
		hid_err(hdev, "msi-claw read2 %d bytes: %02x %02x %02x %02x %02x %02x %02x %02x \n", ret,
			dmabuf[0], dmabuf[1], dmabuf[2], dmabuf[3], dmabuf[4], dmabuf[5], dmabuf[6], dmabuf[7]);
		memcpy((void*)buffer, dmabuf, 8);
		ret = 0;
	} else if (ret < 0) {
		hid_err(hdev, "msi-claw failed to read2: %d\n", ret);
		goto msi_claw_read2_err;
	} else {
		hid_err(hdev, "msi-claw read2 %d bytes\n", ret);
		ret = -EINVAL;
		goto msi_claw_read2_err;
	}

msi_claw_read2_err:
	kfree(dmabuf);

	return ret;
}

static int msi_claw_switch_gamepad_mode(struct hid_device *hdev, enum msi_claw_gamepad_mode mode,
	enum msi_claw_mkeys_function mkeys)
{
	int ret;

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, (u8)mode, (u8)mkeys, (u8)0);
	if (ret) {
		hid_err(hdev, "msi-claw failed to send write request for switch controller mode: %d\n", ret);
		return ret;
	}

	ret = msi_claw_write_cmd(hdev, MSI_CLAW_COMMAND_TYPE_READ_GAMEPAD_MODE, (u8)0, (u8)0, (u8)0);
	if (ret) {
		hid_err(hdev, "msi-claw failed to send read request for controller mode: %d\n", ret);
		return ret;
	}

	return 0;
}

static int msi_claw_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	return 0;
}

static int msi_claw_input_mapping(struct hid_device *hdev, struct hid_input *hi, struct hid_field *field,
	struct hid_usage *usage, unsigned long **bit, int *max)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	return 0;
}

static int msi_claw_event(struct hid_device *hdev, struct hid_field *field, struct hid_usage *usage, __s32 value)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	return 0;
}

static int msi_claw_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;
	struct msi_claw_drvdata *drvdata;

	if (!hid_is_usb(hdev)) {
		hid_err(hdev, "msi-claw hid not usb\n");
		return -ENODEV;
	}

	drvdata = devm_kzalloc(&hdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		hid_err(hdev, "msi-claw can't alloc descriptor\n");
		return -ENOMEM;
	}

	hid_set_drvdata(hdev, drvdata);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "msi-claw hid parse failed: %d\n", ret);
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "msi-claw hw start failed: %d\n", ret);
		return ret;
	}

//	hid_err(hdev, "msi-claw on %d\n", (int)hdev->rdesc[0]);

	if (hdev->rdesc[0] == MSI_CLAW_DEVICE_CONTROL_DESC) {
		ret = msi_claw_switch_gamepad_mode(hdev, MSI_CLAW_GAMEPAD_MODE_XINPUT, MSI_CLAW_MKEY_FUNCTION_MACRO);
		if (ret != 0) {
			hid_err(hdev, "msi-claw failed to initialize controller mode: %d\n", ret);
			goto err_stop_hw;
		}
	}

	return 0;

err_stop_hw:
	hid_hw_stop(hdev);
	return ret;
}

static void msi_claw_remove(struct hid_device *hdev)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	hid_hw_stop(hdev);
}

static const struct hid_device_id msi_claw_devices[] = {
	{ HID_USB_DEVICE(0x0DB0, 0x1901) },
	{ }
};
MODULE_DEVICE_TABLE(hid, msi_claw_devices);

static struct hid_driver msi_claw_driver = {
	.name			= "msi-claw",
	.id_table		= msi_claw_devices,
	.probe			= msi_claw_probe,
	.remove			= msi_claw_remove,
	//.input_mapping	= msi_claw_input_mapping,
	//.event		= msi_claw_event,
	//.raw_event		= msi_claw_raw_event
};
module_hid_driver(msi_claw_driver);

MODULE_LICENSE("GPL");
