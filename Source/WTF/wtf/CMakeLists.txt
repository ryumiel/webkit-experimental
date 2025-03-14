set(WTF_HEADERS
    ASCIICType.h
    AVLTree.h
    Assertions.h
    Atomics.h
    Bag.h
    BagToHashMap.h
    BitVector.h
    Bitmap.h
    BoundsCheckedPointer.h
    BumpPointerAllocator.h
    ByteOrder.h
    CompilationThread.h
    Compiler.h
    CryptographicUtilities.h
    CryptographicallyRandomNumber.h
    CurrentTime.h
    DataLog.h
    DateMath.h
    DecimalNumber.h
    DeferrableRefCounted.h
    Deque.h
    DisallowCType.h
    DoublyLinkedList.h
    FastMalloc.h
    FeatureDefines.h
    FilePrintStream.h
    FlipBytes.h
    Forward.h
    FunctionDispatcher.h
    Functional.h
    GetPtr.h
    GregorianDateTime.h
    HashCountedSet.h
    HashFunctions.h
    HashIterators.h
    HashMap.h
    HashMethod.h
    HashSet.h
    HashTable.h
    HashTraits.h
    HexNumber.h
    IteratorAdaptors.h
    IteratorRange.h
    ListHashSet.h
    Locker.h
    MD5.h
    MainThread.h
    MallocPtr.h
    MathExtras.h
    MediaTime.h
    MessageQueue.h
    MetaAllocator.h
    MetaAllocatorHandle.h
    Noncopyable.h
    NumberOfCores.h
    OSAllocator.h
    OSRandomSource.h
    OwnPtr.h
    OwnPtrCommon.h
    PageAllocation.h
    PageBlock.h
    PageReservation.h
    ParallelJobs.h
    ParallelJobsGeneric.h
    ParallelJobsLibdispatch.h
    ParallelJobsOpenMP.h
    PassOwnPtr.h
    PassRef.h
    PassRefPtr.h
    Platform.h
    PrintStream.h
    ProcessID.h
    RAMSize.h
    RandomNumber.h
    RandomNumberSeed.h
    RawPointer.h
    RedBlackTree.h
    Ref.h
    RefCounted.h
    RefCountedLeakCounter.h
    RefCounter.h
    RefPtr.h
    RetainPtr.h
    RunLoop.h
    SHA1.h
    SaturatedArithmetic.h
    SegmentedVector.h
    SpinLock.h
    StackBounds.h
    StackStats.h
    StaticConstructors.h
    StdLibExtras.h
    Stopwatch.h
    StringExtras.h
    StringHasher.h
    StringPrintStream.h
    ThreadIdentifierDataPthreads.h
    ThreadSafeRefCounted.h
    ThreadSpecific.h
    Threading.h
    ThreadingPrimitives.h
    VMTags.h
    ValueCheck.h
    Vector.h
    VectorTraits.h
    WTFThreadData.h
    WorkQueue.h
    WeakPtr.h
    dtoa.h

    dtoa/bignum-dtoa.h
    dtoa/bignum.h
    dtoa/cached-powers.h
    dtoa/diy-fp.h
    dtoa/double-conversion.h
    dtoa/double.h
    dtoa/fast-dtoa.h
    dtoa/fixed-dtoa.h
    dtoa/strtod.h
    dtoa/utils.h

    text/AtomicString.h
    text/AtomicStringImpl.h
    text/AtomicStringTable.h
    text/Base64.h
    text/CString.h
    text/IntegerToStringConversion.h
    text/LChar.h
    text/StringBuffer.h
    text/StringCommon.h
    text/StringHash.h
    text/StringImpl.h
    text/StringView.h
    text/WTFString.h

    threads/BinarySemaphore.h

    unicode/CharacterNames.h
    unicode/Collator.h
    unicode/UTF8.h
)

