# Source files (add path to VPATH below)
SRCS += VL53L1X_api.c
SRCS += VL53L1X_calibration.c
SRCS += vl53l1_platform.c

# Where to find BSP source files
VPATH += ./VL53L1X/API/core
VPATH += ./VL53L1X/API/platform

# Where to find BSP header files
IPATH += ./VL53L1X/API/core
IPATH += ./VL53L1X/API/platform
