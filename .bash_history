ls
sudo apt update
sudo apt full-upgrade -y
ls -l /dev/i2c-1
sudo i2cdetect -y 1
mkdir bmp280_driver
cd bmp280_driver
touch bmp280.c bmp280.h main.c Makefile README.md
git init
cd ./..
sudo apt update
sudo apt install -y git
ls
cd bmp280_driver
ls
git init
