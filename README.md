# Introduction

本程序使用一个二进制文件disk.img模拟磁盘，unix文件系统将建立在这个虚拟磁盘上。unix文件系统由5个region组成：超级块、索引节点表位图、索引节点表、数据块位图和数据库区域。

