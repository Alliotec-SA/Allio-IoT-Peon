Import("env")
from SCons.Script import SetOption

# Avoid cc1plus.exe OOM on low-RAM hosts when PlatformIO compiles in parallel.
SetOption("num_jobs", 1)
