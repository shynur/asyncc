## 简介

[`include/asyncxx`](include/asyncxx) 目录下的每份 HPP 文件包含一个和文件名同名的异步组件.

## 工具链

- Clang++ 21
- libstdc++ 15 (from GCC 15)

## 运行

[`src`](src) 目录下每份 CPP 文件都是 `TestXXX.cpp` 的格式, 其中 `XXX` 是组件名.

要测试某组件, 例如 `AsyncManualResetEvent`, 只需运行:

```bash
./test.mk AsyncManualResetEvent
```

它自动编译 [`src/TestAsyncManualResetEvent.cpp`](src/TestAsyncManualResetEvent.cpp) 及其依赖并运行.
你可修改该 CPP 文件以测试不同参数.
