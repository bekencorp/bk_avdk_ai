使用说明：
1、windows系统下，解压bk_avi.7z
2、打开CMD命令行窗口，进入到解压后的目录bk_avi
3、输入命令进行格式转换，常用命令说明:
  1) 查看工具help说明(通过该命令可以查看详细参数说明)
    .\bk_avi.exe -h
  2) 转换avi格式(-i 参数后面指定要转换的源avi文件)
    .\bk_avi.exe -i .\sample.avi
  3) 转换avi格式, 并且对左、右两边分别做旋转 (-i 参数后面指定要转换的源avi文件 -lr 参数指定左边旋转的度数  -rr 参数指定右边旋转的度数)
    以下例子中将sample.avi文件的左边顺时针旋转90度，右边顺时针旋转180度
    .\bk_avi.exe -i .\sample.avi -lr 90 -rr 180
    
    以下例子中将sample.avi文件的左边顺时针旋转90度，右边不旋转
    .\bk_avi.exe -i .\sample.avi -lr 90

4. 运行完成后，会在output文件夹内最新生成的文件夹下生成output.avi文件，即为转换后的avi文件。

