/*
 * tool64
 * By Dylan Johnston
 *
 * This simple tool prints N64 ROM header information and converts between
 * different rom formats (v64, n64, z64).
 */

#define N64 0x40123780 // little endian
#define Z64 0x80371240 // big endian
#define V64 0x37804012 // byteswaped.

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct rom_header_s {
	uint8_t lat;
	uint8_t pgs1;
	uint8_t pwd;
	uint8_t pgs2;
	uint32_t clock_rate;
	uint32_t progam_counter;
	uint32_t release;
	uint32_t crc1;
	uint32_t crc2;
	uint64_t unk1;
	uint8_t name[20];
	uint32_t unk2;
	uint32_t manufacturer;
	uint16_t cartridge_id;
	uint8_t country_code;
	uint8_t version;
} rom_header_t;

typedef struct rom_s {
	FILE *f;
	rom_header_t *h;
	uint32_t fmt;
} rom_t;

const char   *country_str(uint8_t c);
int           load_rom(rom_t *, char *);
uint32_t      format(rom_header_t *);
const char   *format_str(uint32_t);
uint32_t      clockrate(rom_header_t *);
void          swap(uint8_t *, int, int);
void          dwordswap(uint8_t *, int);
void          wordswap(uint8_t *, int);
char         *ext(char *);
rom_header_t *header(FILE *);
int           rom_size(FILE *);

const char *country_str(uint8_t c) {
	char *l;

	switch(c) {
		case 0x37:
			return "Beta";
		case 0x41:
			return "Asia (NTSC)";
		case 0x42:
			return "Brazil";
		case 0x43:
			return "China";
		case 0x44:
			return "Germany";
		case 0x45:
			return "North America";
		case 0x46:
			return "France";
		case 0x47:
			return "Gateway 64 (NTSC)";
		case 0x48:
			return "Netherlands";
		case 0x49:
			return "Italy";
		case 0x4A:
			return "Japan";
		case 0x4B:
			return "Korean";
		case 0x4C:
			return "Gateway 64 (PAL)";
		case 0x4E:
			return "Canada";
		case 0x50:
			return "Europe";
		case 0x53:
			return "Spain";
		case 0x55:
			return "Australia";
		case 0x57:
			return "Scandinavia";
		case 0x58:
			return "Europe";
		case 0x59:
			return "Europe";
		default:
			return "Unknown";
	}
};

uint32_t format(rom_header_t* h) {
	uint32_t f;

	f  = (h->lat  & 0xFF) << 24;
	f |= (h->pgs1 & 0xFF) << 16;
	f |= (h->pwd  & 0xFF) << 8;
	f |= (h->pgs2 & 0xFF);

	return f;
}

const char *format_str(uint32_t fmt) {
	switch(fmt) {
		case N64:
			return "Little Endian";
		case V64:
			return "Byte Swapped";
		case Z64:
			return "Big Endian";
		case 0:
			return "NULL";
		default:
			printf("problemi %x\n", fmt);
			return "Unknown";
	}
}

uint32_t clockrate(rom_header_t* h) {
	uint32_t cr = 0;

	cr =  ((h->clock_rate      ) & 0xFF) >> 24;
	cr |= ((h->clock_rate << 8 ) & 0xFF) >> 16;
	cr |= ((h->clock_rate << 16) & 0xFF) >> 8;
	cr |= ((h->clock_rate << 24) & 0xFF);

	return cr;
}

/*
   0000h              (1 byte): initial PI_BSB_DOM1_LAT_REG value (0x80)
   0001h              (1 byte): initial PI_BSB_DOM1_PGS_REG value (0x37)
   0002h              (1 byte): initial PI_BSB_DOM1_PWD_REG value (0x12)
   0003h              (1 byte): initial PI_BSB_DOM1_PGS_REG value (0x40)
   0004h - 0007h     (1 dword): ClockRate
   0008h - 000Bh     (1 dword): Program Counter (PC)
   000Ch - 000Fh     (1 dword): Release
   0010h - 0013h     (1 dword): CRC1
   0014h - 0017h     (1 dword): CRC2
   0018h - 001Fh    (2 dwords): Unknown (0x0000000000000000)
   0020h - 0033h    (20 bytes): Image name
   Padded with 0x00 or spaces (0x20)
   0034h - 0037h     (1 dword): Unknown (0x00000000)
   0038h - 003Bh     (1 dword): Manufacturer ID
   0x0000004E = Nintendo ('N')
   003Ch - 003Dh      (1 word): Cartridge ID
   003Eh - 003Fh      (1 word): Country code
   0x4400 = Germany ('D')
   0x4500 = USA ('E')
   0x4A00 = Japan ('J')
   0x5000 = Europe ('P')
   0x5500 = Australia ('U')
   0040h - 0FFFh (1008 dwords): Boot code
   */

char *ext(char *filename) {
	char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "";
	return dot + 1;
}

rom_header_t* header(FILE *f) {
	rom_header_t* x = malloc(sizeof(rom_header_t));

	fread(x, sizeof(rom_header_t), 1, f);
	rewind(f);

	return x;
}

int rom_size(FILE *f) {
	int s = 0;
	fseek(f, 0, SEEK_END);
	s = ftell(f);
	rewind(f);
	return s;
}

