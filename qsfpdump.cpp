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
#include <cmath>


const char* Red="\033[0;31m";
const char* NoColor="\033[0m";
const char* Yellow="\033[0;33m";

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
		//print_part(&data[0], "%c",   "Rev", 56, 4);
		print_part(&data[0], "%c",   "PN", 40, 16);
		print_part(&data[0], "%c",   "SN", 68, 16);
		print_part(&data[0], "%c",   "DC", 84,  6);
		
		printf("Typ=0x%02X\n", data[0]);
		printf("Connector=0x%02X\n", data[2]);
		printf("Bitrate=%u\n", data[12]*100); //MBd
		printf("Wavelength=%u\n", ((data[60]<<8)|data[61])/2); //nm
		
	}

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

		uint16_t rx_power, tx_power, rx_power_sum, tx_power_sum;
		uint8_t rx_reg_start = 34;
		uint8_t tx_reg_start = 50;
		float rx_dbm, tx_dbm, tx_dbm_sum, rx_dbm_sum;
		uint8_t offset = 0;
		printf("\n Channel Monitors \n");

		rx_power_sum = 0; tx_power_sum = 0;
		for ( uint8_t i = 0; i < 4; i++) 
		{
			rx_power = (diag[rx_reg_start + offset]<<8)|diag[rx_reg_start + offset + 1];
			tx_power = (diag[tx_reg_start + offset]<<8)|diag[tx_reg_start + offset + 1];
			rx_power_sum += rx_power;
			tx_power_sum += tx_power;
	                rx_dbm=10*std::log10((float)rx_power/10000);
			tx_dbm=10*std::log10((float)tx_power/10000);
	                
			printf("Rx%d Power: %d. Dbm: %f", i, rx_power, rx_dbm);
	                if(rx_dbm < -6) 
	                	printf(" %sERROR%s \n", Red, NoColor);
			else if (rx_dbm < -3)
				printf(" %sATTENTION%s\n", Yellow, NoColor);
			else
				printf(" OK\n");
		
			printf("Tx%d Power: %d. Dbm: %f", i, tx_power, tx_dbm);
                        if(tx_dbm < -6) 
                                printf(" %sERROR%s \n", Red, NoColor);
                        else if (tx_dbm < -3) 
				printf(" %sATTENTION%s\n", Yellow, NoColor);
                        else
                                printf(" OK\n"); 
			offset += 2;
		}
                rx_dbm_sum=10*std::log10((float)rx_power_sum/10000);
                tx_dbm_sum=10*std::log10((float)tx_power_sum/10000);
		
		printf("SUM Rx Power: %d. Dbm: %f", rx_power_sum, rx_dbm_sum);
                if(rx_dbm_sum < -6) 
	                printf(" %sERROR%s\n ", Red, NoColor);
                else if (rx_dbm_sum < 2) 
			printf(" %sATTENTION%s\n", Yellow, NoColor);
                else
                        printf(" OK\n");
                       

		printf("SUM Tx Power: %d. Dbm: %f", tx_power_sum, tx_dbm_sum);
                if(tx_dbm_sum < -6) 
	                printf(" %sERROR%s\n ", Red, NoColor);
                else if (tx_dbm_sum < 4) 
			printf(" %sATTENTION%s\n", Yellow, NoColor);
                else
                        printf(" OK\n");

//		printf("Temperatur=%.2f\n", conv(&diag[96], 256, 1)); //C
//		printf("VCC=%.2f\n", conv(&diag[98], 10000, 0)); //V
//		printf("TXbias=%.2f\n", conv(&diag[100], 500, 0)); //mA
//		printf("TXpower=%.3f\n", conv(&diag[102], 10000, 0)); //mW
//		printf("RXpower=%d\n", (int)(conv(&diag[104], 10000, 0)*1000.0)); //mW
//		printf("LaserTemp=%.2f\n", conv(&diag[106], 256, 1)); //C
//		printf("TEC=%5.2f\n", conv(&diag[108], 10, 1)); // mA
	}

	close(fd);
	return 0;
}
