# The LuaX Programming Language

> **Consistency over efficiency.**

Easy to learn, Easy to use.

Not easy to maintain.

No magic. No syntactic sugar. No funny features. No boilerplate code.

Has nothing to do with Lua. *I'm bad at naming things.*

**No bullshit.**

## Introduction

**LuaX** is a small, dynamically typed programming language designed for building simple modular applications with some simple structure.

It features:

* **Expression-based syntax**
  Everything is an expression. Statements are just expressions with effects.

  *Except for `let` statements.*

* **First-class modules and types**
  Modules (`mod`) and types (`type`) are ordinary values. You can pass them around, construct them, and organize them however you like.

* **No classes**
  Just functions, types. Simple and explicit.

* **Explicit memory model**
  Driven by a simple garbage collector.

* **Minimal standard library**
  You write what you need. Use `use` to import what you want.

---

## Philosophy

* If something looks like a side effect, **it probably is**.
* If something is too clever to understand, **it probably shouldn't exist**.
* If something breaks, **you'll know where and why**.
* **Closures are everywhere, even if they don't seem like one.**

---

## Hello, World

```lua
let io = use "std/io";

io::println("Hello, world!");
```

Yes, that’s it.

---

## Example: Generic Type with Methods

```lua
func Vector2(TData) {
    let _VecType = type {
        field x = TData;
        field y = TData;

        func new(x, y) {
            let obj = _VecType {};
            obj.x = x;
            obj.y = y;
            return obj;
        }
    };
    return _VecType;
}

let Vector2i = Vector2(typing::Int);
let vec = Vector2i::new(1, 2);
io::println(vec.x, vec.y);
```

* Types can be returned from functions. Use it for creating generic types.
* Constructors are just normal functions.
* `field` defines the expected structure, but **there is no private state**.
* Note that the `let _VecType = ...` is necessary. That's because, **as previously mentioned, closures are everywhere.**
