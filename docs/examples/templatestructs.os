struct DynArray<T> {
    size: int = 0;
    first: T*? = nullptr;
    append() => void {};
    prepend() => void {};
    push_back(other: T) => void { this.append(T) };
    push(other: T) => void { this.prepend(T) };
    pop() => void {};
    pop_back() => void {};
}

DynArray<T> can + (other: T) => void {
    this.push(T);
}

DynArray<T> can -- () => void {
    this.pop();
}

DynArray<T> can [] (index: int) => T? {
    if (index > this.size) {
        return null;
    }

    const itemPosition =  this.first + (index * sizeOf(T));
    
    return *itemPosition;
}