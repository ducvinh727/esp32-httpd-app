deps_config := \
	/home/ducvinh/workspace/esp-idf/components/aws_iot/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/bt/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/esp32/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/ethernet/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/fatfs/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/freertos/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/log/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/lwip/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/mbedtls/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/openssl/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/spi_flash/Kconfig \
	/home/ducvinh/workspace/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/ducvinh/workspace/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/ducvinh/workspace/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/ducvinh/Desktop/esp32-httpd-app/main/Kconfig.projbuild \
	/home/ducvinh/workspace/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
