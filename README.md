# WIP: fsfs-emulator

## DoD
- **F**lash **S**imple **F**ile **S**ystem
  - [X] based on chunks of memory that can be selected ~~at the compile time~~
    - [X] chunk size 1024kb
    - [X] chunk size 2048kb
    - [X] chunk size 4096kb
  - [ ] C++ style interface
    - [X] `read` - reads selected lenght of data from given inode with respect to given offset
    - [X] `write` - writes to inode with selected lenght of data with respect to offset, also allocate another inodes if necessary
    - [ ] `stats` - print all reports of the file system 
    - [X] `format` - formats the disk and initialzie propper file system structure
    - [X] `mount` - checks the integrity of the file system
    - [X] `unmount` - free up the disk
    - [X] `create` - create new inode
    - [X] `remove` - mark the inode as not allocated to be overwritten in the future or unlink data block
  - [ ] memory cell wear problem optimization
- Disk space emulator
  - [X] based on chunks of memory that can be selected ~~at the compile time~~
    - [X] chunk size 1024kb
    - [X] chunk size 2048kb
    - [X] chunk size 4096kb
  - [X] C++ style interface
    - [X] `open` - opens the file that contains an image of the disk space
    - [X] `read` - perform a read operation on disk by selected size of chunk
    - [X] `write` - perform a write operation on disk by selected size of chunk
    - [X] `size` - returns the ammonts of blocks in the disk
    - [X] `mount` - sets the disk as mounted
    - [X] `unmount` - sets the disk as unmounted
- CLI emulator
  - [ ] displays all action that the file system can perfom
  - [ ] pack files into the emulated disk
  - [ ] unpack files from emulated disk
  - [ ] displays found virtual disks
  - [ ] displays stats of the disk and file system
- Tests
  - [X] Disk space emulator
    - [X] `create`
    - [X] `open`
    - [X] `write`
    - [X] `mount`
    - [X] size getters
  - [X] File system
    - [X] `read`
    - [X] `write`
    - [ ] `stats`
    - [X] `format`
    - [X] `mount`
    - [X] `unmount`
    - [X] `create`
    - [X] `remove`


## Sources - educational/inspiration
1. https://www3.nd.edu/~pbui/teaching/cse.30341.fa17/project06.html
2. https://sourceware.org/jffs2/jffs2.pdf
3. https://www.longdom.org/open-access/nand-flash-memory-organization-and-operations-2165-7866-1000139.pdf
4. https://sites.cs.ucsb.edu/~rich/class/cs270/projects/project-hints.html
5. https://www.cs.uic.edu/~jbell/CourseNotes/OperatingSystems/12_FileSystemImplementation.html
6. http://martin.hinner.info/fs/bfs/bfs-structure.html#compaction
7. https://web.cs.wpi.edu/~rek/DCS/D04/UnixFileSystems.html