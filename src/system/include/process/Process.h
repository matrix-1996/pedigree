/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef PROCESS_H
#define PROCESS_H

#ifdef THREADS

#include <compiler.h>
#include <process/Thread.h>
#include <processor/state.h>
#include <utilities/Vector.h>
#include <utilities/StaticString.h>
#include <utilities/String.h>
#include <Atomic.h>
#include <Spinlock.h>
#include <LockGuard.h>
#include <process/Semaphore.h>
#include <process/Mutex.h>
#include <utilities/Tree.h>
#include <utilities/MemoryAllocator.h>
#include <time/Time.h>

#include <Subsystem.h>

class VirtualAddressSpace;
class File;
class FileDescriptor;
class User;
class Group;
class DynamicLinker;

/**
 * An abstraction of a Process - a container for one or more threads all running in
 * the same address space.
 */
class Process
{
    friend class Thread;
public:
    /** Subsystems may inherit Process to provide custom functionality. However, they need
     *  to know whether a Process pointer is subsystem-specific. This enumeration is designed
     *  to allow functions using Process objects in subsystems with inherited Process objects
     *  to be able to figure out what type the Process is without depending on any external
     *  accounting.
     */
    enum ProcessType
    {
        Stock,
        Posix
    };

    /**
     * Processes have their own state, aside from the state of their threads.
     * These states are very general and don't reflect the current scheduling
     * state of the process as a whole in intricate detail.
     */
    enum ProcessState
    {
        Active,
        Suspended,
        Terminating,
        Terminated,
        Reaped, /// Reaped means the process has had a status retrieved.
    };

    /** Default constructor. */
    Process();

    /** Constructor for creating a new Process. Creates a new Process as
     * a UNIX fork() would, from the given parent process. This constructor
     * does not create any threads.
     * \param pParent The parent process. */
    Process(Process *pParent);

    /** Destructor. */
    virtual ~Process();

    /** Adds a thread to this process.
     *  \return The thread ID to be assigned to the new Thread. */
    size_t addThread(Thread *pThread);
    /** Removes a thread from this process. */
    void removeThread(Thread *pThread);

    /** Returns the number of threads in this process. */
    size_t getNumThreads();
    /** Returns the n'th thread in this process. */
    Thread *getThread(size_t n);

    /** Returns the process ID. */
    size_t getId()
    {
        return m_Id;
    }

    /** Returns the description string of this process. */
    LargeStaticString &description()
    {
        return str;
    }

    /** Returns our address space */
    VirtualAddressSpace *getAddressSpace()
    {
        return m_pAddressSpace;
    }

    /** Sets the exit status of the process. */
    void setExitStatus(int code)
    {
        m_ExitStatus = code;
    }
    /** Gets the exit status of the process. */
    int getExitStatus()
    {
        return m_ExitStatus;
    }

    /** Marks the process as reaped. */
    void reap()
    {
        m_State = Reaped;
    }

    /** Kills the process. */
    void kill() NORETURN;
    /** Suspends the process. */
    void suspend();
    /** Resumes the process from suspend. */
    void resume();

    /** Returns the parent process. */
    Process *getParent()
    {
        return m_pParent;
    }

    /** Returns the current working directory. */
    File *getCwd()
    {
        return m_Cwd;
    }
    /** Sets the current working directory. */
    void setCwd(File *f)
    {
        m_Cwd = f;
    }

    /** Returns the current controlling terminal. */
    File *getCtty()
    {
        return m_Ctty;
    }
    /** Sets the controlling terminal. */
    void setCtty(File *f)
    {
        m_Ctty = f;
    }


    /** Returns the memory space allocator for primary address space. */
    MemoryAllocator &getSpaceAllocator()
    {
        return m_SpaceAllocator;
    }
    /** Returns the memory space allocator for dynamic address space. */
    MemoryAllocator &getDynamicSpaceAllocator()
    {
        return m_DynamicSpaceAllocator;
    }