set(WTF_SOURCES
    Assertions.cpp
    Atomics.cpp
    BitVector.cpp
    CompilationThread.cpp
    CryptographicUtilities.cpp
    CryptographicallyRandomNumber.cpp
    CurrentTime.cpp
    DataLog.cpp
    DateMath.cpp
    DecimalNumber.cpp
    FastBitVector.cpp
    FastMalloc.cpp
    FilePrintStream.cpp
    FunctionDispatcher.cpp
    GregorianDateTime.cpp
    HashTable.cpp
    MD5.cpp
    MainThread.cpp
    MediaTime.cpp
    MetaAllocator.cpp
    NumberOfCores.cpp
    OSRandomSource.cpp
    PageBlock.cpp
    ParallelJobsGeneric.cpp
    PrintStream.cpp
    RAMSize.cpp
    RandomNumber.cpp
    RefCountedLeakCounter.cpp
    RefCounter.cpp
    RunLoop.cpp
    SHA1.cpp
    SixCharacterHash.cpp
    StackStats.cpp
    StackBounds.cpp
    StringPrintStream.cpp
    Threading.cpp
    WTFThreadData.cpp
    WorkQueue.cpp
    dtoa.cpp

    dtoa/bignum-dtoa.cc
    dtoa/bignum.cc
    dtoa/cached-powers.cc
    dtoa/diy-fp.cc
    dtoa/double-conversion.cc
    dtoa/fast-dtoa.cc
    dtoa/fixed-dtoa.cc
    dtoa/strtod.cc

    text/AtomicString.cpp
    text/AtomicStringTable.cpp
    text/Base64.cpp
    text/CString.cpp
    text/StringBuilder.cpp
    text/StringImpl.cpp
    text/StringStatics.cpp
    text/StringView.cpp
    text/WTFString.cpp

    threads/BinarySemaphore.cpp

    unicode/UTF8.cpp
)

set(WTF_INCLUDE_DIRECTORIES
    "${BMALLOC_DIR}"
    "${WTF_DIR}"
    "${WTF_DIR}/wtf"
    "${WTF_DIR}/wtf/dtoa"
    "${WTF_DIR}/wtf/text"
    "${WTF_DIR}/wtf/threads"
    "${WTF_DIR}/wtf/unicode"
    "${THIRDPARTY_DIR}"
    "${CMAKE_BINARY_DIR}"
    "${DERIVED_SOURCES_DIR}"
)

set(WTF_LIBRARIES
    ${CMAKE_DL_LIBS}
    bmalloc
)

list(APPEND WTF_SOURCES
    unicode/icu/CollatorICU.cpp
)
list(APPEND WTF_INCLUDE_DIRECTORIES
    ${ICU_INCLUDE_DIRS}
)
list(APPEND WTF_LIBRARIES
    ${ICU_I18N_LIBRARIES}
    ${ICU_LIBRARIES}
)

if (WIN32)
    list(APPEND WTF_SOURCES
        OSAllocatorWin.cpp
        ThreadSpecificWin.cpp
        ThreadingWin.cpp
    )
else ()
    list(APPEND WTF_HEADERS
        ThreadIdentifierDataPthreads.h
    )
    list(APPEND WTF_SOURCES
        OSAllocatorPosix.cpp
        ThreadIdentifierDataPthreads.cpp
        ThreadingPthreads.cpp
    )
endif ()

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

WEBKIT_WRAP_SOURCELIST(${WTF_SOURCES})
include_directories(${WTF_INCLUDE_DIRECTORIES})
add_library(WTF ${WTF_LIBRARY_TYPE} ${WTF_HEADERS} ${WTF_SOURCES})
target_link_libraries(WTF ${WTF_LIBRARIES})
set_target_properties(WTF PROPERTIES COMPILE_DEFINITIONS "BUILDING_WTF")
set_target_properties(WTF PROPERTIES FOLDER "JavaScriptCore")

if (WTF_OUTPUT_NAME)
    set_target_properties(WTF PROPERTIES OUTPUT_NAME ${WTF_OUTPUT_NAME})
endif ()
