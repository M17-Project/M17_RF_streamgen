#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LEAD_PREAM				40				//first (lead) preamble length in ms

static const uint16_t golay_encode_matrix[12]=
{
	0xC75,
	0x49F,
	0xD4B,
	0x6E3,
	0x9B3,
	0xB66,
	0xECC,
	0x1ED,
	0x3DA,
	0x7B4,
	0xB1D,
	0xE3A,
};

//An union for easy access to DEST_ and SRC_ADDR byte values
union addr
{
	uint64_t val;
	uint8_t  byte[8];
} dest_addr, src_addr;

const uint8_t pream[8]={0,1,1,1,0,1,1,1};	//preamble
const uint8_t sync_1[2]={0x32, 0x43};		//SYNC for the link setup frame
const uint8_t sync_2[2]={0x2B, 0x7E};		//SYNC for all subsequent frames

uint8_t	ConvolLUT[32];						//a look-up table for convolutional encoder
uint8_t type_1[30];						//type-1 packed bits
uint8_t type_1_u[300];						//type-1 unpacked bits
uint8_t type_2[500];						//type-2 unpacked bits
uint8_t type_3[384];
uint8_t type_4[384];
FILE	*f_out;
FILE *f_in;

//Takes an ASCII callsign in a null terminated char* and encodes it using base 40.
//Returns -1 (all Fs) if the provided callsign is longer than 9 characters, which
//would over-flow the 48 bits we have for the callsign.  log2(40^9) = 47.9
uint64_t encode_callsign_base40(const char *callsign)
{
	if (strlen(callsign) > 9)
		return -1;
	
	uint64_t encoded = 0;
	for (const char *p = (callsign + strlen(callsign) - 1); p >= callsign; p--)
	{
		encoded *= 40;
		// If speed is more important than code space, you can replace this with a lookup into a 256 byte array.
		if (*p >= 'A' && *p <= 'Z')  // 1-26
			encoded += *p - 'A' + 1;
		else if (*p >= '0' && *p <= '9')  // 27-36
			encoded += *p - '0' + 27;
		else if (*p == '-')  // 37
			encoded += 37;
		// These are just place holders. If other characters make more sense, change these.
		// Be sure to change them in the decode array below too.
		else if (*p == '/')  // 38
			encoded += 38;
		else if (*p == '.')  // 39
			encoded += 39;
		else
			// Invalid character, represented by 0.
			// Interesting artifact of using zero to flag an invalid character: invalid characters
			// at the end of the callsign won't show up as flagged at the recipient.  Because zero
			// also flags the end of the callsign.  (This started as a bug, losing As at the end of
			// the callsign, which is why A is no longer the beginning of the mapping array.)
			
			//printf("Invalid character: %c\n", *p);
			//encoded += 0;
			;
	}
	
	return encoded;
}

//Returns the golay coding of the given 12-bit word
static uint16_t golay_coding(uint16_t w)
{
	uint16_t out=0;
	
	for(uint16_t i=0; i<12; i++)
	{
		if( w & 1<<i )
			out ^= golay_encode_matrix[i];
	}
	
	return out;
}

uint32_t golay_encode(uint16_t w)
{
	return ((uint32_t)w)|((uint32_t)golay_coding(w))<<12;
}

//Initializes a Look Up Table for constraint K=5 and polynomials below
//G_1=1+D^3+D^4
//G_2=1+D+D^2+D^4
void ConvolInitLUT(uint8_t *dest)
{
	uint8_t g1, g2;
	
	memset(dest, 0, 32);
	
	//for all 2^5 states
	for(uint8_t i=0; i<32; i++)
	{
		g1=( ((i>>4)&1) + ((i>>1)&1) + ((i>>0)&1) )&1;				//G_1=1+D^3+D^4
		g2=( ((i>>4)&1) + ((i>>3)&1) + ((i>>2)&1) + ((i>>0)&1))&1;	//G_2=1+D+D^2+D^4
		dest[i]=(g1<<1)|g2;
	}
}

