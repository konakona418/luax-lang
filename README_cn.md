# LuaX 语言

> **语法一致性这一块**

学起来简单，用起来方便。

用于敏捷开发屎山代码。

和 Lua 没半毛钱关系。

---

## 简介

**LuaX** 是一门小巧、动态类型的编程语言，适合构建结构简单、模块化的小型应用。

它的特点包括：

* **基于表达式的语法**
  一切皆为表达式。语句只是带有副作用的表达式而已。
  *除了 `let` 声明和各种控制流语句。*

* **模块和类型都是一等公民**
  模块（`mod`）和类型（`type`）都是普通值。爱咋用咋用。

* **没有类**
  至少长得不像。

* **显式的内存模型**
  由简单的垃圾回收器驱动。其实是我写不出来更复杂的了。

* **极简标准库**
  没有轮子为什么不自己造。用 `use` 导入需要的模块。

---

## 设计哲学

* 如果一个东西看起来像副作用，**那它大概率就是副作用**。
* 如果一个东西看起来很难理解，**那它就不该存在**。
* 如果哪儿出了问题，**不关我的事**。
* **闭包无处不在，就算你看不出来。**

---

## Hello, World

```lua
let io = use "std/io";

io::println("Hello, world!");
```

就这样。

甚至还可以更短：

```lua
(use "std/io")::println("Hello, world!");
```

因为 `use` 是个表达式，会返回模块的值。

---

## 示例：带方法的泛型类型

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

* 类型可以作为函数的返回值，这样就能写出泛型类型。
* 构造函数其实就是普通函数。
* `field` 声明的是预期的结构 —— **但不支持私有字段**。
* 注意 `let _VecType = ...` 这一步不能省。之前提到，**闭包无处不在。**
