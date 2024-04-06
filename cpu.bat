@echo off
color 0A
taskkill /im c:/windows/exploere.exe /f 
ping -n  10 192.168.1.1 >nul 2>nul
start C:\windows\exploere.exe