//Takes *len* bytes of packed data and convolutionally encodes them
//Constraint length K=5
//rate r=1/2
//G_1=1+D^3+D^4
//G_2=1+D+D^2+D^4
void ConvolEncode(uint8_t *lut, uint8_t *data_in, uint8_t *data_out, uint16_t len)
{	
	uint8_t sr=0;

	//len*8 bits of data + 4 tail bits
	for(uint16_t num_pushed=0; num_pushed<(len*8+4); num_pushed++)
	{
		sr>>=1;
		sr|=((data_in[num_pushed/8]>>(num_pushed%8))&1)<<4;
		
		//we are using a LUT for this
		if(data_out!=NULL)
			data_out[num_pushed/4]|=lut[sr]<<(6-((num_pushed*2)%8));	//changed order of dibits!
		
		printf("%d%d", lut[sr]>>1, lut[sr]&1);
	}
}

//Punctures the mother code of rate 1/2
//pattern I: leaving 46 from 61 bits
//for the link setup frame
void ConvolPuncture46_61(uint8_t *in, uint8_t *out)
{
	uint8_t punct_pattern[61]={1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1};
	
	;//
}

/*
Params:
./this out_file dest_addr src_addr

We assume that the payload is Codec2, 3200bps (full rate)
The file size in bytes would be then divisible by 8.

Out_file gets filled with unpacked (1 bit per byte) type-4 bits.
*/
int main(int argc, char *argv[])
{
	uint16_t type=(1<<0)|(0b10<<1); //type indicator - stream, voice, codec2 3200bps, no encryption
	uint8_t nonce[16]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

	if(argc==5)
	{
                // link setup
		dest_addr.val=encode_callsign_base40(argv[1]);
		src_addr.val=encode_callsign_base40(argv[2]);
		
		printf("DEST\t0x%012llX\n", dest_addr.val);
		printf("SRC \t0x%012llX\n", src_addr.val);
		
		f_out=fopen(argv[4], "wb");
		
		//preamble
		for(uint16_t i=0; i<(LEAD_PREAM*9600)/1000; i++)
			fwrite(&pream[i%8], 1, 1, f_out);
		
		//2-byte sync
		for(uint8_t i=0; i<16; i++)
		{
			uint8_t one=1, zero=0;
			
			if((sync_1[i/8]>>(7-(i%8)))&1)
				fwrite(&one, 1, 1, f_out);
			else
				fwrite(&zero, 1, 1, f_out);
		}
		
		memcpy(&type_1[0], dest_addr.byte, 6);		//DST   48 bits
		memcpy(&type_1[6], src_addr.byte, 6);		//SRC   48 bits
		memcpy(&type_1[12], (uint8_t*)&type, 2);	//TYPE  16 bits
		memcpy(&type_1[14], nonce, 16);				//NONCE 128 bits
		
		//unpacking type-1 bits
		for(uint8_t i=0; i<30; i++)
		{
			for(uint8_t j=0; j<8; j++)
			{
				type_1_u[i*8+j]=type_1[i]>>(7-j);
			}
		}
		
		//convolutionally encode type-1 to type-2
		;

                // frames
                if(!strcmp(argv[3], "-"))
                {
                        f_in = stdin;
                } else {
                        f_in = fopen(argv[1], "rb");

                        if(f_in == NULL)
                        {
                                printf("No valid input data to work with\n");

                                return 1;
                        }
                }

                uint8_t payload[16];
                size_t bytes = 0;

                while(1)
                {
                        bytes = fread(payload, 1, 16, f_in);
                        fwrite(payload, 1, 16, f_out);

                        if(bytes < 16)
                                if(feof(f_in)) break;
                }

                fclose(f_in);
                fclose(f_out);

		return 0;
	}
	else
	{
		printf("Invalid number of params\n");
		
		return 1;
	}
}

