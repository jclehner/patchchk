#if !defined(_WIN32)
#include <arpa/inet.h>
#elif defined(_MSC_VER)
#include <BaseTsd.h>
#define __attribute__(x)
typedef SSIZE_T ssize_t;
#define htonl(x) _byteswap_ulong((x))
#define ntohl(x) _byteswap_ulong((x))
#elif defined(__GNUC__)
#define htonl(x) __builtin_bswap32((x))
#define ntohl(x) __builtin_bswap32((x))
#endif
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef __GNUC__
#include <getopt.h>
#endif // _GNUC

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct chk_header {
	uint32_t magic;
	uint32_t header_len;
	uint8_t  version[8];
	uint32_t kernel_chksum;
	uint32_t rootfs_chksum;
	uint32_t kernel_len;
	uint32_t rootfs_len;
	uint32_t image_chksum;
	uint32_t header_chksum;
	/* char board_id[] */
};

// chechksum code credit to OpenWRT's mkchkimg

struct ntgr_checksum
{
	uint32_t c0;
	uint32_t c1;
};

static void ntgr_checksum_init(struct ntgr_checksum* c)
{
	c->c0 = c->c1 = 0;
}

static void ntgr_checksum_add(struct ntgr_checksum* c, const void* bufp, size_t len)
{
	const unsigned char* buf = bufp;
	size_t i;

	for (i = 0; i < len; i++) {
		c->c0 += buf[i] & 0xff;
		c->c1 += c->c0;
	}
}

static inline unsigned long ntgr_checksum_fini(struct ntgr_checksum* c)
{
	uint32_t b, checksum;

	b = (c->c0 & 65535) + ((c->c0 >> 16) & 65535);
	c->c0 = ((b >> 16) + b) & 65535;
	b = (c->c1 & 65535) + ((c->c1 >> 16) & 65535);
	c->c1 = ((b >> 16) + b) & 65535;
	checksum = ((c->c1 << 16) | c->c0);

	return checksum;
}

int usage(int ret)
{
	fprintf(stderr,
		"Usage: patchchk [OPTIONS...] FILE\n"
		"\n"
		"Options:\n"
		" -v [version]  Change version\n"
		" -r [region]   Change region\n"
		" -b [board id] Change board id\n"
		"\n"
		"patchchk v" VERSION ", Copyright (C) 2022 Joseph C. Lehner\n"
		"patchchk is free software, licensed under the GNU GPLv3\n"
		"Source code at https://github.com/jclehner/patchchk\n"
	);

	return ret;
}

static const char* regions[] = {
	"",
	"WW",
	"NA",
};

static void die(const char* format, ...) __attribute__((noreturn, format(printf, 1, 2)));

static void die(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

void parse_region(uint8_t* version, const char* str)
{
	int n = atoi(str);

	if (n && n < 0xff) {
		version[0] = n & 0xff;
		return;
	} else if (!n) {
		for (int i = 0; i < ARRAY_SIZE(regions); ++i) {
			if (!strcmp(str, regions[i])) {
				version[0] = i & 0xff;
				return;
			}
		}
	}

	die("Error: unknown region '%s'\n", str);
}

static const char* region_to_str(struct chk_header* hdr)
{
	int region = hdr->version[0];

	if (region < ARRAY_SIZE(regions)) {
		if (*regions[region] != '\0') {
			return regions[region];
		}
	}

	return "??";
}

static const char* version_to_str(struct chk_header* hdr)
{
	static char buf[128];
	uint8_t* v = hdr->version;

	snprintf(buf, sizeof(buf), "%u.%u.%u.%u_%u.%u.%u",
			v[1], v[2], v[3], v[4], v[5], v[6], v[7]);

	return buf;
}

void parse_version(uint8_t* version, const char* str)
{
	unsigned v[7];
	int n = sscanf(str, "%u.%u.%u.%u_%u.%u.%u", &v[0], &v[1], &v[2], &v[3],
			&v[4], &v[5], &v[6]);

	if (n == 7) {
		for (int i = 0; i < sizeof(v) / sizeof(v[0]); ++i) {
			if (v[i] < 0x100) {
				version[i + 1] = v[i] & 0xff;
			} else {
				die("Error: invalid version part %d (must be < 256)\n", v[i]);
			}
		}
	} else {
		die("Error: version must be 'A.B.C.D_E.F.G'\n");
	}
}

int main(int argc, char** argv)
{
	char* version = NULL;
	char* region = NULL;
	char* new_board_id = NULL;

#ifndef _MSC_VER
	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, "+r:v:b:")) != -1) {
		switch (c) {
		case 'r':
			region = optarg;
			break;
		case 'v':
			version = optarg;
			break;
		case 'b':
			new_board_id = optarg;
			break;
		case 'h':
			return usage(0);
		default:
			return usage(1);
		}
	}
