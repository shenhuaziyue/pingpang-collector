# 乒乓球辅助收集程序

用 C 语言写的乒乓球图片收录工具。给每张图算 pHash 指纹，和图库里已有的图比对，快速找出重复或相似的球拍照片。

> 图形界面部分由 AI 生成，仅作学习参考。

## 功能

- 选一张图，自动和图库里的所有图比对，按相似度从高到低列出结果
- 相似度 70% 以上才算命中
- 加入图库时遇到同名文件，可以选择：加后缀另存 / 覆盖 / 取消
- 两个版本：命令行版 `pingpang.exe`、图形界面版 `gui.exe`

## 用法

图库和清单都放在 `data` 目录：

| 路径 | 用途 |
|---|---|
| `data/tuku/` | 存收录进来的图片 |
| `data/tukulujing.txt` | 图片路径清单，一行一个 |

**图形界面版**：双击 `gui.exe`，点「选择图片…」，看完结果点「加入图库」。

**命令行版**：双击 `pingpang.exe`，按提示输入图片路径。

程序用的是相对路径，**必须在工程根目录运行**。

## 编译

需要 MinGW-w64（gcc）。双击 `build.bat` 会同时编出两个 exe。

手动编译：

```
gcc -std=c11 -Wall -Wextra main.c kt/duQuBMP.c kt/jiSuanHash.c kt/hanMingJuLi.c kt/tpruku.c -o pingpang.exe
```

```
gcc -std=c11 -Wall -Wextra -mwindows -finput-charset=GBK -fexec-charset=GBK gui.c kt/duQuBMP.c kt/jiSuanHash.c kt/hanMingJuLi.c kt/tpruku.c -o gui.exe
```

## 文件说明

| 文件 | 作用 |
|---|---|
| `main.c` | 命令行版入口 |
| `gui.c` | 图形界面入口 |
| `kt/duQuBMP.c` | 读 24 位 BMP 的像素数据 |
| `kt/jiSuanHash.c` | 算 pHash 指纹 |
| `kt/hanMingJuLi.c` | 算两个指纹的汉明距离 |
| `kt/tpruku.c` | 把图片复制进图库 |

## 已知限制

- 只支持 24 位色 BMP，其他格式要先另存为 BMP
- 图形界面版暂时没有「删除图库图片」的功能
- 源码是 GBK 编码，手动编译时要带 `-finput-charset=GBK -fexec-charset=GBK`

## 版权

代码仅供学习参考，不作任何担保。
