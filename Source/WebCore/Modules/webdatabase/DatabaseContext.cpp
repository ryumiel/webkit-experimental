/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "DatabaseContext.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Database.h"
#include "DatabaseManager.h"
#include "DatabaseTask.h"
#include "DatabaseThread.h"
#include "Document.h"
#include "Page.h"
#include "SchemeRegistry.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "Settings.h"

namespace WebCore {

// How the DatabaseContext Life-Cycle works?
// ========================================
// ... in other words, who's keeping the DatabaseContext alive and how long does
// it need to stay alive?
//
// The DatabaseContext is referenced from RefPtrs in:
// 1. ScriptExecutionContext
// 2. Database
//
// At Birth:
// ========
// We create a DatabaseContext only when there is a need i.e. the script tries to
// open a Database via DatabaseManager::openDatabase().
//
// The DatabaseContext constructor will call setDatabaseContext() on the
// the ScriptExecutionContext. This sets the RefPtr in the ScriptExecutionContext
// for keeping the DatabaseContext alive. Since the DatabaseContext is only
// created from the script thread, it is safe for the constructor to call
// ScriptExecutionContext::setDatabaseContext().
//
// Once a DatabaseContext is associated with a ScriptExecutionContext, it will
// live until after the ScriptExecutionContext destructs. This is true even if
// we don't succeed in opening any Databases for that context. When we do
// succeed in opening Databases for this ScriptExecutionContext, the Database
// will re-use the same DatabaseContext.
//
// At Shutdown:
// ===========
// During shutdown, the DatabaseContext needs to:
// 1. "outlive" the ScriptExecutionContext.
//    - This is needed because the DatabaseContext needs to remove itself from the
//      ScriptExecutionContext's ActiveDOMObject list and ContextDestructionObserver
//      list. This removal needs to be executed on the script's thread. Hence, we
//      rely on the ScriptExecutionContext's shutdown process to call
//      stop() and contextDestroyed() to give us a chance to clean these up from
//      the script thread.
//
// 2. "outlive" the Databases.
//    - This is because they may make use of the DatabaseContext to execute a close
//      task and shutdown in an orderly manner. When the Databases are destructed,
//      they will deref the DatabaseContext from the DatabaseThread.
//
// During shutdown, the ScriptExecutionContext is shutting down on the script thread
// while the Databases are shutting down on the DatabaseThread. Hence, there can be
// a race condition as to whether the ScriptExecutionContext or the Databases
// destruct first.
//
// The RefPtrs in the Databases and ScriptExecutionContext will ensure that the
// DatabaseContext will outlive both regardless of which of the 2 destructs first.


DatabaseContext::DatabaseContext(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_hasOpenDatabases(false)
    , m_isRegistered(true) // will register on construction below.
    , m_hasRequestedTermination(false)
#if PLATFORM(IOS)
    , m_paused(false)
#endif
{
    // ActiveDOMObject expects this to be called to set internal flags.
    suspendIfNeeded();

    context->setDatabaseContext(this);

    // For debug accounting only. We must do this before we register the
    // instance. The assertions assume this.
    auto& databaseManager = DatabaseManager::singleton();
    databaseManager.didConstructDatabaseContext();

    databaseManager.registerDatabaseContext(this);
}

DatabaseContext::~DatabaseContext()
{
    stopDatabases();
    ASSERT(!m_databaseThread || m_databaseThread->terminationRequested());

    // For debug accounting only. We must call this last. The assertions assume
    // this.
    DatabaseManager::singleton().didDestructDatabaseContext();
}

// This is called if the associated ScriptExecutionContext is destroyed while
// we're still associated with it. That's our cue to disassociate and shutdown.
// To do this, we stop the database and let everything shut down naturally
// because the database closing process might still make use of this context.
// It is not safe to just delete the context here.
void DatabaseContext::contextDestroyed()
{
    stopDatabases();
    ActiveDOMObject::contextDestroyed();
}

// stop() is from stopActiveDOMObjects() which indicates that the owner Frame
// or WorkerThread is shutting down. Initiate the orderly shutdown by stopping
// the associated databases.
void DatabaseContext::stop()
{
    stopDatabases();
}

bool DatabaseContext::canSuspend() const
{
    if (!hasOpenDatabases() || !m_databaseThread)
        return true;

    return !m_databaseThread->hasPendingDatabaseActivity();
}

DatabaseThread* DatabaseContext::databaseThread()
{
    if (!m_databaseThread && !m_hasOpenDatabases) {
#if PLATFORM(IOS)
        MutexLocker lock(m_databaseThreadMutex);
#endif
        // It's OK to ask for the m_databaseThread after we've requested
        // termination because we're still using it to execute the closing
        // of the database. However, it is NOT OK to create a new thread
        // after we've requested termination.
        ASSERT(!m_hasRequestedTermination);

        // Create the database thread on first request - but not if at least one database was already opened,
        // because in that case we already had a database thread and terminated it and should not create another.
        m_databaseThread = DatabaseThread::create();
        if (!m_databaseThread->start())
            m_databaseThread = nullptr;

#if PLATFORM(IOS)
        if (m_databaseThread)
            m_databaseThread->setPaused(m_paused);
#endif
    }

    return m_databaseThread.get();
}

#if PLATFORM(IOS)
void DatabaseContext::setPaused(bool paused)
{
    MutexLocker lock(m_databaseThreadMutex);

    m_paused = paused;
    if (m_databaseThread)
        m_databaseThread->setPaused(m_paused);
}
#endif // PLATFORM(IOS)

bool DatabaseContext::stopDatabases(DatabaseTaskSynchronizer* synchronizer)
{
    if (m_isRegistered) {
        DatabaseManager::singleton().unregisterDatabaseContext(this);
        m_isRegistered = false;
    }

    // Though we initiate termination of the DatabaseThread here in
    // stopDatabases(), we can't clear the m_databaseThread ref till we get to
    // the destructor. This is because the Databases that are managed by
    // DatabaseThread still rely on this ref between the context and the thread
    // to execute the task for closing the database. By the time we get to the
    // destructor, we're guaranteed that the databases are destructed (which is
    // why our ref count is 0 then and we're destructing). Then, the
    // m_databaseThread RefPtr destructor will deref and delete the
    // DatabaseThread.

    if (m_databaseThread && !m_hasRequestedTermination) {
        m_databaseThread->requestTermination(synchronizer);
        m_hasRequestedTermination = true;
        return true;
    }
    return false;
}

bool DatabaseContext::allowDatabaseAccess() const
{
    if (is<Document>(*m_scriptExecutionContext)) {
        Document& document = downcast<Document>(*m_scriptExecutionContext);
        if (!document.page() || (document.page()->usesEphemeralSession() && !SchemeRegistry::allowsDatabaseAccessInPrivateBrowsing(document.securityOrigin()->protocol())))
            return false;
        return true;
    }
    ASSERT(m_scriptExecutionContext->isWorkerGlobalScope());
    // allowDatabaseAccess is not yet implemented for workers.
    return true;
}

void DatabaseContext::databaseExceededQuota(const String& name, DatabaseDetails details)
{
    if (is<Document>(*m_scriptExecutionContext)) {
        Document& document = downcast<Document>(*m_scriptExecutionContext);
        if (Page* page = document.page())
            page->chrome().client().exceededDatabaseQuota(document.frame(), name, details);
        return;
    }
    ASSERT(m_scriptExecutionContext->isWorkerGlobalScope());
    // FIXME: This needs a real implementation; this is a temporary solution for testing.
    const unsigned long long defaultQuota = 5 * 1024 * 1024;
    DatabaseManager::singleton().setQuota(m_scriptExecutionContext->securityOrigin(), defaultQuota);
}

SecurityOrigin* DatabaseContext::securityOrigin() const
{
    return m_scriptExecutionContext->securityOrigin();
}

bool DatabaseContext::isContextThread() const
{
    return m_scriptExecutionContext->isContextThread();
}

} // namespace WebCore
