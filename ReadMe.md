## 简介

[`include/asyncxx`](include/asyncxx) 目录下的每份 HPP 文件包含一个和文件名同名的异步组件.

## 工具链

- Clang++ 21
- libstdc++ 15 (from GCC 15)

## 运行

[`src`](src) 目录下每份 CPP 文件都是 `TestXXX.cpp` 的格式, 其中 `XXX` 是组件名.
要测试, 例如, 组件 `AsyncManualResetEvent`:

```bash
./test.mk AsyncManualResetEvent
```
