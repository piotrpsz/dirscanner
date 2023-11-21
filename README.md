# dirscanner
Disk search program. Searches for directories and files with the specified name,
files containing the specified text. Name and text searches are performed using regular expressions.<br>

Scanning of individual directories and subdirectories is performed in parallel using the TBB library.<br>
Each directory is scanned in dedicated tasks (not a posix thread).<br>
Tasks are intelligently allocated to individual threads by the scheduler in TBB.<br>
Posix threads are also created and managed by the TBB library.<br>

The linear clean disk scan (without checking anything) of my whole disk <br>
(over a million directories, almost seven million files) takes:
```
execution time: 218.10626712500002s
```
Parallel scanning (in dedicated tbb-tasks) takes:
```
execution time: 45.249340667000006s
```
(Of course, results will be different on different computers.)