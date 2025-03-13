#pragma once
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>
// 3. Memory management and pointers
// Provide:
// 
// Explicit Memory Allocation: Allow users to allocate memory manually, but wrap it in objects to ensure safety.
// Pointers as Objects: Model pointers and memory as objects, exposing low-level operations like dereferencing and address manipulation.
class PointerObj : public Object {
private:
    uintptr_t address;

public:
    PointerObj(uintptr_t addr);

    template <typename T>
    T getValue() const {
        return *reinterpret_cast<T*>(address);
    }

    template <typename T>
    void setValue(T value) {
        *reinterpret_cast<T*>(address) = value;
    }

    uintptr_t getAddress() const;

    template <typename T>
    std::shared_ptr<PointerObj> asType() const {
        return std::make_shared<PointerObj>(reinterpret_cast<uintptr_t>(reinterpret_cast<T*>(address)));
    }

    void registerMethods();
    void registerProperties();
    void setDefaultValue();
};