#else
	int optind = 1;
	for (; optind < argc; ++optind) {
		if (argv[optind][0] == '-') {
			if ((optind + 1) < argc) {
				switch (argv[optind][1]) {
				case 'r':
					region = argv[++optind];
					break;
				case 'v':
					version = argv[++optind];
					break;
				case 'b':
					new_board_id = argv[++optind];
					break;
				default:
					return usage(1);
				}
			}
		} else {
			break;
		}
	}

#endif

	if (optind >= argc) {
		return usage(1);
	}

	FILE* fp = fopen(argv[optind], "rb+");
	if (!fp) {
		perror("Error: fopen");
		return 1;
	}

	struct chk_header hdr;
	if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
		die("Error: failed to read header\n");
	}

	if (ntohl(hdr.magic) != 0x2a23245e) {
		die("Error: not a CHK image file\n");
	}

	unsigned hdr_len = ntohl(hdr.header_len);
	ssize_t board_len = hdr_len - sizeof(hdr);
	if (board_len <= 0) {
		die("Error: invalid header length %u\n", hdr_len);
	}

	char* board_id = malloc(board_len + 1);
	if (fread(board_id, board_len, 1, fp) != 1) {
		die("Error: failed to read board ID\n");
	}

	board_id[board_len] = '\0';

	if (region) {
		parse_region(hdr.version, region);
		printf("Region  : %s\n", region_to_str(&hdr));
	}

	if (version) {
		parse_version(hdr.version, version);
		printf("Version : %s\n", version_to_str(&hdr));
	}

	if (new_board_id) {
		printf("Board   : %s\n", new_board_id);
		free(board_id);
		board_id = new_board_id;
	}

	if (region || version || new_board_id) {
		hdr.header_chksum = 0;

		struct ntgr_checksum chk;
		ntgr_checksum_init(&chk);
		ntgr_checksum_add(&chk, &hdr, sizeof(hdr));
		ntgr_checksum_add(&chk, board_id, strlen(board_id));
		hdr.header_chksum = htonl(ntgr_checksum_fini(&chk));

		printf("Checksum: 0x%08" PRIx32 "\n", ntohl(hdr.header_chksum));

		rewind(fp);

		if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1) {
			die("Error: failed to write header\n");
		}

		if (fwrite(board_id, board_len, 1, fp) != 1) {
			die("Error: failed to write board ID\n");
		}

		fclose(fp);
		return 0;
	}

	printf("Board ID       : %s\n", board_id);
	printf("Region         : %s\n", region_to_str(&hdr));
	printf("Version        : %s\n", version_to_str(&hdr));
	printf("Sizes\n");
	printf("- Header       : %" PRIu32 "\n", hdr_len);
	printf("- Kernel       : %" PRIu32 "\n", ntohl(hdr.kernel_len));
	printf("- Root FS      : %" PRIu32 "\n", ntohl(hdr.rootfs_len));
	printf("Checksums\n");
	printf("- Header       : 0x%08" PRIx32 "\n", ntohl(hdr.header_chksum));
	printf("- Kernel       : 0x%08" PRIx32 "\n", ntohl(hdr.kernel_chksum));
	printf("- Root FS      : 0x%08" PRIx32 "\n", ntohl(hdr.rootfs_chksum));
	printf("- Image        : 0x%08" PRIx32 "\n", ntohl(hdr.image_chksum));

	return 0;
}
