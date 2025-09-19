#ifndef LINGXIN_SYSTEM_TIME_H
#define LINGXIN_SYSTEM_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int mill_sec;
  int sec;
  int min;
  int hour;
  int day;
  int mon;
  int year;
} LINGXIN_TIME;

//获取设备当前时间
void get_current_time(LINGXIN_TIME *lingxin_time);

#ifdef __cplusplus
}
#endif
#endif // LINGXIN_SYSTEM_TIME_H
