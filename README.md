# A simple 2048 game in terminal

一个晚上两三小时速写的，可能有bug（ is_new\[4\]\[4\] 就是写完发现和2048规则不同加的，不想重构了... ） （ 话说我今天才知道要写2048啊... ）

Known bug(s):

1.If user click esc, the arrow keys will have no effect for 1 time.

### Reference:

1._[《UNIX环境高级编程（第三版）》 ( APUE )](https://raw.githubusercontent.com/yiyi079/XD2048/master/UNIX环境高级编程（中文第三版）.pdf)_ ( signal, thread, terminal )

2.[SHELL：echo -e "\033\[字背景颜色;字体颜色m字符串\033\[0m"](https://blog.csdn.net/roler_/article/details/17506181) ( terminal )

3.[Markdown 教程 | 菜鸟教程](https://www.runoob.com/markdown/md-tutorial.html) ( Markdown )
