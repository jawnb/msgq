msgq: System V IPC Message Queue Python Extension Module
========================================================

This extension wraps and makes system calls related to System V message queues 
available to Python applications. The following system calls are exposed:

- ftok: convert a pathname and a project identifier to a System V IPC key
- msgget: get a message queue identifier
- msgsnd: send message to queue
- msgrcv: receive message from queu
- msgctl: message control operations

For more information on the system calls go visit your man pages.

The idea is that Python objects are passed between processes through a message 
queue. There are several classes that support different serialization formats.
<pre>
>>> from msgq import IPCMessageQueue
>>> q = IPCMessageQueue('/tmp', 'a')
>>> q.put('this is a test')
>>> q.get()
'this is a test'

</pre>



To build the extension type in the following:
$ setup.py build
And to install the extenstion try this:
$ setup.py install
