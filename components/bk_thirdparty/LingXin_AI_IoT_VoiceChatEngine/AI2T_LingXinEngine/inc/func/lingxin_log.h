#ifndef LINGXIN_LOG_H
#define LINGXIN_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * log打印，根据log级别使用下面这四种方法
 * log中自动添加当前文件名和代码行，无需手动设置
 * log格式：[time] [version] [level] [module:line]: log内容
 */
#define lingxin_log_info(format, ...) _lingxin_log_print_internal_(LINGXIN_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define lingxin_log_debug(format, ...) _lingxin_log_print_internal_(LINGXIN_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define lingxin_log_warn(format, ...) _lingxin_log_print_internal_(LINGXIN_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define lingxin_log_error(format, ...) _lingxin_log_print_internal_(LINGXIN_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)


#define LINGXIN_INFO		(1 << 0)
#define	LINGXIN_DEBUG		(1 << 1)
#define	LINGXIN_WARN		(1 << 2)
#define	LINGXIN_ERROR		(1 << 3)
#define	LINGXIN_USER		(1 << 4)


//内部方法，勿用
void _lingxin_log_print_internal_(int level, const char* filePath, int line, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // LINGXIN_LOG_H