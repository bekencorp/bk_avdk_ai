1，libosal.a libkws.a libumd.a libssp.a 加载这个几个库
2，main.c里面有些调用逻辑
把过降噪和回声消除的1路数据，送到识别接口
3，osal include里面都是头文件
4，lp_asfix.h grammar.h 分别是800k的声学模型 和 你好魔方的唤醒词的 语法模型

备注:算法做了使用时间限制