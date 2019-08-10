/*
    streamchunkwrite - given a file to receive size and score pairs, read
    data from a stream, chunk the data into variable length segments, and
    store those to venti.

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
#include "dedup.h"

enum
{
	// XXX What to do here?
	VtMaxLumpSize = 65535,
};

void
usage(void)
{
	fprint(2, "usage: streamchunkwrite [-h host] scorefile\n");
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

void dump_scores(chunkdesc_t *chunks, unsigned int count) {
  const int size = 4 + VtScoreSize;
  uchar output[size];
  for (unsigned int n = 0; n < count; n++) {
    chunkdesc_t *chunk = &chunks[n];
    output[0] = chunk->blocksize & 0xff;
    output[1] = (chunk->blocksize >> 8) & 0xff;
    output[2] = (chunk->blocksize >> 16) & 0xff;
    output[3] = (chunk->blocksize >> 24) & 0xff;
    memcpy(&output[4], chunk->score, VtScoreSize);
    int wantout = size;
    int outbytes = write(scorefd, output, wantout);
    if (outbytes != wantout) {
      sysfatal("could not output scores");
    }
  }
}

void
threadmain(int argc, char *argv[])
{
	char *host;
	int type;
	uchar *p;
        chunkdesc_t* output_scores;
        rabinpoly_t* poly;
        
        char buf[65536];
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

        unsigned int min_chunk_size = 4096; // fs block size
        unsigned int avg_chunk_size = 8192; // zhu paper
        unsigned int max_chunk_size = 57344; // venti max
        unsigned int window_size = 48; // from sdfs
        
        poly = rabin_init(window_size, avg_chunk_size,
          min_chunk_size, max_chunk_size);

        if (!poly) {
          sysfatal("could not initialize rabin poly");
        }

        output_scores = (chunkdesc_t *)malloc(1024*sizeof(chunkdesc_t));
        if (!output_scores) {
          sysfatal("could not initialize output scores buffer");
        }
        unsigned int scoreptr = 0;

        if (argv[0][0] == '-' && argv[0][1] == 0) {
          scorefd = 1;
        } else {
          scorefd = create(argv[0], OWRITE, 0600);
          if (scorefd < 0) {
            sysfatal("could not open score file: %r");
          }
        }

	z = vtdial(host);
	if(z == nil)
		sysfatal("could not connect to server: %r");
	if(vtconnect(z) < 0)
		sysfatal("vtconnect: %r");

	p = vtmallocz(VtMaxLumpSize+1);

        int len, segment_len = 0;
        int new_segment = 0;

        char *buftowrite = (char *)&p[0];
        while ((nbytes = read(0, buf, sizeof(buf))) > 0) {
          char *buftoread = (char *)&buf[0];
          while ((len = rabin_segment_next(poly, buftoread, nbytes, &new_segment)) > 0) {
          memcpy(buftowrite, buftoread, len);
          segment_len += len;
          buftoread += len;
          buftowrite += len;
          nbytes -= len;


          if (new_segment) {
            chunkdesc_t* loc = &output_scores[scoreptr];
            loc->blocksize = segment_len;
            output(type, p, segment_len, loc->score);
            buftowrite = (char *)&p[0];
            scoreptr++;
            if (scoreptr == 1024) {
              dump_scores(output_scores, scoreptr);
              scoreptr = 0;
            }
            segment_len = 0;
          }

          if (!nbytes) {
            break;
          }
        }
        if (len == -1) {
          sysfatal("failed to process the segment\n");
        }
      }

      chunkdesc_t* loc = &output_scores[scoreptr];
      loc->blocksize = segment_len;
      output(type, p, segment_len, loc->score);
      buftowrite = (char *)&p[0];
      scoreptr++;
      dump_scores(output_scores, scoreptr);
      scoreptr = 0;
      if (close(scorefd)) {
        sysfatal("problems closing score file");
      }


        free(output_scores);
        rabin_free(&poly);
	vthangup(z);
	threadexitsall(0);
}
