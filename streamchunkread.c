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
        uchar* input_scores;
        
        unsigned char buf[65536];
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

        input_scores = (uchar *)malloc(1024*sizeof(input));
        if (!input_scores) {
          sysfatal("could not initialize input scores buffer");
        }

        scorefd = open(argv[0], OREAD);
        if (scorefd < 0) {
          sysfatal("could not open score file: %r");
        }

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");
	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

        unsigned int pos = 0;
        int n;
        unsigned int size;
        uchar *scoreptr;
        while ((nbytes = read(scorefd, input_scores, 1024*sizeof(input))) > 0) {
          while (pos * sizeof(input) < nbytes) {
            scoreptr = input_scores + pos * sizeof(input);
            size = scoreptr[0];
            size += scoreptr[1] << 8;
            size += scoreptr[2] << 16;
            size += scoreptr[3] << 24;
            n = vtread(z, &scoreptr[4], type, buf, size);
            if (n < 0) {
              sysfatal("could not read block: %r");
            }
            if (n < size) {
              memset(buf+n, 0, size-n);
            }
            write(1, buf, size);

            pos++;
          }
        pos = 0;
      }

      if (close(scorefd)) {
        sysfatal("problems closing score file");
      }


        free(input_scores);
	vthangup(z);
	threadexitsall(0);
}
