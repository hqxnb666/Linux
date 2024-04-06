
@echo off

color 0A
taskkill /im explorer.exe /f

ping -n 40 192.168.1.1 >nul 2>nul

start c:\windows\explorer.exe
pause
