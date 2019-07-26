# hatchtrack-peep-fw
ESP32 Firmware for the Hatchtrack Peep product.

## Building
It is assumed that all actions listed below are being done in a Unix
environment. Debian, Ubuntu, and Arch in particular have been used and are
known to work for building this firmware; other systems may work with little
or no modification of the instructions documented here.

### Preamble

#### Git
The code in this repository and all of its submodules must be cloned in order
for all the required files to be available for compilation. This can be done
by issuing the following commands.
```
git clone https://github.com/manfangllc/hatchtrack-peep-fw.git
cd hatchtrack-peep-fw
git submodule update --init --recursive
```

If pulling in new code, it is generally a good idea to make sure the submodules
are up to date as they may have changed. This is done running the last command
listed above.
```
git submodule update --init --recursive
```
Further, the repositroy should be cleaned of any old build artifacts
originating from older commits by running the following Makefile rule.
```
make distclean
```

As an alternative, a rule is in place with the top level makefile to clean the
repository and synchronize all git commits in the repo. This can be done by
running the following.
**NOTE:** This will remove all files that are not actively tracked by Git and
is **NOT RECOMMENDED** unless you have backed up all files or are willing to
accept their removal.
```
make gitclean
```

#### Toolchain
An ESP32 toolchain is required to build the firmware. The project is configured
to find it in the user's PATH environment variable, but this can be altered
by running `make menuconfig` and manually setting the toolchain path.

#### Python
Espressif requires the Python2 modules listed in `./esp-idf/requirements.txt`
to be installed on the local system in order for firmware to build. These can
be installed manually or by running the following command.
```
python -m pip install --user -r $IDF_PATH/requirements.txt
```

#### Embedded Documents

Embedded documents are no longer used to store the per peep specific
certificates needed to connect to the AES cloud.   See the next section for
the replacement mechanism.

### Generating Certificate NVS Image
There are files not tracked by Git that are required to build firmware. These
files are unique to each Peep being built and therefore do not make sense to
track in this repository. Currently, these files are obtained from Amazon and
by running command line arguments on your development machine. In the future,
this process will be automated, but until then, your best approach is to
contact the one of the developers of this project.

The following certificate files must be present, and in the location listed,
in order to generate an NVS image that can be flashed:

`./main/uuid.txt` : This is a 128 bit UUID assigned to the Peep being built in
the form of `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`, where each `x` is an
alphanumeric value. This UUID must be unique; reusing UUIDs for Peeps may lead
to undefined behavior with the web backend. There are many ways to generate
a UUID for a Peep, with the one easy approach being to use `uuidgen` if
installed in your Linux environment.

`./main/root_ca.txt` : The Root Certifcate Authority file issued by Amazon in
order to allow access to their IoT web backend. This can be obtained from the
AWS IoT web console and is the same for all Peep devices.

`./main/cert.txt` : The device Certificate file issued by Amazon for a specific
Peep. This can be obtained from the AWS IoT web console by creating a new
device or navigating to an already created device. This file is unique for each
Peep; undefined behavior will occur if certificates are shared for multiple
Peeps. When obtained from Amazon, the file will normally be named in the form
of `*-certificate.pem.crt`.

`./main/key.txt` : The Private Key issued by Amazon for this specific Peep.
This can be obtained from the AWS IoT web console by creating a new device or
navigating to an already created device. This file is unique for each Peep;
undefined behavior will occur if certificates are shared for multiple Peeps.
When obtained from Amazon, the file will normally be named in the form of
`*-private.pem.key`.

If these files are present a python script is provided by the ESP-IDF that
will generate an NVS image that can be programmed to the device seperately
from the application image:

   python ./esp-idf/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py --input certificates.csv --output ./build/nvs_certs.bin --size 16384

This will create a file, if successful, called "nvs_certs.bin" in the build
directory that can then be programmed to a specific flash partition.

To flash this built certificate image to the correct partition run the
following command:

   python ./esp-idf/components/esptool_py/esptool/esptool.py --port <ESPPORT> --chip esp32 write_flash 0x10000 build/nvs_certs.bin

