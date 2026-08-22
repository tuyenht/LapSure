Optional CPU thermal/power provider
Recommended Windows backend: LibreHardwareMonitor.
Expected helper contract: cpu_temp_c|cpu_power_w|cpu_clock_mhz|thermal_throttle
The helper must be reviewed and SHA-256 allowlisted as lhm_bridge.
Without it, CPU package thermal remains UNKNOWN.
