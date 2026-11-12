# KNOMI V2 – Custom Firmware (BP)

This fork contains a **custom firmware for the BIGTREETECH KNOMI V2** (version **BPv1.0.4**):
180° UI rotation, animations in the UI theme color, calmer Voron logo, animated Voron logo on the
printing screen, theme-colored values, no negative target temperatures and print progress
calculated like Mainsail.

* Source code and documentation: branch [`firmware`](https://github.com/its-Blackpoint/KNOMIv2-CustomFW/tree/firmware)
  ([README](https://github.com/its-Blackpoint/KNOMIv2-CustomFW/blob/firmware/README.md),
  [all changes](https://github.com/its-Blackpoint/KNOMIv2-CustomFW/blob/firmware/CHANGES.md))
* Ready-to-flash OTA binary: [`knomi2_BPv1.0.4.bin`](https://github.com/its-Blackpoint/KNOMIv2-CustomFW/raw/firmware/release/knomi2_BPv1.0.4.bin)

The original BTT content of this repository (manuals, hardware, GIFs, stock firmware) follows below. KNOMI V1 content was removed, as this firmware is for KNOMI V2 only.

---

# KNOMI2
Online manual ：[here](https://bigtreetech.github.io/docs/KNOMI2.html#)

# Klipper config

[KNOMI.cfg](./KNOMI.cfg) is the klipper config of the [latest version firmware](https://github.com/bigtreetech/KNOMI/tree/firmware)


# Firmware source code
[Here](https://github.com/bigtreetech/KNOMI/tree/firmware) is the stock BTT firmware source code.

# Firmware update
### OTA
* Download the pre compiled firmware from GitHub ([KNOMI2](./KNOMI2/Firmware/knomi2_firmware.bin)) or build your own firmware
* Enter KNOMI's IP or hostname (default is `knomi.local`) in the browser of a device with the same LAN as knomi, and then click `Update FW`<br/>
<img src=Images/ota_1.png width="400" /><br/>
* Select the firmware file just downloaded to start updating. After the update is complete, KNOMI will automatically restart and run the new firmware.<br/>
<img src=Images/ota_2.png width="400" /><br/>

### Using `Flash Download Tools` from Espressif
This step is used when KNOMI cannot start and run OTA feature normally
* Refer to the steps [here](https://bigtreetech.github.io/docs/KNOMI.html#update-firmware), but use firmware files from this repository.([KNOMI2](./KNOMI2/Firmware/))