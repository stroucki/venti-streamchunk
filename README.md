venti-streamchunk
=================

(c) 2019 by Michael Stroucken <mxs@cmu.edu>

This program was inspired by the 18-746 Storage Systems
class at CMU, which currently has for its second lab a variable chunked
deduplication FUSE filesystem called CloudFS, as well as the venti
storage system developed as part of Plan9.

## Introduction to venti

Venti is a write-once read-many object storage system, where the keys
are the SHA1 hash of the value (data) itself. It was developed as part
of the Plan9 operating system. A port to unix system exists.

Plan9's venti, even though it has several bad design decisions,
has proven itself in my environment to be a safe, archival, WORM
storage system.

The included primary backup tools in venti are vbackup and vac. vac
can back up files and directories, then expose them again in a
FUSE filesystem. However, metadata such as permissions are not available
to a degree that would permit a full restoration of state.

vbackup
reads raw filesystem data and can recreate an equivalent filesystem on
restore, or expose it via NFS. However, it only supports a limited
selection of filesystems. XFS and ext4 are not supported, for example.

The venti tools all presuppose that data is sent in full fixed block
increments, but internally venti stores variable blocks if a block
ends with a series of NULs. Venti calls this "zero truncation".

It is possible to send individual blocks to venti, of variable sizes
up to a maximum of 57 KB.

venti is based on fixed size blocking, so adding a byte to the beginning 
of a file will essentially fail to exploit any similarity between the 
two versions.

## CloudFS

CloudFS is a teaching tool used to demonstrate the utility of caching, 
content-addressable storage, compression and variable length 
segmentation in filesystems.

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

As venti allows the storage of variable blocks, this stream can be 
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
include files, being available.

To compile, run `make`. If your plan9port installation is not in the default location, you can pass the location to make as follows:
```
VENTIINC=~/git/plan9port/include/ VENTILIB=~/git/plan9port/lib/ make
```

## Usage

Set up your environment like you would for any venti client, then to store:-
```
cat | streamchunkwrite /tmp/scores
``
To read:-
```
streamchunkread /tmp/scores|cat
```

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

On 2012-05-02, Hugo Patterson came to talk at CMU. He said that Data 
Domain had chosen a minimum block size of 2K and a maximum of 64K.

On 2016-11-28, he mentioned that a doubling of the chunk size results, 
as a rule of thumb, into 15% extra storage usage.

## Evaluation

In short: It works, the venti protocol is slow.
(more to come)

## References
(to come)
