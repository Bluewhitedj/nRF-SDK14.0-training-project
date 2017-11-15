var nrf51_getting_started =
[
    [ "Running precompiled examples", "getting_started_precompiled.html", null ],
    [ "Installing the SDK", "getting_started_installing.html", null ],
    [ "Compiling and running a first example", "getting_started_examples.html", [
      [ "Erasing the board", "getting_started_examples.html#gs_examples_erasing", null ],
      [ "Running the example", "getting_started_examples.html#gs_examples_running", null ]
    ] ],
    [ "Running examples that use a SoftDevice", "getting_started_softdevice.html", [
      [ "Programming SoftDevices", "getting_started_softdevice.html#getting_started_sd", [
        [ "nRFgo Studio", "getting_started_softdevice.html#getting_started_sd_studio", null ],
        [ "ARM Keil", "getting_started_softdevice.html#getting_started_sd_keil", null ],
        [ "GCC makefile", "getting_started_softdevice.html#getting_started_sd_gcc", null ]
      ] ],
      [ "Running ANT examples", "getting_started_softdevice.html#getting_started_sd_ant52", null ]
    ] ],
    [ "Running a serialized application", "nrf51_setups_serialization.html", [
      [ "Serialization hardware setup", "nrf51_setups_serialization.html#serialization_hardware", [
        [ "UART (BLE)", "nrf51_setups_serialization.html#serialization_hardware_uart", null ],
        [ "SPI (BLE)", "nrf51_setups_serialization.html#serialization_hardware_spi", null ],
        [ "UART (ANT)", "nrf51_setups_serialization.html#serialization_hardware_uart_ant", null ]
      ] ],
      [ "Serialization software setup", "nrf51_setups_serialization.html#serialization_software", null ]
    ] ],
    [ "Using the SDK with other boards", "sdk_for_custom_boards.html", [
      [ "Supported boards", "sdk_for_custom_boards.html#supported_board", null ],
      [ "Enabling support for a board", "sdk_for_custom_boards.html#bsp_location", null ],
      [ "Adding support for a custom board", "sdk_for_custom_boards.html#custom_board_support", null ]
    ] ],
    [ "SDK configuration header file", "sdk_config.html", [
      [ "CMSIS Configuration Annotations Wizard", "sdk_config.html#sdk_config_annotations", null ],
      [ "Template projects", "sdk_config.html#sdk_config_template", null ],
      [ "Standard projects", "sdk_config.html#sdk_config_standard", null ],
      [ "Overriding the sdk_config.h configuration", "sdk_config.html#sdk_config_overriding", null ]
    ] ],
    [ "Migration guide", "migration.html", [
      [ "Bluetooth low energy (BLE)", "migration.html#migration_ble", [
        [ "SoftDevice handler", "migration.html#migration_ble_sdhandler", null ],
        [ "Advertising Module (ble_advertising)", "migration.html#migration_ble_adv_mod", null ],
        [ "ble_advertising_init", "migration.html#migration_ble_adv_init", null ],
        [ "Advertising Encoder (ble_advdata)", "migration.html#migration_ble_adv_enc", null ],
        [ "Connection Parameters Module (ble_conn_params)", "migration.html#migration_ble_conn_params", null ]
      ] ],
      [ "Flash Data Storage (FDS)", "migration.html#migration_fds", [
        [ "FDS record chunks removed", "migration.html#migration_fds_chunks", null ],
        [ "fds_header_t", "migration.html#migration_fds_header", null ],
        [ "CRC configuration options", "migration.html#migration_fds_config", null ]
      ] ],
      [ "fstorage", "migration.html#migration_fstorage", null ],
      [ "Configuration of the BLE stack", "migration.html#migration_ble_config", null ],
      [ "Libraries", "migration.html#migration_libs", [
        [ "nrf_cli", "migration.html#migration_libs_nrf_cli", null ],
        [ "nrf_log", "migration.html#migration_libs_nrf_log", null ],
        [ "nrf_pwr_mgmt", "migration.html#migration_libs_nrf_pwr_mgmt", null ],
        [ "Atomic FIFO", "migration.html#migration_libs_atomic_fifo", null ]
      ] ],
      [ "NFC", "migration.html#migration_nfc", [
        [ "NDEF record and message descriptors", "migration.html#migration_nfc_ndef_record", null ],
        [ "NFC BLE pairing library", "migration.html#migration_nfc_pairing_lib", null ]
      ] ],
      [ "DFU", "migration.html#migration_dfu", [
        [ "Inactivity timeout timer", "migration.html#migration_dfu_timer", null ],
        [ "DFU API changes", "migration.html#migration_dfu_api", null ]
      ] ],
      [ "USB", "migration.html#migration_usb", [
        [ "USBD START and STOP events renamed", "migration.html#migration_usb_events", null ],
        [ "Processing of high level events in the main loop", "migration.html#migration_usb_main_loop", null ],
        [ "USB audio class API rebuilt", "migration.html#migration_usb_audio", null ]
      ] ]
    ] ]
];