#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

#define BLKLEN 240
#define SYMBLK 39
#define SINGLE 0407
#define SHARED 0410
#define IANDD 0411

void checksum(unsigned char *bp);
void addsymbol(char *name, int type, unsigned short value);
int rad50(char **sym);
void rad50out(int sym, char *out);

struct ldasym {
    unsigned short name1;
    unsigned short name2;
    unsigned short value;
};
struct ldasym isymbols[025252];
struct ldasym dsymbols[025252];
struct ldasym *istp = isymbols;
struct ldasym *dstp = dsymbols;
struct ldasym *stp = isymbols;

int main(int argc, char **argv) {
    int fd, ofd, i, phase;
    char ofile[256];
    ssize_t len, want;

    unsigned char ispace[0200000] = {0};
    unsigned char dspace[0200000] = {0};
    unsigned char *memory;
    unsigned char symbols[0200000] = {0};
    char symbolnames[100000];
    int namesize;

    struct {
	unsigned short magic;
	unsigned short text;
	unsigned short data;
	unsigned short bss;
	unsigned short symbols;
	unsigned short entry;
	unsigned short unused;
	unsigned short flag;
    } header;

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned short address;
	unsigned char geninfo;
	unsigned char words;
	unsigned short start;
	unsigned short size;
	unsigned short transfer;
	unsigned short ddt;
	unsigned short core;
	unsigned short name1;
	unsigned short name2;
	unsigned short ident1;
	unsigned short ident2;
	unsigned short time1;
	unsigned short time2;
	unsigned short date;
	unsigned short emtsres;
	unsigned short endcomd;
	unsigned char checksum;
    } comd = { 1, 36, 0, 1, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0};

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned short address;
	unsigned char data[BLKLEN];
	unsigned char checksum;
    } block = { 1, 246, 0, {0}, 0 };

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned short address;
	unsigned char checksum;
    } transfer = { 1, 6, 0177777, 0 };

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned char count;
	unsigned char type;
	struct ldasym modules[2];
	unsigned char checksum;
    } modtab = { 1, 18, 1, -1, { {050561, 053600, 0}, {-1, -1, 0} }, 0 };

    struct seclim {
	unsigned short limit;
	unsigned char module;
	unsigned char space;
    };

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned char count;
	unsigned char type;
	struct seclim text;
	struct seclim data;
	struct ldasym end;
	unsigned char checksum;
    } modsec = { 1, 20, 1, 0200, {0, 1, 0}, {0, 1, 0}, {-1, -1, 0}, 0 };

    struct {
	unsigned short hdr;
	unsigned short length;
	unsigned char count;
	unsigned char module;
	struct ldasym symbols[SYMBLK];
	struct ldasym end;
	unsigned char checksum;
    } symblk = {1, 246, 1, 1, { {0} }, {-1, -1, 0}, 0};

    unsigned int count;
    unsigned int remaining;
    unsigned char *bp;

    if (argc != 2) {
	fprintf(stderr, "Usage: atolda file\n");
	exit(1);
    }

    // Open the input file in a.out format and read the header and contents
    // into memory.

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	perror("input open failed");
	exit(1);
    }

    len = read(fd, &header, sizeof(header));
    if (len != sizeof(header)) {
	printf("read only %zd bytes while looking for header\n", len);
	exit(1);
    }
    if (header.magic == SINGLE) {
	want = header.text + header.data;
	len = read(fd, ispace, want);
	if (len != want) {
	    printf("read %zd bytes of text+data while expecting %zd\n",
		   len, want);
	    exit(1);
	}
    } else {
	len = read(fd, ispace, header.text);
	if (len != header.text) {
	    printf("read %zd bytes of text while expecting %zd\n",
		   len, header.text);
	    exit(1);
	}
	len = read(fd, dspace, header.data);
	if (len != header.data) {
	    printf("read %zd bytes of data while expecting %zd\n",
		   len, header.data);
	    exit(1);
	}
    }	
    len = read(fd, symbols, header.symbols);
    if (len != header.symbols) {
	printf("read %zd bytes of symbols while expecting %zd\n",
	       len, header.symbols);
	exit(1);
    }
    if (symbols[0] == 0) {
	len = read(fd, symbolnames, sizeof(symbolnames));
	if (len < sizeof(namesize)) {
	    printf("read %zd bytes of symbol name length while expecting %zd\n",
		   len, sizeof(namesize));
	    exit(1);
	}
	namesize = (*(short*)symbolnames << 16) | *(short*)&symbolnames[2];
	if (len < namesize) {
	    if (len < sizeof(symbolnames)) {
		printf("read %zd bytes of symbol names while expecting %zd\n",
		       len, namesize);
	    } else {
		printf("symbol table size %zd bytes exceeds space %zd\n",
		       namesize, sizeof(symbolnames));
	    }
	    exit(1);
	}
    }

    // The a.out file was all read successfully, so open the output.

    snprintf(ofile, sizeof(ofile), "%s.lda", argv[1]);
    ofd = open(ofile, O_WRONLY | O_CREAT | O_TRUNC);
    if (ofd < 0) {
	perror("output open failed");
	exit(1);
    }
    if (fchmod(ofd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
	perror("output chmod failed");
	exit(1);
    }

    comd.size = header.text;
    // %%% Should write program name, ident, date and time into COMD

    remaining = header.text;
    block.address = 0;
    memory = ispace;

    for (phase = 1; phase < 3; ++phase) {
	if (header.magic == SINGLE) {
	    remaining += header.data;
	    comd.size += header.data;
	    comd.transfer = header.entry;
	    transfer.address = header.entry;
	    ++phase;
	}

	if (phase == 1 || header.magic != SHARED) {
	    checksum((unsigned char*)&comd);
	    len = write(ofd, &comd, comd.length + 1);
	    if (len != comd.length + 1) {
		printf("wrote %zd bytes of comd block while expecting %zd\n",
		       len, comd.length + 1);
		exit(1);
	    }
	}

	count = BLKLEN;
	while (1) {
	    if (count > remaining)
		count = remaining;
	    block.length = count + 6;
	    bcopy(memory, block.data, count);
	    checksum((unsigned char*)&block);

	    len = write(ofd, &block, block.length + 1);
	    if (len != block.length + 1) {
		printf("wrote %zd bytes of block while expecting %zd\n",
		       len, block.length + 1);
		exit(1);
	    }

	    if (remaining <= BLKLEN)
		break;
	    remaining -= count;
	    block.address += count;
	    memory += count;
	}

	if (phase == 2 || header.magic == IANDD) {
	    checksum((unsigned char*)&transfer);
	    len = write(ofd, &transfer, transfer.length + 1);
	    if (len != transfer.length + 1) {
		printf("wrote %zd bytes of transfer block while expecting %zd\n",
		       len, transfer.length + 1);
		exit(1);
	    }
	}

	remaining = header.data;
	transfer.address = header.entry;
	memory = dspace;
	if (header.magic == SHARED) {
	    block.address += 0017777;
	    block.address &= 0160000;
	} else if (header.magic == IANDD) {
	    block.address = 0;
	    comd.size = header.data;
	    comd.transfer = 0177777;
	}
    }

    if (symbols[0] == 0) {
	for (i = 0; i < header.symbols; i += 8) {
	    int module = *(short*)&symbols[i];
	    int offset = *(short*)&symbols[i + 2];
	    int type = symbols[i + 4];
	    unsigned short value = *(short*)&symbols[i + 6];
	    char *name = &symbolnames[offset];
	    printf("%07o %6d %06o %02o %s\n", module, offset, value, type, name);
	    if (module == 0)
		addsymbol(name, type, value);
	}
    } else {
	for (i = 0; i < header.symbols; i += 12) {
	    char *name = &symbols[i];
	    int type = symbols[i + 8];
	    unsigned short value = *(short*)&symbols[i + 10];
	    symbols[i + 8] = '\0';
	    printf("%-8s %06o %02o\n", name, value, type);
	    addsymbol(name, type, value);
	}
    }

    checksum((unsigned char*)&modtab);
    len = write(ofd, &modtab, modtab.length + 1);
    if (len != modtab.length + 1) {
	printf("wrote %zd bytes of module table block while expecting %zd\n",
	       len, modtab.length + 1);
	exit(1);
    }
    if (header.magic == IANDD)
	modsec.data.space = 1;
    checksum((unsigned char*)&modsec);
    len = write(ofd, &modsec, modsec.length + 1);
    if (len != modsec.length + 1) {
	printf("wrote %zd bytes of module section limit block while expecting %zd\n",
	       len, modsec.length + 1);
	exit(1);
    }

    while (1) {
	for (i = 0; i < SYMBLK; ++i) {
	    if (stp == istp || stp == dstp) break;
	    symblk.symbols[i].name1 = (*stp).name1;
	    symblk.symbols[i].name2 = (*stp).name2;
	    symblk.symbols[i].value = (*stp).value;
	    ++stp;
	    {
		char name[6];
		rad50out(symblk.symbols[i].name1, name);
		rad50out(symblk.symbols[i].name2, &name[3]);
		printf("%-6.6s = %06o\n", name, symblk.symbols[i].value);
	    }
	}
	if (i == 0) break;
	if (i < SYMBLK) {
	    symblk.symbols[i].name1 = -1;
	    symblk.symbols[i].name2 = -1;
	    symblk.symbols[i].value = 0;
	}
	symblk.length = (i + 1) * 6 + 6;
	checksum((unsigned char*)&symblk);
	len = write(ofd, &symblk, symblk.length + 1);
	if (len != symblk.length + 1) {
	    printf("wrote %zd bytes of symbol block while expecting %zd\n",
		   len, symblk.length + 1);
	    exit(1);
	}
	if (i < SYMBLK) {
	    if (stp == dstp) break;
	    stp = dsymbols;
	}
    }

    // Write a symbol table end marker block
    symblk.symbols[0].name1 = -1;
    symblk.symbols[0].name2 = -1;
    symblk.symbols[0].value = 0;
    symblk.length = 1 * 6 + 6;
    symblk.count = 0;
    checksum((unsigned char*)&symblk);
    len = write(ofd, &symblk, symblk.length + 1);
    if (len != symblk.length + 1) {
	printf("wrote %zd bytes of symbol end block while expecting %zd\n",
	       len, symblk.length + 1);
	exit(1);
    }

    return 0;
}

