# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_KSCAN_FT5336	kscan_ft5336.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_ITE_IT8XXX2	kscan_ite_it8xxx2.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_XEC		kscan_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_SDL		kscan_sdl.c)
zephyr_library_sources_ifdef(CONFIG_KSCAN_HT16K33	kscan_ht16k33.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE	    kscan_handlers.c)
