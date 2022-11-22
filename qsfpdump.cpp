#include <stdio.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

float conv(uint8_t *data, uint16_t div, uint8_t sig)
{
	if (sig){
		return ((int16_t)  ((data[0] << 8) + data[1]))/(float) div;
	}
	return ((uint16_t) ((data[0] << 8) + data[1]))/(float) div;
}

void print_part(uint8_t *data, const char *ptype, const char *prefix, int start, int len)
{
	printf("%s=", prefix);
	for (int i=start; i<start+len; i++){
		printf(ptype, data[i]);
	}
	printf("\n");
}



bool load_eeprom(int fd, uint8_t offset, unsigned to_read, uint8_t *data)
{
	// set offset
	int ret = write(fd, &offset, 1);
	if(ret != 1){
		printf("load_eeprom: wr err: %s\n", strerror(errno));
		return false;	
	}
	// read data
	for (int i=0; i < to_read; i++){
		uint8_t b;
		int ret = read(fd, &b, 1);
		if(ret != 1){
			printf("load_eeprom: rd err: %s\n", strerror(errno));
			return false;
		}
		data[i] = b;
	}
	return true;
}

int main(int argc, char *argv[])
{
	if (argc <2){
		printf("usage: sfpdump i2cdev\n");
		return 1;
	}
	char *bus_dev_name = argv[1];
	int fd = open(bus_dev_name, O_RDWR);
	if (fd < 0){
		printf("can open %s: %s\n", bus_dev_name, strerror(errno));
		return 1;
	}
	
	// eeprom 
	{
		int dev_addr = 0x50;
		int ret = ioctl(fd, I2C_SLAVE, dev_addr);
		if (ret <0){
			printf("can't set i2c addr %02x: %s\n", dev_addr, strerror(errno));
			return 1;
		}
		uint8_t data[256];
		bool ok = load_eeprom(fd, 128, 128, data);
		if(!ok){
			return 1;
		}
		print_part(&data[0], "%c",   "Hersteller", 20, 16);
		print_part(&data[0], "%02X", "OUI", 37, 3);
		print_part(&data[0], "%c",   "Rev", 56, 4);
		print_part(&data[0], "%c",   "PN", 40, 16);
		print_part(&data[0], "%c",   "SN", 68, 16);
		print_part(&data[0], "%c",   "DC", 84,  6);
		
		printf("Typ=0x%02X\n", data[0]);
		printf("Connector=0x%02X\n", data[2]);
		printf("Bitrate=%u\n", data[12]*100); //MBd
		printf("Wavelength=%u\n", data[60] * 256 + data[61]); //nm
		
	}
#if 0
	//Diagnostic Monitoring Interface
	{
		int dev_addr = 0x50;
		int ret = ioctl(fd, I2C_SLAVE, dev_addr);
		if (ret <0){
			printf("can't set i2c addr %02x: %s\n", dev_addr, strerror(errno));
			return 1;
		}
		uint8_t diag[256];
		bool ok = load_eeprom(fd, 0, 128, diag);
		if(!ok){
			return 1;
		}
		printf("Temperatur=%.2f\n", conv(&diag[96], 256, 1)); //C
		printf("VCC=%.2f\n", conv(&diag[98], 10000, 0)); //V
		printf("TXbias=%.2f\n", conv(&diag[100], 500, 0)); //mA
		printf("TXpower=%.3f\n", conv(&diag[102], 10000, 0)); //mW
		printf("RXpower=%d\n", (int)(conv(&diag[104], 10000, 0)*1000.0)); //mW
		printf("LaserTemp=%.2f\n", conv(&diag[106], 256, 1)); //C
		printf("TEC=%5.2f\n", conv(&diag[108], 10, 1)); // mA
	}
#endif
	close(fd);
	return 0;
}
