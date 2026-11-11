// standard/1/Collections.os
// Plan's `std/collections`: one import that pulls in every collection. Use as Collections.List.List<T>, Collections.Map.Map<K,V>, ...
// (If your module system supports re-exports, replace these with `export module`.)
module List = import "standard/1/List.os";
module Map = import "standard/1/Map.os";
module Set = import "standard/1/Set.os";
module Queue = import "standard/1/Queue.os";
module Stack = import "standard/1/Stack.os";
module Deque = import "standard/1/Deque.os";
module PriorityQueue = import "standard/1/PriorityQueue.os";
module Algo = import "standard/1/Algo.os";
