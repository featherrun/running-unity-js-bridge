APP_ABI := armeabi-v7a x86  

#APP_STL := stlport_static

APP_STL := gnustl_static
APP_CPPFLAGS := -frtti -DCC_ENABLE_CHIPMUNK_INTEGRATION=1 -std=c++11 -fsigned-char -fexceptions