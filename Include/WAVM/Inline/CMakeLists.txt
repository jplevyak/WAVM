set(PublicHeaders
	Assert.h
	BasicTypes.h
	Config.h.in
	CLI.h
	ConcurrentHashMap.h
	DenseStaticIntSet.h
	Errors.h
	FloatComponents.h
	Hash.h
	HashMap.h   Impl/HashMapImpl.h   Impl/HashMap.natvis
	HashSet.h   Impl/HashSetImpl.h   Impl/HashSet.natvis
	HashTable.h Impl/HashTableImpl.h Impl/HashTable.natvis
	I128.h      Impl/I128Impl.h      Impl/I128Impl.LICENSE
	IndexMap.h
	IntrusiveSharedPtr.h
	IsNameChar.h
	Lock.h
	OptionalStorage.h
	Serialization.h
	Time.h
	Timing.h
	Unicode.h)

WAVM_ADD_LIB_COMPONENT(Inline
	NONCOMPILED_SOURCES ${PublicHeaders})

add_subdirectory(xxhash)