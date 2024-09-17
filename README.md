In this repository I tried to create a logging system for both windows and linux ! If possible I want to create a single application that is usable on both Windows/Linux other wise gonna create two different branches one for Windows and another one for linux ! 

For any networking the core concept lies in socket so for the current project I am gonna be using libraries that depended on socket rather than writing my own with only socket written in c and python3!


Implementation:
Python3:

    Python3 already has a library to log system data into a text file called logging!
    
    We can use pip to install it : pip install logging

    If you are running the python3 script in linux , you need to use it in sudo mode / as root user! Since captuering/reading and editing network packets do require sudo mode , linux thinks capturing packet is insecure in some way   we have nothing to worry about !
C:
We use the standard network libraries which defines both structures and function for communication like netinet for network data structures !
This server has time stamp per packet added at the starting of the every packet capture!

I create two different files 
        Logger.c which is a network logger run on localhost!
        Access.c which is used a logging server for other machines (Still on work!)


        How to run
        Similar to other c programs we use gcc to compile it gcc Logger.c -o a.out and run the output binary with root priv (similar to python3 logging server!)