    /** Gets the current user. */
    User *getUser()
    {
        return m_pUser;
    }
    /** Sets the current user. */
    void setUser(User *pUser)
    {
        m_pUser = pUser;
    }
    
    /** Gets the effective user. */
    User *getEffectiveUser()
    {
        return m_pEffectiveUser;
    }
    /** Sets the effective user. */
    void setEffectiveUser(User *pUser)
    {
        m_pEffectiveUser = pUser;
    }

    /** Gets the current group. */
    Group *getGroup()
    {
        return m_pGroup;
    }
    /** Sets the current group. */
    void setGroup(Group *pGroup)
    {
        m_pGroup = pGroup;
    }
    
    /** Gets the current effective group. */
    Group *getEffectiveGroup()
    {
        return m_pEffectiveGroup;
    }
    void setEffectiveGroup(Group *pGroup)
    {
        m_pEffectiveGroup = pGroup;
    }

    void setLinker(DynamicLinker *pDl)
    {
        m_pDynamicLinker = pDl;
    }
    DynamicLinker *getLinker()
    {
        return m_pDynamicLinker;
    }

    void setSubsystem(Subsystem *pSubsystem)
    {
        m_pSubsystem = pSubsystem;
    }
    Subsystem *getSubsystem()
    {
        return m_pSubsystem;
    }

    /** Gets the type of the Process (subsystems may override) */
    virtual ProcessType getType()
    {
        return Stock;
    }

    void addWaiter(Semaphore *pWaiter);
    void removeWaiter(Semaphore *pWaiter);
    size_t waiterCount() const;

    bool hasSuspended()
    {
        bool bRet = m_bUnreportedSuspend;
        m_bUnreportedSuspend = false;
        return bRet;
    }
    bool hasResumed()
    {
        bool bRet = m_bUnreportedResume;
        m_bUnreportedResume = false;
        return bRet;
    }

    ProcessState getState() const
    {
        return m_State;
    }

    void markTerminating()
    {
        m_State = Terminating;
    }

    void trackPages(ssize_t nVirtual, ssize_t nPhysical, ssize_t nShared)
    {
        m_Metadata.virtualPages += nVirtual;
        m_Metadata.physicalPages += nPhysical;
        m_Metadata.sharedPages += nShared;
    }

    void resetCounts()
    {
        m_Metadata.virtualPages = 0;
        m_Metadata.physicalPages = 0;
        m_Metadata.sharedPages = 0;
        m_Metadata.startTime = Time::getTimeNanoseconds();
    }

    /**
     * Record the current time in the relevant field for this process.
     *
     * Use to set the point in time from which the next difference will be
     * taken.
     */
    void recordTime(bool bUserspace)
    {
        Time::Timestamp now = Time::getTimeNanoseconds();
        if (bUserspace)
        {
            m_LastUserspaceEntry = now;
        }
        else
        {
            m_LastKernelEntry = now;
        }
    }

    /**
     * Counts the time spent since the last recordTime(), and then updates the
     * relevant time field to the current time.
     *
     * Use when scheduling.
     */
    void trackTime(bool bUserspace)
    {
        Time::Timestamp now = Time::getTimeNanoseconds();
        if (bUserspace)
        {
            Time::Timestamp diff = now - m_LastUserspaceEntry;
            m_LastUserspaceEntry = now;
            m_Metadata.userTime += diff;
        }
        else
        {
            Time::Timestamp diff = now - m_LastKernelEntry;
            m_LastKernelEntry = now;
            m_Metadata.kernelTime += diff;
        }
    }

    /** Gets timestamps. */
    Time::Timestamp getUserTime() const
    {
        return m_Metadata.userTime;
    }
    Time::Timestamp getKernelTime() const
    {
        return m_Metadata.kernelTime;
    }
    Time::Timestamp getStartTime() const
    {
        return m_Metadata.startTime;
    }

