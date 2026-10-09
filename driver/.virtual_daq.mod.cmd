savedcmd_virtual_daq.mod := printf '%s\n'   virtual_daq.o | awk '!x[$$0]++ { print("./"$$0) }' > virtual_daq.mod
