# 配置libwebsockets的选项
set(LWS_WITH_SSL CACHE BOOL "Enable SSL support" OFF)
set(LWS_WITHOUT_EXTENSIONS CACHE BOOL "Disable extensions" OFF)

set(LWS_WITH_SHARED CACHE BOOL "Disable extensions" OFF)

# 禁用服务器功能
set(LWS_WITHOUT_SERVER CACHE BOOL "Don't build the server part of the library" ON)
set(LWS_WITHOUT_TESTAPPS CACHE BOOL "Don't build the libwebsocket-test-apps" ON)
set(LWS_WITHOUT_TEST_SERVER CACHE BOOL "Don't build the test server" ON)
set(LWS_WITHOUT_TEST_SERVER_EXTPOLL CACHE BOOL "Don't build the test server version that uses external poll" ON)
set(LWS_WITHOUT_TEST_PING CACHE BOOL "Don't build the ping test application" ON)
set(LWS_WITHOUT_TEST_CLIENT CACHE BOOL "Don't build the client test application" ON)