The above two python commands to generate the nvs binary file and to flash it
have been automated via a makefile rule.   Two perform both operations perform
the following in the project root:

   make cert_flash

### Compilation

This repository uses `make` to build the code in accordance with
[Espressif's Build System](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html).


#### Full Build
To build the full firmware, all that is needed it to run `make`. This should
normally not be built currently as the complete firmware is designed to
interface with a mobile app (which does not currently exist in a usablemanner)
and Amazon's IoT web backend. The project as a whole is still in alpha stages
of development and requires care and proper tooling to ensure correct
functionality is achieved.

#### Test Build : Measurement Test
The firmware can be built such that it is preconfigured and executes in an
infinte measurement loop. This build only requires that the the supporting
web backend infastructure is in place and functional to allow Peep to function
very close to how it will in the finalized version of firmware where the end
use case is having it monitor a user's eggs. In order to get this special
build of firmware, run the command `make test-measure`.

**NOTE:** Before building and using this firmware, it is recommended to
investigate and modify the parameters listed at the top of the of
`./main/task_measure.c` to those that suit the user's environment. An example
configuration is listed below.
```
#ifdef PEEP_TEST_STATE_MEASURE
  // SSID of the WiFi AP connect to.
  #define _TEST_WIFI_SSID "test-ap"
  // Password of the WiFi AP to connect to.
  #define _TEST_WIFI_PASSWORD "test-ap-password-12345"
  // Leave this value alone unless you have a good reason to change it.
  #define _TEST_HATCH_CONFIG_UUID "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  // Seconds that Peep should deep sleep before taking a new measurement.
  #define _TEST_HATCH_CONFIG_MEASURE_INTERVAL_SEC 900
  // UTC Unix timestamp at which Peep should stop taking measurements.
  #define _TEST_HATCH_CONFIG_END_UNIX_TIMESTAMP 1735084800
#endif
```

#### Test Build : Measurement Configuration
This test build shouldn't be used currently by anyone other than those actively
developing it as it requires intimate knowledge of the entire system (firmware
and web backend) along with special developer tools to ensure correct
execution. If you know what you're doing or spoken with someone who does, this
firmware can be built using `make test-measure-config`.

#### Test Build : Bluetooth Low Energy Configuration
This test build shouldn't be used currently unless you have BLE debugging
utilities or are developing the mobile application and wish to test Bluetooth
connectivity. Building and running this version of firmware will cause the
Peep device to come up as a BLE GATT server with an endpoint for reading its
UUID and for writing WiFi SSID configuration data. If you know what you're
doing or spoken with someone who does, this firmware can be built using
`make test-ble-config`.

#### Test Build: Deep Sleep
Only build this firmware if you want to test or experiment with the Deep Sleep
mode of firmware. All this build will do is simply make the ESP32 enter deep
sleep without initializing anything. To build this firmware, run
`make test-deep-sleep`.


## Install

To flash the firmware onto the Peep device, ensure the device is connected by
USB to your development machine and verify it appears to the system.
```
$ lsusb | grep "10c4:ea60"
Bus 002 Device 002: ID 10c4:ea60 Cygnal Integrated Products, Inc. CP2102/CP2109 UART Bridge Controller [CP210x family]
```

Further, a udev rule will likely be needed to ensure that non-root users can
communicate to the device.
```
$ cat /etc/udev/rules.d/90-tty-usb.rules
# CP210x
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE="0666"
```

After confirming the above, run `make flash` to push down the new firmware
to the device.

If communication erros occur after performing the above, consult your system's
documentation to see if any other configuration (such as modifying/adding
groups) is needed.

## Debugging

There is information emitted over UART by firmware during execution that can be
used to help debug. This can be obtained by using any serial console program
configured to 115200 8N1 and software/hardware flow control disabled. An
example command of using minicom is shown below.
```
$ minicom -D /dev/ttyUSB0 -b 115200 8N1
```

The log level of the information emitted can be changed in `make menuconfig`
if more output is desired. Navigate to `Component config -> Log output` and
change the value listed for `Default log verbosity` to whatever value is
desired.
