#pragma once

#include <memory>
#include <string>
#include <vector>

#include "WAVM/IR/Types.h"
#include "WAVM/IR/Value.h"
#include "WAVM/Inline/Assert.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Platform/Diagnostics.h"

// Declare IR::Module to avoid including the definition.
namespace WAVM { namespace IR {
	struct Module;
}}

// Declare the different kinds of objects. They are only declared as incomplete struct types here,
// and Runtime clients will only handle opaque pointers to them.
#define WAVM_DECLARE_OBJECT_TYPE(kindId, kindName, Type)                                           \
	struct Type;                                                                                   \
                                                                                                   \
	RUNTIME_API void addGCRoot(const Type* type);                                                  \
	RUNTIME_API void removeGCRoot(const Type* type);                                               \
                                                                                                   \
	RUNTIME_API Runtime::Type* as##kindName(Object* object);                                       \
	RUNTIME_API Runtime::Type* as##kindName##Nullable(Object* object);                             \
	RUNTIME_API const Runtime::Type* as##kindName(const Object* object);                           \
	RUNTIME_API const Runtime::Type* as##kindName##Nullable(const Object* object);                 \
	RUNTIME_API Object* asObject(Type* object);                                                    \
	RUNTIME_API const Object* asObject(const Runtime::Type* object);                               \
                                                                                                   \
	RUNTIME_API void setUserData(Runtime::Type* object, void* userData, void (*finalizer)(void*)); \
	RUNTIME_API void* getUserData(const Runtime::Type* object);                                    \
                                                                                                   \
	template<> inline Runtime::Type* as<Type>(Object * object) { return as##kindName(object); }    \
	template<> inline const Runtime::Type* as<const Type>(const Object* object)                    \
	{                                                                                              \
		return as##kindName(object);                                                               \
	}

namespace WAVM { namespace Runtime {

	struct Object;

	// Tests whether an object is of the given type.
	RUNTIME_API bool isA(const Object* object, const IR::ExternType& type);

	RUNTIME_API IR::ExternType getExternType(const Object* object);

	RUNTIME_API void setUserData(Object* object, void* userData, void (*finalizer)(void*));
	RUNTIME_API void* getUserData(const Object* object);

	inline Object* asObject(Object* object) { return object; }
	inline const Object* asObject(const Object* object) { return object; }

	template<typename Type> Type* as(Object* object);
	template<typename Type> const Type* as(const Object* object);

	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::function, Function, Function);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::table, Table, Table);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::memory, Memory, Memory);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::global, Global, Global);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::exceptionType, ExceptionType, ExceptionType);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::moduleInstance, ModuleInstance, ModuleInstance);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::context, Context, Context);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::compartment, Compartment, Compartment);
	WAVM_DECLARE_OBJECT_TYPE(ObjectKind::foreign, Foreign, Foreign);

	//
	// Garbage collection
	//

	// A GC root pointer.
	template<typename ObjectType> struct GCPointer
	{
		GCPointer() : value(nullptr) {}
		GCPointer(ObjectType* inValue)
		{
			value = inValue;
			if(value) { addGCRoot(value); }
		}
		GCPointer(const GCPointer<ObjectType>& inCopy)
		{
			value = inCopy.value;
			if(value) { addGCRoot(value); }
		}
		GCPointer(GCPointer<ObjectType>&& inMove)
		{
			value = inMove.value;
			inMove.value = nullptr;
		}
		~GCPointer()
		{
			if(value) { removeGCRoot(value); }
		}

		void operator=(ObjectType* inValue)
		{
			if(value) { removeGCRoot(value); }
			value = inValue;
			if(value) { addGCRoot(value); }
		}
		void operator=(const GCPointer<ObjectType>& inCopy)
		{
			if(value) { removeGCRoot(value); }
			value = inCopy.value;
			if(value) { addGCRoot(value); }
		}
		void operator=(GCPointer<ObjectType>&& inMove)
		{
			if(value) { removeGCRoot(value); }
			value = inMove.value;
			inMove.value = nullptr;
		}

		operator ObjectType*() const { return value; }
		ObjectType& operator*() const { return *value; }
		ObjectType* operator->() const { return value; }

	private:
		ObjectType* value;
	};

	// Increments the object's counter of root references.
	RUNTIME_API void addGCRoot(const Object* object);

	// Decrements the object's counter of root referencers.
	RUNTIME_API void removeGCRoot(const Object* object);

	// Frees any unreferenced objects owned by a compartment.
	RUNTIME_API void collectCompartmentGarbage(Compartment* compartment);

	// Clears the given GC root reference to a compartment, and collects garbage for it. Returns
	// true if the entire compartment was freed by the operation, or false if there are remaining
	// root references that can reach it.
	RUNTIME_API bool tryCollectCompartment(GCPointer<Compartment>&& compartment);

	//
	// Exception types
	//

