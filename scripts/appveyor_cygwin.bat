if _%arch%_ == _x64_ set CYGWIN_ROOT=C:\cygwin64
if _%arch%_ == _x86_ set CYGWIN_ROOT=C:\cygwin
set PATH=%CYGWIN_ROOT%\bin;%SYSTEMROOT%\system32

goto %1

:install
if _%arch%_ == _x64_ set SETUP=setup-x86_64.exe
if _%arch%_ == _x86_ set SETUP=setup-x86.exe
if not exist %PKGCACHE% mkdir %PKGCACHE%

echo Updating Cygwin and installing build prerequsites
%CYGWIN_ROOT%\%SETUP% -qnNdO -R "%CYGWIN_ROOT%" -s "%CYGWIN_MIRROR%" -l "%PKGCACHE%" -g -P "bison,ccache,flex,glproto,libexpat-devel,libllvm-devel,libxcb-dri2-devel,libxcb-glx-devel,libxcb-xfixes-devel,libX11-devel,libX11-xcb-devel,libXdamage-devel,libXext-devel,libXfixes-devel,ninja,meson,python2-mako,zlib-devel"
goto :eof

:build_script
bash -lc "cd $APPVEYOR_BUILD_FOLDER; meson build -Ddri-drivers= -Dgallium-drivers=swrast -Dvulkan-drivers= -Dplatforms=x11,surfaceless -Dglx=dri --wrap-mode=nofallback && ninja -C build && ccache -s"
goto :eof

:after_build
goto :eof
