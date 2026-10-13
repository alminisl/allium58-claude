# Allium58 wired: Pro Micro footprint with an RP2040 drop-in (Sea-Picro style).
CONVERT_TO = rp2040_ce

VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE = yes
OLED_ENABLE = yes
OLED_DRIVER = ssd1306
MOUSEKEY_ENABLE = no
EXTRAKEY_ENABLE = yes
WPM_ENABLE = yes
TAP_DANCE_ENABLE = yes
COMBO_ENABLE = no
GRAVE_ESC_ENABLE = no
MAGIC_ENABLE = no
BOOTMAGIC_ENABLE = no

COMMAND_ENABLE = no
CONSOLE_ENABLE = no

QMK_SETTINGS = yes
LTO_ENABLE = yes

KEY_OVERRIDE_ENABLE = no

SRC += clawd.c

# The mascot render needs ~1.7 KB of stack; with interrupts on top it overflowed the 2 KB default and hung the main loop.
# 0x1000 does not fit: the stacks share a 4 KB scratch RAM bank with the exception stack.
USE_PROCESS_STACKSIZE = 0xA00
