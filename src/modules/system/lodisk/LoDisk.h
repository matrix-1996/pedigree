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
#ifndef LODISK_H
#define LODISK_H

#include <processor/types.h>
#include <utilities/String.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <machine/Disk.h>
#include <utilities/Cache.h>

/// 4 KB is enough to fit 8 HD sectors and 2 CD sectors, which makes it a good
/// choice for the page cache size.
#define FILEDISK_PAGE_SIZE      4096

/** Abstraction for Files as Disks
  * This allows disk images to be opened as a real disk for installer ramdisks
  * or for testing.
  *
  * Specifying RamOnly causes the file to be stored only in RAM with no changes
  * committed to the file.
  */
class FileDisk : Disk
{
    public:

        enum AccessType
        {
            /** Read from file into RAM, changes persist in
              * RAM, never written to file
              */
            RamOnly = 0,

            /** Read from file, changes go to file */
            Standard
        };

        FileDisk() :
            m_pFile(0), m_Mode(Standard), m_PageCache()
        {
            FATAL("I require a constructor with arguments!");
        }

        FileDisk(String file, AccessType mode = Standard);
        virtual ~FileDisk();

        virtual void getName(String &str)
        {
            if(m_pFile)
                str = m_pFile->getName();
            else
                str = String("FileDisk");
        }

        bool initialise();

        virtual uint64_t read(uint64_t location, uint64_t nBytes, uintptr_t buffer);
        virtual uint64_t write(uint64_t location, uint64_t nBytes, uintptr_t buffer);

    private:

        FileDisk(const FileDisk&);
        FileDisk& operator = (const FileDisk&);

        /// File we're using as a disk
        File *m_pFile;

        /// Access mode
        AccessType m_Mode;

        /** Pages that have been read.
          * This code works on 4 KB pages in order to reduce
          * disk access. This should provide a performance
          * increase at a minimal expense as most reads are
          * contiguous over at least a kilobyte of data.
          *
          * Always touched on writes, Standard access modes
          * write here and then write the page to disk.
          *
          * \todo Maintain a dirty page list for Standard
          *       modes, which a worker thread handles
          *       in order to improve performance.
          */
        Tree<size_t, void*> m_PageCache;
};

#endif
