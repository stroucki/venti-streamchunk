/*
    streamchunkread - given a list of size and score pairs, retrieve the
    data from venti and create a stream

    Copyright (c) 2019 Michael Stroucken <mxs@cmu.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <u.h>
#include <libc.h>
#include <venti.h>
#include <libsec.h>
#include <thread.h>
#include <fcntl.h>

enum
{
	// XXX What to do here?
	VtMaxLumpSize = 65535,
};

void
usage(void)
{
	fprint(2, "usage: streamchunkread [-h host] scorefile\n");
	threadexitsall("usage");
}

struct chunkdesc_st {
  unsigned int blocksize;
  uchar score[VtScoreSize];
};

uchar input[4 + VtScoreSize];

typedef struct chunkdesc_st chunkdesc_t;

VtConn *z;
int scorefd;

void
threadmain(int argc, char *argv[])
{
	char *host;
        int type;
        uchar* scorebuf;
        
        unsigned char outbuf[65536];
        unsigned int nbytes = 0;

	fmtinstall('F', vtfcallfmt);
	fmtinstall('V', vtscorefmt);

	host = nil;
	type = VtDataType;
	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

        scorebuf = (uchar *)malloc(1024*sizeof(input));
        if (!scorebuf) {
          sysfatal("could not initialize input scores buffer");
        }

        if (argv[0][0] == '-' && argv[0][1] == 0) {
          scorefd = 0;
        } else {
          scorefd = open(argv[0], OREAD);
          if (scorefd < 0) {
            sysfatal("could not open score file: %r");
          }
        }

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");
	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

        int n;
        unsigned int size;
        uchar *scoreptr;
        uchar* scorebufinptr = scorebuf;
        uchar* scorebufoutptr = scorebuf;
        while ((nbytes = read(scorefd, scorebufinptr, 1024*sizeof(input) - (scorebufinptr - scorebuf))) > 0) {
          scorebufinptr += nbytes;
          while (scorebufoutptr + sizeof(input) <= scorebufinptr) {
            scoreptr = scorebufoutptr;
            size = scoreptr[0];
            size += scoreptr[1] << 8;
            size += scoreptr[2] << 16;
            size += scoreptr[3] << 24;
            n = vtread(z, &scoreptr[4], type, outbuf, size);
            if (n < 0) {
              sysfatal("could not read block: %r");
            }
            if (n < size) {
              memset(outbuf+n, 0, size-n);
            }
            write(1, outbuf, size);

            scorebufoutptr += sizeof(input);
          }

          if (scorebufinptr == scorebufoutptr) {
            scorebufinptr = scorebufoutptr = scorebuf;
          }
        }

        if (close(scorefd)) {
          sysfatal("problems closing score file");
        }


        free(scorebuf);
	vthangup(z);
	threadexitsall(0);
}
