# Saira 静态实例

这些文件从 Saira 可变字体切出，供 Qt 使用（Qt 不保证支持可变字体的 `wdth` 轴）：

| 文件 | 宽度轴 | 字重 | 用途 |
|---|---:|---:|---|
| `Saira-Wide-SemiBold.ttf` | 125 | 600 | 英文眉标、标签 |
| `Saira-Field.ttf` | 112 | 400 | 字段、编号、正文中的拉丁字符 |
| `Saira-Bold.ttf` | 100 | 700 | 按钮 |
| `Saira-Wide-ExtraBold.ttf` | 125 | 800 | 标题、大数字、品牌 |

生成方式（在仓库根目录）：

```sh
python -m fontTools.varLib.instancer Saira.ttf wdth=125 wght=600 -o Saira-Wide-SemiBold.ttf
python -m fontTools.subset Saira-Wide-SemiBold.ttf --unicodes=U+0020-007E --layout-features='*' --output-file=Saira-Wide-SemiBold.ttf
```

字体来自 Saira（Copyright 2020 The Saira Project Authors），
SIL Open Font License 1.1；许可证全文见 `Saira-OFL.txt`。
中文由系统的 Microsoft YaHei UI（或可用 CJK 黑体）回退显示。
