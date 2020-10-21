from types import SimpleNamespace as _NS


class NS(_NS):
    def __init__(self, **kwargs):
        _NS.__init__(self, **kwargs)

    def __getitem__(self, item):
        return self.__dict__[item]

    def __setitem__(self, key, value):
        self.__dict__[key] = value

    def get(self, key):
        return self.__dict__.get(key)

    def keys(self):
        return self.__dict__.keys()