#define WAVM_ENUM_INTRINSIC_EXCEPTION_TYPES(visit)                                                 \
	visit(outOfBoundsMemoryAccess, WAVM::IR::ValueType::anyref, WAVM::IR::ValueType::i64);         \
	visit(outOfBoundsTableAccess, WAVM::IR::ValueType::anyref, WAVM::IR::ValueType::i64);          \
	visit(outOfBoundsDataSegmentAccess,                                                            \
		  WAVM::IR::ValueType::anyref,                                                             \
		  WAVM::IR::ValueType::i64,                                                                \
		  WAVM::IR::ValueType::i64);                                                               \
	visit(outOfBoundsElemSegmentAccess,                                                            \
		  WAVM::IR::ValueType::anyref,                                                             \
		  WAVM::IR::ValueType::i64,                                                                \
		  WAVM::IR::ValueType::i64);                                                               \
	visit(stackOverflow);                                                                          \
	visit(integerDivideByZeroOrOverflow);                                                          \
	visit(invalidFloatOperation);                                                                  \
	visit(invokeSignatureMismatch);                                                                \
	visit(reachedUnreachable);                                                                     \
	visit(indirectCallSignatureMismatch);                                                          \
	visit(uninitializedTableElement, WAVM::IR::ValueType::anyref, WAVM::IR::ValueType::i64);       \
	visit(calledAbort);                                                                            \
	visit(calledUnimplementedIntrinsic);                                                           \
	visit(outOfMemory);                                                                            \
	visit(misalignedAtomicMemoryAccess, WAVM::IR::ValueType::i64);                                 \
	visit(invalidArgument);

	// Information about a runtime exception.
	namespace ExceptionTypes {
#define DECLARE_INTRINSIC_EXCEPTION_TYPE(name, ...) RUNTIME_API extern ExceptionType* name;
		WAVM_ENUM_INTRINSIC_EXCEPTION_TYPES(DECLARE_INTRINSIC_EXCEPTION_TYPE)
#undef DECLARE_INTRINSIC_EXCEPTION_TYPE
	};

	// Creates an exception type instance.
	RUNTIME_API ExceptionType* createExceptionType(Compartment* compartment,
												   IR::ExceptionType sig,
												   std::string&& debugName);

	// Returns a string that describes the given exception type.
	RUNTIME_API std::string describeExceptionType(const ExceptionType* type);

	// Returns the parameter types for an exception type instance.
	RUNTIME_API IR::TypeTuple getExceptionTypeParameters(const ExceptionType* type);

	//
	// Resource quotas
	//

	struct ResourceQuota;
	typedef std::shared_ptr<ResourceQuota> ResourceQuotaRef;
	typedef std::shared_ptr<const ResourceQuota> ResourceQuotaConstRef;
	typedef const std::shared_ptr<ResourceQuota>& ResourceQuotaRefParam;
	typedef const std::shared_ptr<const ResourceQuota>& ResourceQuotaConstRefParam;

	RUNTIME_API ResourceQuotaRef createResourceQuota();

	RUNTIME_API Uptr getResourceQuotaMaxTableElems(ResourceQuotaConstRefParam);
	RUNTIME_API Uptr getResourceQuotaCurrentTableElems(ResourceQuotaConstRefParam);
	RUNTIME_API void setResourceQuotaMaxTableElems(ResourceQuotaRefParam, Uptr maxTableElems);

	RUNTIME_API Uptr getResourceQuotaMaxMemoryPages(ResourceQuotaConstRefParam);
	RUNTIME_API Uptr getResourceQuotaCurrentMemoryPages(ResourceQuotaConstRefParam);
	RUNTIME_API void setResourceQuotaMaxMemoryPages(ResourceQuotaRefParam, Uptr maxMemoryPages);

	//
	// Exceptions
	//

	struct Exception;

	// Exception UserData
	RUNTIME_API void setUserData(Exception* exception, void* userData, void (*finalizer)(void*));
	RUNTIME_API void* getUserData(const Exception* exception);

	// Creates a runtime exception.
	RUNTIME_API Exception* createException(ExceptionType* type,
										   const IR::UntaggedValue* arguments,
										   Uptr numArguments,
										   Platform::CallStack&& callStack);

	// Destroys a runtime exception.
	RUNTIME_API void destroyException(Exception* exception);

	// Returns the type of an exception.
	RUNTIME_API ExceptionType* getExceptionType(const Exception* exception);

	// Returns a specific argument of an exception.
	RUNTIME_API IR::UntaggedValue getExceptionArgument(const Exception* exception, Uptr argIndex);

	// Returns the call stack at the origin of an exception.
	RUNTIME_API const Platform::CallStack& getExceptionCallStack(const Exception* exception);

	// Returns a string that describes the given exception cause.
	RUNTIME_API std::string describeException(const Exception* exception);

	// Throws a runtime exception.
	[[noreturn]] RUNTIME_API void throwException(Exception* exception);

	// Creates and throws a runtime exception.
	[[noreturn]] RUNTIME_API void throwException(ExceptionType* type,
												 const std::vector<IR::UntaggedValue>& arguments
												 = {});

	// Calls a thunk and catches any runtime exceptions that occur within it. Note that the
	// catchThunk takes ownership of the exception, and is responsible for calling destroyException.
	RUNTIME_API void catchRuntimeExceptions(const std::function<void()>& thunk,
											const std::function<void(Exception*)>& catchThunk);

	// Same as catchRuntimeExceptions, but works on a relocatable stack (e.g. a stack for a thread
	// that will be forked with Platform::forkCurrentThread).
	RUNTIME_API void catchRuntimeExceptionsOnRelocatableStack(void (*thunk)(),
															  void (*catchThunk)(Exception*));

	// Calls a thunk and ensures that any signals that occur within the thunk will be thrown as
	// runtime exceptions.
	RUNTIME_API void unwindSignalsAsExceptions(const std::function<void()>& thunk);

	// Describes an instruction pointer.
	bool describeInstructionPointer(Uptr ip, std::string& outDescription);

	// Describes a call stack.
	RUNTIME_API std::vector<std::string> describeCallStack(const Platform::CallStack& callStack);

	//
	// Functions
	//

	// Invokes a Function with the given arguments, and returns the result. The result is
	// returned as a pointer to an untagged value that is stored in the Context that will be
	// overwritten by subsequent calls to invokeFunctionUnchecked. This allows using this function
	// in a call stack that will be forked, since returning the result as a value will be lowered to
	// passing in a pointer to stack memory for most calling conventions.
	RUNTIME_API IR::UntaggedValue* invokeFunctionUnchecked(Context* context,
														   const Function* function,
														   const IR::UntaggedValue* arguments);

	// Like invokeFunctionUnchecked, but returns a result tagged with its type, and takes arguments
	// as tagged values. If the wrong number or types or arguments are provided, a runtime exception
	// is thrown.
	RUNTIME_API IR::ValueTuple invokeFunctionChecked(Context* context,
													 const Function* function,
													 const std::vector<IR::Value>& arguments);

	// Returns the type of a Function.
	RUNTIME_API IR::FunctionType getFunctionType(const Function* function);

	//
	// Tables
	//

	// Creates a Table. May return null if the memory allocation fails.
	RUNTIME_API Table* createTable(Compartment* compartment,
								   IR::TableType type,
								   Object* element,
								   std::string&& debugName,
								   ResourceQuotaRefParam resourceQuota = ResourceQuotaRef());

	// Reads an element from the table. Throws an outOfBoundsTableAccess exception if index is
	// out-of-bounds.
	RUNTIME_API Object* getTableElement(const Table* table, Uptr index);

	// Writes an element to the table, a returns the previous value of the element.
	// Throws an outOfBoundsTableAccess exception if index is out-of-bounds.
	RUNTIME_API Object* setTableElement(Table* table, Uptr index, Object* newValue);

	// Gets the current size of the table.
	RUNTIME_API Uptr getTableNumElements(const Table* table);

	// Returns the type of a table.
	RUNTIME_API IR::TableType getTableType(const Table* table);

	// Grows or shrinks the size of a table by numElements. Returns the previous size of the table.
	RUNTIME_API bool growTable(Table* table,
							   Uptr numElements,
							   Uptr* outOldNumElems = nullptr,
							   Object* initialElement = nullptr);

	//
	// Memories
	//

	// Creates a Memory. May return null if the memory allocation fails.
	RUNTIME_API Memory* createMemory(Compartment* compartment,
									 IR::MemoryType type,
									 std::string&& debugName,
									 ResourceQuotaRefParam resourceQuota = ResourceQuotaRef());

	// Gets the base address of the memory's data.
	RUNTIME_API U8* getMemoryBaseAddress(Memory* memory);

	// Gets the current size of the memory in pages.
	RUNTIME_API Uptr getMemoryNumPages(const Memory* memory);

	// Returns the type of a memory.
	RUNTIME_API IR::MemoryType getMemoryType(const Memory* memory);

	// Grows or shrinks the size of a memory by numPages. Returns the previous size of the memory.
	RUNTIME_API bool growMemory(Memory* memory, Uptr numPages, Uptr* outOldNumPages = nullptr);

	// Unmaps a range of memory pages within the memory's address-space.
	RUNTIME_API void unmapMemoryPages(Memory* memory, Uptr pageIndex, Uptr numPages);

	// Validates that an offset range is wholly inside a Memory's virtual address range.
	// Note that this returns an address range that may fault on access, though it's guaranteed not
	// to be mapped by anything other than the given Memory.
	RUNTIME_API U8* getReservedMemoryOffsetRange(Memory* memory, Uptr offset, Uptr numBytes);

	// Validates that an offset range is wholly inside a Memory's committed pages.
	RUNTIME_API U8* getValidatedMemoryOffsetRange(Memory* memory, Uptr offset, Uptr numBytes);

	// Validates an access to a single element of memory at the given offset, and returns a
	// reference to it.
	template<typename Value> Value& memoryRef(Memory* memory, Uptr offset)
	{
		return *(Value*)getValidatedMemoryOffsetRange(memory, offset, sizeof(Value));
	}

	// Validates an access to multiple elements of memory at the given offset, and returns a pointer
	// to it.
	template<typename Value> Value* memoryArrayPtr(Memory* memory, Uptr offset, Uptr numElements)
	{
		return (Value*)getValidatedMemoryOffsetRange(memory, offset, numElements * sizeof(Value));
	}

	//
	// Globals
	//

	// Creates a Global with the specified type. The initial value is set to the appropriate zero.
	RUNTIME_API Global* createGlobal(Compartment* compartment,
									 IR::GlobalType type,
									 ResourceQuotaRefParam resourceQuota = ResourceQuotaRef());

	// Initializes a Global with the specified value. May not be called more than once/Global.
	RUNTIME_API void initializeGlobal(Global* global, IR::Value value);

	// Reads the current value of a global.
	RUNTIME_API IR::Value getGlobalValue(const Context* context, const Global* global);

	// Writes a new value to a global, and returns the previous value.
	RUNTIME_API IR::Value setGlobalValue(Context* context,
										 const Global* global,
										 IR::Value newValue);

	// Returns the type of a global.
	RUNTIME_API IR::GlobalType getGlobalType(const Global* global);

	//
	// Modules
	//

	struct Module;
	typedef std::shared_ptr<Module> ModuleRef;
	typedef std::shared_ptr<const Module> ModuleConstRef;
	typedef const std::shared_ptr<Module>& ModuleRefParam;
	typedef const std::shared_ptr<const Module>& ModuleConstRefParam;

	// Compiles an IR module to object code.
	RUNTIME_API ModuleRef compileModule(const IR::Module& irModule);

	// Extracts the compiled object code for a module. This may be used as an input to
	// loadPrecompiledModule to bypass redundant compilations of the module.
	RUNTIME_API std::vector<U8> getObjectCode(ModuleConstRefParam module);

	// Loads a previously compiled module from a combination of an IR module and the object code
	// returned by getObjectCode for the previously compiled module.
	RUNTIME_API ModuleRef loadPrecompiledModule(const IR::Module& irModule,
												const std::vector<U8>& objectCode);

	// Accesses the IR for a compiled module.
	RUNTIME_API const IR::Module& getModuleIR(ModuleConstRefParam module);

	//
	// Instances
	//

	typedef std::vector<Object*> ImportBindings;

	// Instantiates a compiled module, bindings its imports to the specified objects. May throw a
	// runtime exception for bad segment offsets.
	RUNTIME_API ModuleInstance* instantiateModule(Compartment* compartment,
												  ModuleConstRefParam module,
												  ImportBindings&& imports,
												  std::string&& debugName,
												  ResourceQuotaRefParam resourceQuota
												  = ResourceQuotaRef());

	// Gets the start function of a ModuleInstance.
	RUNTIME_API Function* getStartFunction(const ModuleInstance* moduleInstance);

	// Gets the default table/memory for a ModuleInstance.
	RUNTIME_API Memory* getDefaultMemory(const ModuleInstance* moduleInstance);
	RUNTIME_API Table* getDefaultTable(const ModuleInstance* moduleInstance);

	// Gets an object exported by a ModuleInstance by name.
	RUNTIME_API Object* getInstanceExport(const ModuleInstance* moduleInstance,
										  const std::string& name);

	// Gets an array of the objects exported by a module instance. The array indices correspond to
	// the IR::Module::exports array.
	RUNTIME_API const std::vector<Object*>& getInstanceExports(
		const ModuleInstance* moduleInstance);

	//
	// Compartments
	//

	RUNTIME_API Compartment* createCompartment();

	RUNTIME_API Compartment* cloneCompartment(const Compartment* compartment);

	RUNTIME_API Object* remapToClonedCompartment(Object* object, const Compartment* newCompartment);
	RUNTIME_API Function* remapToClonedCompartment(Function* function,
												   const Compartment* newCompartment);
	RUNTIME_API Table* remapToClonedCompartment(Table* table, const Compartment* newCompartment);
	RUNTIME_API Memory* remapToClonedCompartment(Memory* memory, const Compartment* newCompartment);
	RUNTIME_API Global* remapToClonedCompartment(Global* global, const Compartment* newCompartment);
	RUNTIME_API ExceptionType* remapToClonedCompartment(ExceptionType* exceptionType,
														const Compartment* newCompartment);
	RUNTIME_API ModuleInstance* remapToClonedCompartment(ModuleInstance* moduleInstance,
														 const Compartment* newCompartment);

	RUNTIME_API Compartment* getCompartment(const Object* object);
	RUNTIME_API bool isInCompartment(const Object* object, const Compartment* compartment);

	//
	// Contexts
	//

	RUNTIME_API Context* createContext(Compartment* compartment);

	RUNTIME_API struct ContextRuntimeData* getContextRuntimeData(const Context* context);
	RUNTIME_API Context* getContextFromRuntimeData(struct ContextRuntimeData* contextRuntimeData);
	RUNTIME_API Compartment* getCompartmentFromContextRuntimeData(
		struct ContextRuntimeData* contextRuntimeData);

	// Creates a new context, initializing its mutable global state from the given context.
	RUNTIME_API Context* cloneContext(const Context* context, Compartment* newCompartment);

	//
	// Foreign objects
	//

	RUNTIME_API Foreign* createForeign(Compartment* compartment,
									   void* userData,
									   void (*finalizer)(void*));
}}
