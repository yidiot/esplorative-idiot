# esplorative-idiot

esplorative-idiot is a throwaway repository to test the LoRa technology from
within [zephyr].

It intends to build a firmware to run on [Heltec WiFi LoRa 32 V2] boards.

## FETCH, BUILD AND FLASH

Create and enter the workspace directory:

	mkdir esplorative-workspace
	cd esplorative-workspace

Fetch the sources down to the workspace:

	west init -m git@github.com:idiot-prototypes/esplorative-idiot
	west update

Build and flash the LoRa samples from the source tree:

The *receiver* on one hand:

	west build -d build/lora/receive -b heltec_wifi_lora32_v2 esplorative-idiot/samples/drivers/lora/receive/
	west flash -d build/lora/receive

The *sender* on the other hand:

	west build -d build/lora/send -b heltec_wifi_lora32_v2 esplorative-idiot/samples/drivers/lora/send/
	west flash -d build/lora/send

## PREREQUISITE

### CMAKE PACKAGE

Install the Zephyr [CMake package] using [west] `zephyr-export`:

	west zephyr-export

### TOOLCHAIN

Install the [toolchain] by extracting the archive to the home directory
`~/.espressif/tools/zephyr`.

Example on Linux x86_64:

	mkdir -p ~/.espressif/tools/zephyr
	curl -s https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz | tar xvzf - -C ~/.espressif/tools/zephyr

### SHELL COMPLETION (OPTIONAL)

Eventually, install the [shell completion] script using [west] `completion`,
and relog then (or `source ~/.local/share/bash-completion/completions/west`).

Example for bash:

	mkdir -p ~/.local/share/bash-completion/completions
	west completion bash >~/.local/share/bash-completion/completions/west

## BUGS

Report bugs at *https://github.com/idiot-prototypes/esplorative-idiot/issues*

## AUTHOR

Written by Gaël PORTAY *gael.portay@gmail.com*

## COPYRIGHT

Copyright (c) 2022 Gaël PORTAY

This program is licensed under the terms of the Apache License, version 2.0.

[west]: https://github.com/zephyrproject-rtos/west
[zephyr]: https://github.com/zephyrproject-rtos/zephyr
[Heltec WiFi LoRa 32 V2]: https://heltec.org/project/wifi-lora-32/
[CMake package]: https://docs.zephyrproject.org/latest/build/zephyr_cmake_package.html#zephyr-cmake-package-export-west
[toolchain]: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-guides/tools/idf-tools.html#xtensa-esp32-elf
[shell completion]: https://docs.zephyrproject.org/latest/develop/west/install.html#enabling-shell-completion
