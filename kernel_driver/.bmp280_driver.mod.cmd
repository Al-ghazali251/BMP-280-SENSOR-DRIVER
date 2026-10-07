savedcmd_bmp280_driver.mod := printf '%s\n'   bmp280_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > bmp280_driver.mod
