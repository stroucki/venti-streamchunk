venti-streamchunk
=================

(c) 2019 by Michael Stroucken <mxs@cmu.edu>

This program was inspired by the 18-746 Storage Systems
class at CMU, which currently has for its second lab a variable chunked
deduplication FUSE filesystem called CloudFS, as well as the venti
storage system developed as part of Plan9.

## Introduction to venti

Venti is a write-once read-many key-value storage system, where the keys 
are the SHA1 hash of the value (data) itself. It was developed as part 
of the Plan9 operating system. A port to Unix systems of Plan9 software 
exists by the name of Plan9 from User Space, or plan9port.

Plan9's venti, even though it has several bad design decisions,
has proven itself in my environment to be a safe, archival, WORM
storage system.

The included primary backup tools in venti are vbackup and vac. vac
can back up files and directories, then expose them again in a
FUSE filesystem. However, metadata such as permissions are not available
to a degree that would permit a full restoration of state.

vbackup reads raw filesystem data and can recreate an equivalent 
filesystem on restore, or expose it via NFS. However, it only supports a 
limited selection of filesystems. xfs and ext4 are not supported, for 
example.

The venti tools all presuppose that data is sent in full fixed block 
increments, but internally venti stores, by default, blocks stripped of 
any trailing NULs. Venti calls this "zero truncation".

It is possible to send individual blocks to venti, of variable sizes up 
to a maximum of 57 KB (due to odd internal formatting to try to save a 
few bytes).

The venti tools are based on using fixed size blocking, so adding a byte 
to the beginning of a file will essentially fail to exploit any 
similarity between the two versions. Similarly, any third party archive
tools are unlikely to produce output that meets the requirements to
obtain optimal space efficiency.

## Introduction of CloudFS

CloudFS is a teaching tool used in the graduate storage systems class 
(18-746) to demonstrate the utility of caching, content-addressable 
storage, compression and variable length segmentation in filesystems.

In its full development, the CloudFS project is a FUSE filesystem that 
splits data into variable length chunks, compresses them, and will store 
the compressed chunks onto something that can speak the S3 protocol. 
Local storage is used to cache chunks. Retrieval and indexing is handled 
on the S3 server. CloudFS is supposed to handle the deletion of data.
 
An implementation of Rabin segmenting that was provided for the class 
project is included in this project.

## This contribution

The code in this project is based on the read/readfile and 
write/writefile samples in src/cmd/venti, along with the Rabin 
polynomial code provided in 18-746.

Since the variable block chunking mitigates the need to keep blocks
aligned to avoid storing multiple alignments of the same data, third
party tools can be used to create the stream of data to store.

As venti allows the storage of variable length blocks, this stream can be 
provided to venti-streamchunk, which will chunk the data and submit the 
resulting blocks to venti.

venti returns a SHA1 hash of the data submitted to it, and 
venti-streamchunk will then store that hash along with the block size
in a file.

This file can be stored and used to retrieve the data from venti. In 
that case, the blocks will be recalled from venti and assembled into a 
stream that can be further processed.

## Compiling

