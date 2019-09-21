/*
    chunkstats - translate a chunk file from binary to ascii

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
#include <stdio.h>
#include <unistd.h>

int main() {
  unsigned char scorebuf[25];
  unsigned int size;
  scorebuf[24] = 0;

  unsigned int nbytes;
  unsigned char* scoreptr;
  unsigned char* scorebufinptr = scorebuf;
  unsigned char* scorebufoutptr = scorebuf;
  while ((nbytes = read(0, scorebufinptr, 24 - (scorebufinptr - scorebuf))) > 0) {
    scorebufinptr += nbytes;
    while (scorebufoutptr + 24 <= scorebufinptr) {
      scoreptr = scorebufoutptr;
      size = scoreptr[0];
      size += scoreptr[1] << 8;
      size += scoreptr[2] << 16;
      size += scoreptr[3] << 24;

      printf("%d ", size);
      for (int foo = 4; foo < 24; foo++) {
        printf("%2.2x", scoreptr[foo]);
      }
      printf("\n");

      scorebufoutptr += 24;
    }

    if (scorebufinptr == scorebufoutptr) {
      scorebufinptr = scorebufoutptr = scorebuf;
    }
  }

  return 0;
}
