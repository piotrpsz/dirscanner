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

To install the TBB library on macOS: 
<ul>
    <li>brew update</li>
    <li>brew install tbb</li>
</ul>

I use two of my libraries in the program: 'share' and 'clap'.<br>
To add them to your project you need to add them as submodules (when git is already initialized in your project):
<ul>
    <li>git submodule add https://github.com/piotrpsz/share.git</li>
    <li>git submodule add https://github.com/piotrpsz/clap.git</li>
</ul>
<br>
The program runs on macOS and should run on Linux. It almost certainly doesn't work on Windows.

## Example of use
./dirscanner -wq --dir '/Users/piotr' --text 'filesystem' -e 'cpp|h'<br>

For more information on passing parameters to the program, see my 'clap' library.
