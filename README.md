In this repository I tried to create a logging system for both windows and linux ! If possible I want to create a single application that is usable on both Windows/Linux other wise gonna create two different branches one for Windows and another one for linux ! 

For any networking the core concept lies in socket so for the current project I am gonna be using libraries that depended on socket rather than writing my own with only socket written in c and python3 ( These two are the only language I know 🤣 so appologies for any rust enthusiasts who coomes to this repo)!

If everything goes well and I didn't abandon this project in middle like the million others, it would at least usable with both cli and GUI(No promisses about it tho i will try my hardest)!

Implementation:


Python3:

    Python3 already has a library to log system data into a text file called logging!
    
    We can use pip to install it : pip install logging

    If you are running the python3 script in linux , you need to use it in sudo mode / as root user! Since captuering/reading and editing network packets do need require sudo mode , since it just read the  we have nothing to worry about !

