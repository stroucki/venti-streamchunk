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

typedef struct chunkdesc_st chunkdesc_t;

VtConn *z;
int scorefd;

void output(int type, uchar* p, unsigned int n, uchar score[VtScoreSize]) {
  n = vtzerotruncate(type, p, n);
  if (vtwrite(z, score, type, p, n) < 0) {
    sysfatal("vtwrite: %r");
  }
}

void dump_scores(void *scores, unsigned int count) {
  int wantout = count*sizeof(chunkdesc_t);
  int outbytes = write(scorefd, scores, wantout);
  if (outbytes != wantout) {
    sysfatal("could not output scores");
  }
}

void
threadmain(int argc, char *argv[])
{
	char *host;
        int type;
        chunkdesc_t* input_scores;
        
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

        input_scores = (chunkdesc_t *)malloc(1024*sizeof(chunkdesc_t));
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

        int segment_len = 0;

        unsigned int pos = 0;
        int n;
        while ((nbytes = read(scorefd, input_scores, 1024*sizeof(chunkdesc_t))) > 0) {
          while (pos * sizeof(chunkdesc_t) < nbytes) {

            segment_len = input_scores[pos].blocksize;
            n = vtread(z, input_scores[pos].score, type, buf, segment_len);
            if (n < 0) {
              sysfatal("could not read block: %r");
            }
            if (n < segment_len) {
              memset(buf+n, 0, segment_len-n);
            }
            write(1, buf, segment_len);

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
