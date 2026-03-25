# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# MCUboot uses only internal RRAM for boot slots and does not need the external
# SPI NOR flash. Disable spi00 so that the SPI NOR driver (which depends on
# kernel threading) is not compiled into the bootloader image.
if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set(mcuboot_EXTRA_DTC_OVERLAY_FILE
      "${CMAKE_CURRENT_LIST_DIR}/sysbuild/mcuboot/nrf93m1dk_nrf54l15_cpuapp.overlay"
      CACHE INTERNAL "MCUboot DTC overlay for nrf93m1dk"
  )
endif()
