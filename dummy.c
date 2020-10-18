/********************************************\

  Crap generator for the M17 4FSK modulator
  
\********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define SF_NUM		620		//how namy superframes of crap?

FILE *fp;

uint8_t preamble[48];
uint8_t sync[2]={0x32, 0x43};
uint8_t filler[46];

int main(void)
{
	time_t tt;
	srand(time(&tt));
	
	memset(preamble, 0xDD, 48);
	
	fp = fopen("dummy.bin", "wb");
	
	//preamble
	fwrite(preamble, 1, 48, fp);
	
	//fake LICH
	fwrite(sync, 1, 2, fp);
	for(uint8_t j=0; j<46; j++)
		filler[j]=rand()%0xFF;
	fwrite(filler, 1, 46, fp);
	
	//fake data
	for(uint16_t sf=0; sf<SF_NUM; sf++)
	{
		for(uint8_t i=0; i<5; i++)
		{
			fwrite(sync, 1, 2, fp);
			for(uint8_t j=0; j<46; j++)
				filler[j]=rand()%0xFF;
			fwrite(filler, 1, 46, fp);
		}
	}
	
	fclose(fp);
	
	return 0;
}

