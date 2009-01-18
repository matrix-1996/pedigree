/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef FATFILESYSTEM_H
#define FATFILESYSTEM_H

#include <vfs/Filesystem.h>
#include <utilities/List.h>

// FAT Attributes
#define ATTR_READONLY   0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

/** This class provides an implementation of the FAT filesystem. */
class FatFilesystem : public Filesystem
{
public:
  FatFilesystem();

  virtual ~FatFilesystem();
  
    
  /** FAT type */
  enum FatType
  {
    FAT12 = 0, FAT16, FAT32
  };


  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk);
  static Filesystem *probe(Disk *pDisk);
  virtual File getRoot();
  virtual String getVolumeLabel();
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual void truncate(File *pFile);
  virtual void fileAttributeChanged(File *pFile);
  virtual File getDirectoryChild(File *pFile, size_t n);

protected:

  virtual bool createFile(File parent, String filename);
  virtual bool createDirectory(File parent, String filename);
  virtual bool createSymlink(File parent, String filename, String value);
  virtual bool remove(File parent, File file);

  /** The Fat 'superblock', or boot parameter block as we call it normally
      Note that this is from the FAT whitepaper, so the names are as specified
        there (in order to avoid confusion)
      Note also that Superblock32 and Superblock16 could probably go in Superblock
        as a union (perhaps) */
  struct Superblock
  {
    uint8_t   BS_jmpBoot[3];
    uint8_t   BS_OEMName[8];
    uint16_t  BPB_BytsPerSec;
    uint8_t   BPB_SecPerClus;
    uint16_t  BPB_RsvdSecCnt;
    uint8_t   BPB_NumFATs;
    uint16_t  BPB_RootEntCnt;
    uint16_t  BPB_TotSec16;
    uint8_t	  BPB_Media;
    uint16_t  BPB_FATSz16;
    uint16_t  BPB_SecPerTrk;
    uint16_t  BPB_NumHeads;
    uint32_t  BPB_HiddSec;
    uint32_t  BPB_TotSec32;
  } __attribute__((packed));
  
  /** Special FAT BPB starting after the Superblock (FAT32) */
  struct Superblock32
  {
    uint32_t  BPB_FATSz32;
    uint16_t  BPB_ExtFlags;
    uint16_t  BPB_FsVer;
    uint32_t  BPB_RootClus;
    uint16_t  BPB_FsInfo;
    uint16_t  BPB_BkBootSec;
    uint8_t   BPB_Reserved[12];
    uint8_t   BS_DrvNum;
    uint8_t   BS_Reserved1;
    uint8_t   BS_BootSig;
    uint32_t  BS_VolID;
    uint8_t   BS_VolLab[11];
    uint8_t   BS_FilSysType[8];
  } __attribute__((packed));
  
  /** Special FAT BPB starting after the Superblock (FAT16/12) */
  struct Superblock16
  {
    uint8_t   BS_DrvNum;
    uint8_t   BS_Reserved1;
    uint8_t   BS_BootSig;
    uint32_t  BS_VolID;
    int8_t    BS_VolLab[11];
    int8_t    BS_FilSysType[8];
  } __attribute__((packed));

  /** An Fat directory entry. */
  struct Dir
  {
    uint8_t   DIR_Name[11];
    uint8_t   DIR_Attr;
    uint8_t   DIR_NTRes;
    uint8_t   DIR_CrtTimeTenth;
    uint16_t  DIR_CrtTime;
    uint16_t  DIR_CrtDate;
    uint16_t	DIR_LstAccDate;
    uint16_t	DIR_FstClusHI;
    uint16_t	DIR_WrtTime;
    uint16_t	DIR_WrtDate;
    uint16_t	DIR_FstClusLO;
    uint32_t	DIR_FileSize;
  } __attribute__((packed));

  FatFilesystem(const FatFilesystem&);
  void operator =(const FatFilesystem&);
  
  /** Reads a cluster from the disk. */
  bool readCluster(uint32_t block, uintptr_t buffer);
  
  /** Reads a block starting from a specific sector from the disk. */
  bool readSectorBlock(uint32_t sec, size_t size, uintptr_t buffer);
  
  /** Obtains the first sector given a cluster number */
  uint32_t getSectorNumber(uint32_t cluster);
  
  /** Grabs a cluster entry */
  uint32_t getClusterEntry(uint32_t cluster);
  
  /** Converts a string to 8.3 format */
  String convertFilenameTo(String filename);
  
  /** Converts a string from 8.3 format */
  String convertFilenameFrom(String filename);
  
  /** Is a given cluster *VALUE* EOF? */
  bool isEof(uint32_t cluster);
  
  /** Our raw device. */
  Disk *m_pDisk;

  /** Our superblocks */
  Superblock m_Superblock;
  Superblock16 m_Superblock16;
  Superblock32 m_Superblock32;
  
  /** Type of the FAT */
  FatType m_Type;
  
  /** Required information */
  uint64_t m_DataAreaStart; // data area can potentially start above 4 GB
  uint32_t m_RootDirCount;
  
  /** Root directory information */
  union RootDirInfo
  {
    uint32_t sector; // FAT12 and 16 don't use a cluster
    uint32_t cluster; // but FAT32 does...
  } m_RootDir;

  /** Group descriptors. */
//  GroupDesc *m_pGroupDescriptors;

  /** Size of a block (in this case, a cluster) */
  uint32_t m_BlockSize;
};

#endif
