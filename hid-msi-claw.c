#include <linux/dmi.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>

//#include "hid-ids.h"

#define FEATURE_GAMEPAD_REPORT_ID 0x0f

enum msi_claw_gamepad_mode {
    MSI_CLAW_GAMEPAD_MODE_OFFLINE = 0,
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
    MSI_CLAW_COMMAND_TYPE_EXIT_PROFILE_CONFIG = 2,
    MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE = 3,
    MSI_CLAW_COMMAND_TYPE_READ_PROFILE = 4,
    MSI_CLAW_COMMAND_TYPE_READ_PROFILE_ACK = 5,
    MSI_CLAW_COMMAND_TYPE_ACK = 6,
    MSI_CLAW_COMMAND_TYPE_SWITCH_PROFILE = 7,
    MSI_CLAW_COMMAND_TYPE_WRITE_PROFILE_TO_EEPROM = 8,
    MSI_CLAW_COMMAND_TYPE_READ_FIRMWARE_VERSION = 9,
    MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS_ACK = 10,
    MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE = 11,
    MSI_CLAW_COMMAND_TYPE_READ_CURRENT_PROFILE_ACK = 12,
    MSI_CLAW_COMMAND_TYPE_READ_RGB_STATUS = 13,
    MSI_CLAW_COMMAND_TYPE_SYNC_TO_ROM = 34,
    MSI_CLAW_COMMAND_TYPE_RESTORE_FROM_ROM = 35,
    MSI_CLAW_COMMAND_TYPE_SWITCH_MODE = 36,
    MSI_CLAW_COMMAND_TYPE_READ_GAMEPAD_MODE = 38,
    MSI_CLAW_COMMAND_TYPE_GAMEPAD_MODE_ACK = 39,
    MSI_CLAW_COMMAND_TYPE_RESET_DEVICE = 40,
    MSI_CLAW_COMMAND_TYPE_RGB_CONTROL = 224,
    MSI_CLAW_COMMAND_TYPE_CALIBRATION_CONTROL = 253,
    MSI_CLAW_COMMAND_TYPE_CALIBRATION_ACK = 254,
};

struct msi_claw_drvdata {
	struct hid_device *hdev;
	struct input_dev *input;
	struct input_dev *tp_kbd_input;
};

static int msi_claw_switch_gamepad_mode(struct hid_device *hdev, enum msi_claw_gamepad_mode mode,
        enum msi_claw_mkeys_function mkeys)
{
	int ret;
	const unsigned char buf[] = {
		0, 0, 60, MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, (unsigned char)mode, (unsigned char)mkeys, 0, 0
	};
	unsigned char *dmabuf = kmemdup(buf, sizeof(buf), GFP_KERNEL);

	if (!dmabuf) {
		ret = -ENOMEM;
		hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
		return ret;
	}

	ret = hid_hw_raw_request(hdev, FEATURE_GAMEPAD_REPORT_ID, dmabuf, sizeof(buf),
					HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

	kfree(dmabuf);

	if (ret != sizeof(buf)) {
		hid_err(hdev, "msi-claw failed to switch controller mode: %d\n", ret);

        const unsigned char buf2[] = {
            0, 0, 60, MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, (unsigned char)mode, (unsigned char)mkeys, 0
        };
        dmabuf = kmemdup(buf2, sizeof(buf2), GFP_KERNEL);

        if (!dmabuf) {
            ret = -ENOMEM;
            hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
            return ret;
        }

        ret = hid_hw_raw_request(hdev, FEATURE_GAMEPAD_REPORT_ID, dmabuf, sizeof(buf2),
                        HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

        kfree(dmabuf);

        if (ret != sizeof(buf)) {
		    hid_err(hdev, "msi-claw failed to switch controller mode (3): %d\n", ret);
            
            const unsigned char buf3[] = {
                0, 0, 60, MSI_CLAW_COMMAND_TYPE_SWITCH_MODE, (unsigned char)mode, (unsigned char)mkeys
            };
            dmabuf = kmemdup(buf3, sizeof(buf3), GFP_KERNEL);

            if (!dmabuf) {
                ret = -ENOMEM;
                hid_err(hdev, "msi-claw failed to alloc dma buf: %d\n", ret);
                return ret;
            }

            ret = hid_hw_raw_request(hdev, FEATURE_GAMEPAD_REPORT_ID, dmabuf, sizeof(buf3),
                            HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

            kfree(dmabuf);

            if (ret != sizeof(buf)) {
                hid_err(hdev, "msi-claw failed to switch controller mode (3): %d\n", ret);
                
                
                return ret;
            } else {
                return 0;
            }
            
            return ret;
        } else {
            return 0;
        }

		return ret;
	} else {
        return 0;
    }

	return 0;
}

static int msi_claw_raw_event(struct hid_device *hdev,
		struct hid_report *report, u8 *data, int size)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

	return 0;
}

/*
#define asus_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, \
						    max, EV_KEY, (c))
*/
static int msi_claw_input_mapping(struct hid_device *hdev,
		struct hid_input *hi, struct hid_field *field,
		struct hid_usage *usage, unsigned long **bit,
		int *max)
{
	struct msi_claw_drvdata *drvdata = hid_get_drvdata(hdev);

    /*
    if ((usage->hid & HID_USAGE_PAGE) == HID_UP_ASUSVENDOR) {
		switch (usage->hid & HID_USAGE) {
		case 0x10: asus_map_key_clear(KEY_BRIGHTNESSDOWN);	break;
    */

    return 0;
}

static int msi_claw_event(struct hid_device *hdev, struct hid_field *field,
		      struct hid_usage *usage, __s32 value)
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
		return -ENODEV;
	}

    ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "msi-claw hw start failed: %d\n", ret);
		return ret;
	}

    hid_err(hdev, "msi-claw on %d\n", (int)hdev->rdesc[0]);

    ret = msi_claw_switch_gamepad_mode(hdev, MSI_CLAW_GAMEPAD_MODE_XINPUT, MSI_CLAW_MKEY_FUNCTION_MACRO);
    if (ret != 0) {
        hid_err(hdev, "msi-claw failed to initialize controller mode: %d\n", ret);

        // improve this
        ret = -ENODEV;

        goto err_stop_hw;
    }

    // TODO: remove me
    hid_err(hdev, "msi-claw started\n");

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
	{ HID_USB_DEVICE(0x0DB0,
	    0x1901) },
	{ }
};
MODULE_DEVICE_TABLE(hid, msi_claw_devices);

static struct hid_driver msi_claw_driver = {
	.name			= "msi-claw",
	.id_table		= msi_claw_devices,
	//.report_fixup	= asus_report_fixup,
	.probe                  = msi_claw_probe,
	.remove			= msi_claw_remove,
	.input_mapping          = msi_claw_input_mapping,
	.event			= msi_claw_event,
	.raw_event		= msi_claw_raw_event
};
module_hid_driver(msi_claw_driver);

MODULE_LICENSE("GPL");