void checksum(unsigned char *bp) {
    unsigned char sum = 0;
    int length = *(bp+2) + (*(bp+3) << 8);
    int i;
    for (i = 0; i < length; ++i) {
	sum += *bp++;
    }
    *bp = -sum;
}

void addsymbol(char *name, int type, unsigned short value) {
    switch (type) {
    case 000:		// undefined symbol
    case 037:		// file name symbol (produced by ld)
    case 040:		// undefined external (.globl) symbol
    default:
	break;

    case 002:		// text segment symbol
	if (*name == '/') break;
	if (*(name + strlen(name) - 2) == '.' &&
	    *(name + strlen(name) - 1) == 'o') break;
	// fall thru

    case 001:		// absolute symbol
    case 041:		// absolute external symbol
    case 042:		// text segment external symbol
	while (*name == '_' || *name == '~') ++name;
	(*istp).name1 = rad50(&name);
	(*istp).name2 = rad50(&name);
	(*istp).value = value;
	++istp;
	break;

    case 003:		// data segment symbol
    case 004:		// bss segment symbol
    case 043:		// data segment external symbol
    case 044:		// bss segment external symbol		
	while (*name == '_' || *name == '~') ++name;
	(*dstp).name1 = rad50(&name);
	(*dstp).name2 = rad50(&name);
	(*dstp).value = value;
	++dstp;
	break;
    }
}

int rad50(char **sym) {
    static char R50tbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$._0123456789";
    int result = 0;
    int i;
    for (i = 0; i < 3; ++i) {
	char ch = **sym;
	int r = 0;
	if (ch != '\0') {  
	    ch = toupper(ch);
	    for ( ; r < 050; ++r) {
		if (ch == R50tbl[r])
		    break;
	    }
	    ++*sym;
	}
	if (r == 050)
	    r = 0;
	result *= 050;
	result += r;
    }
    return result;
}

void rad50out(int sym, char *out) {
    static char R50tbl[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$._0123456789";
    *(out + 2) = R50tbl[sym % 050];
    sym /= 050;
    *(out + 1) = R50tbl[sym % 050];
    sym /= 050;
    *out = R50tbl[sym % 050];
}