int print_help() {
	printf("tool64\n");
	printf(" example: tool64 info foo.z64\n");
	printf("\n");
	printf("options:\n");
	printf(" info - print rom header info\n");
	printf(" z64 - convert rom to z64 (big endian)\n");
	return EXIT_SUCCESS;
}

// tool64 info xyz.n64 
// 1      2    3
int info(int argc, char **argv) {
	if (argc <= 2) {
		printf("not enough arguments for info\n");
		return EXIT_FAILURE;
	}

	char *path = argv[2];

	rom_t rom;
	if (load_rom(&rom, path) != 0) {
		printf("failed to load rom\n");
		return EXIT_FAILURE;
	}

	rom_header_t *h = rom.h;

	printf("t: %x %x %x %x\n", h->lat, h->pgs1, h->pwd, h->pgs2);
	printf("Path: %s\n", path);
	printf("Name: %s\n", h->name);
	printf("Region: %s (0x%x)\n", country_str(h->country_code), h->country_code);
	printf("Clock Rate: %d\n", h->clock_rate);
	printf("CRC 1: %x\n", h->crc1);
	printf("CRC 2: %x\n", h->crc2);
	printf("File Extension: %s\n", ext(path));
	printf("Format: %s\n", format_str(rom.fmt));
	printf("Size: %d\n", rom_size(rom.f));

	return EXIT_SUCCESS;
};

// tool64 v64 xyz.n64 
// tool64 v64 xyz.n64 xyz2.v64
// 1      2   3       (4)
int convert(int argc, char **argv, int dst_format) {
	if (argc < 3) {
		printf("Error: Not enough arguments for conversion\n");
		return EXIT_FAILURE;
	}

	char *dst_path = 0;
	/* char *dst_format = argv[1]; */
	char *src_path = argv[2];

	if (argc > 3) {
		dst_path = argv[3];
	}

	rom_t rom;
	if (load_rom(&rom, src_path) != 0) {
		printf("Error: Failed to load ROM\n");
		return EXIT_FAILURE;
	}

	if (rom.fmt == dst_format) {
		printf("Input is already %s. Nothing to do!\n", format_str(rom.fmt));
		return EXIT_SUCCESS;
	}

	printf("Converting %s to %s\n", format_str(rom.fmt), format_str(dst_format));

	int romsize = rom_size(rom.f);
	uint8_t* data = malloc(sizeof(uint8_t) * romsize);
	if (data == 0) {
		printf("Error: Failed to retrieve memory\n");
		return EXIT_FAILURE;
	}
	memset(data, 0, sizeof(data));

	int l = fread(data, sizeof(uint8_t), romsize, rom.f);
	if (l != romsize) {
		printf("Error: Unexpected file length\n");
		return EXIT_FAILURE;
	}

	// convert to big endian.
	if (rom.fmt == N64) {
		dwordswap(data, romsize);
	}
	if (rom.fmt == V64) {
		wordswap(data, romsize);
	}

	// then convert to final format
	if (dst_format == V64) {
		wordswap(data, romsize);
	}

	if (dst_format == N64) {
		dwordswap(data, romsize);
	}

	FILE* pFile = fopen ("myfile.bin", "wb");
	fwrite (data, sizeof(uint8_t), romsize, pFile);
	fclose (pFile);

	return EXIT_SUCCESS;
};

void downcase(char *str) {
	int i = 0;
	while( str[i] ) {
		str[i] = tolower(str[i]);
		i++;
	}
};


int load_rom(rom_t *rom, char *path) {
	FILE *f = fopen(path, "rb");
	if (f == 0) {
		return 1;
	}

	rom_header_t *h = header(f);
	uint32_t fmt = format(h);

	if (fmt == N64) {
		dwordswap((uint8_t*)h, sizeof(rom_header_t));  
	}
	if (fmt == V64) {
		wordswap((uint8_t*)h, sizeof(rom_header_t));  
	}

	rom->f = f;
	rom->h = h;
	rom->fmt = fmt;

	return 0;
};

int main(int argc, char **argv) {
	if (argc < 2) {
		print_help();
		return EXIT_FAILURE;
	}

	char *cmd = argv[1];
	downcase(cmd);

	if (strncmp(cmd, "help", 4) == 0) {
		return print_help();
	} else if (strncmp(cmd, "info", 4) == 0) {
		return info(argc, argv);
	} else if (strcmp(cmd, "z64") == 0) {
		return convert(argc, argv, Z64);
	} else if (strcmp(cmd, "n64") == 0) {
		return convert(argc, argv, N64);
	} else if (strcmp(cmd, "v64") == 0) {
		return convert(argc, argv, V64);
	} else {
		printf("unknown command %s\n", cmd);
		return EXIT_FAILURE;
	}
}

// ABCD -> BADC
void wordswap(uint8_t* buf, int len) {
	for(int i = 0; i < len; i += 2) {
		swap(buf, i, i+1);
	}
}

// ABCD -> DCBA
void dwordswap(uint8_t *buf, int len) {
	for(int i = 0; i < len; i += 4) {
		swap(buf, i, i+3);
		swap(buf, i+1, i+2);
	}
}

void swap(uint8_t *buf, int a, int b) {
	char x = buf[a];
	buf[a] = buf[b];
	buf[b] = x;
}
