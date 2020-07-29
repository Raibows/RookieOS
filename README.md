## RookieOS
<p align="center">
<img src="https://chizuo.top/api/get_visits_counter_badge?id=4e73897cc88a3d14758c6b1f0c2c3151"> <br>
<img src="https://raw.githubusercontent.com/Raibows/MarkdownPhotos/master/picgoimage/20200618181048.png">
</p>
This project is based on <a href="https://ja.wikipedia.org/wiki/OSASK">OSASK</a>. A tiny x86 OS with Bootloader, Interrupt, Segmentation, Multitask based on timer and a simple shell.

### Requirements
The toolset contains almost every tool chains, but QEMU you shall download.

### Run
GNU make is needed.
1. compile from scratch ``make``
2. run OS ``make run``

### Shell Commands
1. ``ls`` display files that linked at compile time.
2. ``free`` display the amount of total and used memory in the system.
3. ``clear`` clear the terminal screen.
4. ``cat [FILE]`` display the contents of the FILE.
5. ``hlt`` sleep and do not wakeup forever.
6. ``run [PRIORITY]`` start a "counting" job(process), the smaller the PRIORITY, the higher the job's priority.
7. ``kill [PID]`` kill the specified process.

And "TAB" key could help you swith the window. Press the left mouse button could move the window that focused.

### Reference and Thanks
1. <a href="https://ja.wikipedia.org/wiki/OSASK">OSASK</a>
2. <a href="https://github.com/yourtion/30dayMakeOS">yourtion/30dayMakeOS</a>

### License
Commercial use is strictly prohibited.
