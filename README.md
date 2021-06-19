# fsfs-emulator

## DoD
- **S**imple **F**ile **S**ystem
  - [ ] - based on chunks of memory that can be selected at the compile time 
    - [ ] - chunk size 1024kb
    - [ ] - chunk size 4096kb
  - [ ] - linux C style interface
    - [ ] - read - reads selected lenght of data from given inode with respect to given offset
    - [ ] - write - writes to inode with selected lenght of data with respect to offset, also allocate another inodes if necessary
    - [ ] - stats - print all reports of the file system 
    - [ ] - format - formats the disk and initialzie propper file system structure
    - [ ] - mount - checks the integrity of the file system
    - [ ] - unmount - free up the disk
    - [ ] - create - create new inode
    - [ ] - remove - mark the inode as not allocated to be overwritten in future
    - [ ] - remove_hard - not only mark the inode as free but also overwrite the data
    - [ ] - remove_block - remove block from the inode (shirinking files)
  - [ ] - memory cell wear problem optimization
- Disk space emulator
  - [ ] - based on chunks of memory that can be selected at the compile time 
    - [ ] - chunk size 1024kb
    - [ ] - chunk size 4096kb
  - [ ] C++ style interface
    - [ ] - open - opens the file that contains an image of the disk space
    - [ ] - read - perform a read operation on disk by selected size of chunk
    - [ ] - write - perform a write operation on disk by selected size of chunk
    - [ ] - size - returns the ammonts of blocks in the disk
    - [ ] - mount - sets the disk as mounted
    - [ ] - unmount - sets the disk as unmounted
- CLI emulator
  - [ ] - displays all action that the file system can perfom
  - [ ] - pack files into the emulated disk
  - [ ] - unpack files from emulated disk
  - [ ] - displays found virtual disks
  - [ ] - displays stats of the disk and file system
- Tests
  - [ ] - Disk space emulator
    - [ ] - creating virtual disk
    - [ ] - opening and mounting virtual disk
    - [ ] - writting and reading to the virtual disk
    - [ ] - returns correct size value

## Sources 
1. https://www3.nd.edu/~pbui/teaching/cse.30341.fa17/project06.html
2. https://sourceware.org/jffs2/jffs2.pdf