    /** Get process usage. */
    ssize_t getVirtualPageCount() const
    {
        return m_Metadata.virtualPages;
    }
    ssize_t getPhysicalPageCount() const
    {
        return m_Metadata.physicalPages;
    }
    ssize_t getSharedPageCount() const
    {
        return m_Metadata.sharedPages;
    }

    /** Set this process' root. */
    void setRootFile(File *pFile)
    {
        m_pRootFile = pFile;
    }

    /** Get this process' root. */
    File *getRootFile() const
    {
        return m_pRootFile;
    }

    /**
     * Get the init process (first userspace process, parent of all
     * userspace processes).
     */
    static Process *getInit();

    /** Set the init process. */
    static void setInit(Process *pProcess);

private:
    Process(const Process &);
    Process &operator = (const Process &);

    /**
     * Our list of threads.
     */
    Vector<Thread*> m_Threads;
    /**
     * The next available thread ID.
     */
    Atomic<size_t> m_NextTid;
    /**
     * Our Process ID.
     */
    size_t m_Id;
    /**
     * Our description string.
     */
    LargeStaticString str;
    /**
     * Our parent process.
     */
    Process *m_pParent;
    /**
     * Our virtual address space.
     */
    VirtualAddressSpace *m_pAddressSpace;
    /**
     * Process exit status.
     */
    int m_ExitStatus;
    /**
     * Current working directory.
     */
    File *m_Cwd;
    /**
     * Current controlling terminal.
     */
    File *m_Ctty;
    /**
     * Memory allocator for primary address space.
     */
    MemoryAllocator m_SpaceAllocator;
    /**
     * Memory allocator for dynamic address space, if any.
     */
    MemoryAllocator m_DynamicSpaceAllocator;
    /** Current user. */
    User *m_pUser;
    /** Current group. */
    Group *m_pGroup;
    /** Effective user. */
    User *m_pEffectiveUser;
    /** Effective group. */
    Group *m_pEffectiveGroup;

    /** The Process' dynamic linker. */
    DynamicLinker *m_pDynamicLinker;

    /** The subsystem for this process */
    Subsystem *m_pSubsystem;

    /** Semaphores to release whenever we are killed, suspended, or resumed. */
    List<Semaphore *> m_Waiters;

    /** Whether we have suspended but not reported it. */
    bool m_bUnreportedSuspend;

    /** Whether we have resumed but not reported it. */
    bool m_bUnreportedResume;

    /** Our current state. */
    ProcessState m_State;

    /**
     * State we were in before suspend. Ensures if we were sleeping before,
     * we still will be after a resume.
     */
    Thread::Status m_BeforeSuspendState;

    /** Concurrency lock for complex Process data structures. */
    Spinlock m_Lock;

    /** Releases all locks in m_Waiters once. */
    void notifyWaiters();

    /** Stores metadata about this process. */
    struct ProcessMetadata
    {
        ProcessMetadata() :
            virtualPages(0), physicalPages(0), sharedPages(0), userTime(0),
            kernelTime(0), startTime(0)
        {}

        /// Virtual address space consumed, including that which would trigger
        /// a successful trap to page data in.
        ssize_t virtualPages;
        /// Physical address space consumed, barring that which is shared.
        ssize_t physicalPages;
        /// Shared pages consumed.
        ssize_t sharedPages;

        /// Time spent in userspace as this process.
        Time::Timestamp userTime;
        /// Time spent in the kernel as this process.
        Time::Timestamp kernelTime;

        /// Time at which process started.
        Time::Timestamp startTime;
    } m_Metadata;

    /** Last time we entered the kernel. */
    Time::Timestamp m_LastKernelEntry;

    /** Last time we entered userspace. */
    Time::Timestamp m_LastUserspaceEntry;

    /** Root directory for this process. NULL == system-wide default. */
    File *m_pRootFile;

    /** Init process (terminated processes' children will reparent to this). */
    static Process *m_pInitProcess;

public:
    Semaphore m_DeadThreads;
};

#endif

#endif
