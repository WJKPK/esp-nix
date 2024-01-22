#!${pkgs.stdenv.shell}
esptool.py -p /dev/ttyACM0 -b 460800 --before default_reset --after hard_reset --chip esp32c3 \
    write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x0 result/bootloader/bootloader.bin \
    0x8000 result/partition_table/partition-table.bin 0x10000 result/hello_world.bin
