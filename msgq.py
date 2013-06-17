import msgq
import json
import pickle

class IPCMessageQueue(object):
    serialize = lambda x: x
    deserialize = lambda x: x
    perm_mask = 0666

    def __init__(self, path, id, exclusive=False):
        self.key = _msgq.ftok(path, id)

        if exclusive:
            flag = _msgq.IPC_EXCL
        else:
            flag = _msgq.IPC_CREAT

        self.id = _msgq.msgget(self.key, self.perm_mask | flag)

    def put(self, item, flag=None):
        serialize = self.serialize
        if flag is None:
            flag = 0
        msg_string = self.serialize.__func__(str(item))
        _msgq.msgsnd(self.id, flag, msg_string)

    def get(self, flag=None):
        if flag is None:
            flag = 0

        serialized_string = _msgq.msgrcv(self.id, flag)

        return self.deserialize.__func__(serialized_string)

    def __iter__(self):
        while True:
            try:
                yield self.get()
            except:
                break

class PickleQueue(IPCMessageQueue):
    serialize = pickle.dumps
    deserialize = pickle.loads

class JSONQueue(IPCMessageQueue):
    serialize = json.dumps
    deserialize = json.loads
