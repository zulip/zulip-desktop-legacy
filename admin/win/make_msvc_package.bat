@echo off

:: Builds a Zulip Desktop release with MSVC


REM set SSO_BUILD=%2

if "%1" == "" (
    echo No version specified!
    exit /b
) else (
    echo Building Version %1
)


echo Loading MSVC Environment
call c:\Qt\qt_env.bat

pushd %TMP%

if exist zulip-desktop\ (
    rmdir zulip-desktop /s /q
)

git clone git@git.zulip.net:eng/zulip-desktop.git
pushd zulip-desktop
git checkout leo-windows-proper-fixes
mkdir build
pushd build

echo Ready to build

cmake -DBUILD_WITH_QT5=On -G "NMake Makefiles JOM" -DCMAKE_BUILD_TYPE=Release ..
jom

SignTool sign /f C:\Users\zulip\signkey.pfx /p berainyacliplachwayacome /t http://timestamp.verisign.com/scripts/timstamp.dll /d "Zulip" /du "https://zulip.com" zulip.exe

echo Building NSIS package
nmake package

:: To sign the executable, uncomment the following line. This requires a signing key zulip.pfx in your home dir
:: SignTool sign /f C:\Users\zulip\signkey.pfx /p PW_HERE /t http://timestamp.verisign.com/scripts/timstamp.dll /d "Zulip" /du "https://zulip.com" "zulip-%1.exe"

mv "zulip-%1.exe" C:\Users\Zulip\Packages

popd
popd

rmdir zulip-desktop /s /q

popd

echo Generated zulip-%1.exe in C:\Users\Zulip\Packages!