This project depends on a compiled copy of the plan9port, along with
include files, being available. Find the code at
[https://9fans.github.io/plan9port/].

To compile, run `make`. If your plan9port installation is not in the default location, you can pass the location to make as follows:
```
VENTIINC=~/git/plan9port/include/ VENTILIB=~/git/plan9port/lib/ make
```

## Usage

Set up your environment like you would for any venti client, then to store:-
```
cat | streamchunkwrite /tmp/scores
```
To read:-
```
streamchunkread /tmp/scores|cat
```

Of course `cat` can be any producer or consumer of data.

If you want to override the contents of a venti environment variable, 
you can provide one as an argument like `streamchunkwrite -h 
'tcp!localhost!17034' /tmp/scores`.

## Technical detail

The list of hashes and sizes is currently a sequence of { block size 
(little endian 4 bytes), SHA1 hash (20 bytes) }.

The chunking parameters chosen were
  * min_chunk_size = 4096 bytes (chosen because of filesystem block size)
  * avg_chunk_size = 8192 bytes (based on Zhu paper)
  * max_chunk_size = 57344 bytes (venti maximum)
  * window_size = 48 bytes (chosen from SDFS)

This software relies on venti to store only a single instance of the 
same data. It does not try to avoid sending duplicate data. This allows 
the omission of a client-side hashing of the data.

On 2012-05-02, Hugo Patterson came to talk at CMU. He said that Data 
Domain had chosen a minimum block size of 2K and a maximum of 64K.

On 2016-11-28, he mentioned that a doubling of the chunk size results, 
as a rule of thumb, in 15% extra storage usage.

## Evaluation

### The concept

A tar file of my home directory of size 93 GB was segmented with the 
default parameters. The sum of the unique set of segments was 72 GB (77% 
of original). The most duplicated segments were the following:

```
 489 54428 hash-a
 510 4104 hash-b
 540 5659 hash-c
 592 4179 hash-d
 648 5609 hash-e
 745 4179 hash-f
1256 4179 hash-g
2342 57344 hash-h
4331 4179 hash-i
1721733 4096 hash-0
```

Table shows repetition count, block size and hash. Note that by far 
blocks of all NULs are the most common, but in general there isn't all 
that much repetition. I do try to avoid having multiple copies of data 
spread around my directory. But we hope that it means that subsequent 
backups *will* reuse most of the segments.

I then took a tar file of a previous backup of my home directory and 
processed it the same way. Original size was 146 GB. Total size of 
unique segments was 129 GB (87%). Total number of unique chunks was 12 
million.

Sorting by (repetition * block size) ascending, in steps of one million 
segments, the amount of the dataset covered is:-

```
3%, 6%, 9%, 13%, 18%, 23%, 30%, 37%, 46%, 57%, 71%, 94%
```

Less than 727000 blocks (6%) were duplicate, but they made up 18% of the 
original data. Within a backup instance, savings from segment 
deduplication are likely to be small but still significant. A system 
like venti will only keep one instance of each segment in storage by 
design. Due to the low rate of duplicate segments, it does not seem 
fruitful to provide a cache to prevent duplicate writes (which would 
have no effect anyway). But looking at the list of most duplicated 
segments in this dataset:-

```
 489 54428 hash-a
 510 4104 hash-b
 540 5659 hash-c
 592 4179 hash-d
 648 5609 hash-e
 745 4179 hash-f
1256 4179 hash-g
2342 57344 hash-h
4331 4179 hash-i
1326485 4096 hash-0
```

We see that the only difference is the lesser amount of NUL blocks, and 
that otherwise the data that was in the other dataset is still here.

Using afio, using a different archive format than tar, resulted in a 
size of unique segments of 128 GB. Looking at the resultant list of most 
deduplicated segments here:-

```
 489 54428 hash-a
 510 4104 hash-b
 540 5659 hash-c
 592 4179 hash-d
 648 5609 hash-e
 745 4179 hash-f
1256 4179 hash-g
2342 57344 hash-h
4331 4179 hash-i
1326412 4096 hash-0
```

As can be seen, the variable chunking makes the data essentially 
format-agnostic. The size of extra storage incurred for using a 
different archiver on the same data amounted to 4 GB, or 2.9%.

### The system
In short: It works, the venti protocol is slow.
(more to come)

### Alternatives

I explored [Restic](https://restic.net/), positive points were that it 
had a lot of internal parallelism due to being written in Go. Negative 
points were no support for compression, insistence on using its own 
encryption, requirement for cache space in the home directory, and high 
load when using a local data store.

I also explored [Borg](https://www.borgbackup.org/), positive points 
were a slightly more efficient data store than Restic. Negative points 
were the use of python and the lack of parallelism it entails, glacial 
speed when using LZMA compression.

[Venti](https://9fans.github.io/plan9port/) has been my workhorse backup 
system for years. Positive aspects are the inability to delete data, 
straightforward storage formats. Negative aspects are the slow protocol, 
which does not permit transaction batching, limited support for modern 
Unix filesystems, inability to delete data, odd implementation choices, 
fixed compression scheme, fixed block sizes using default tools, and 
high system load.

All the prior systems show bad performance, generally on the order of 
~15 MB/s throughput due to index lookup bottlenecks as a consequence of 
minimal cache locality. The Zhu paper talks about Locality Preserved 
Caching to drive the Data Domain storage system throughput over 100 
MB/s.

Years prior, I investigated [bup](https://bup.github.io/). It tried to 
coerce the git source code repository backend to store data. Git tries 
hard to minimize the data stored, which is good for source code, but 
when faced with a diverse data set of photographs, movies, disk images 
and source code, as the amount of data stored grew over 1 TB, 
performance collapsed to unusability.

## References

Benjamin Zhu, Kai Li, Hugo Patterson: "Avoiding the Disk Bottleneck in 
the Data Domain Deduplication File System", 6th USENIX Conference on 
File and Storage Technologies, Feb 26-29, 2008, San Jose, CA

Sean Quinlan, Sean Dorward: "Venti: a new approach to archival storage", 
Proceedings of the USENIX Conference on File And Storage Technologies, 2 
(2002), pp. 89-101

Michael O. Rabin: "Fingerprinting by random polynomials", Technical 
Report TR-15-81, Center for Research in Computing Technology, Harvard 
University, 1981
