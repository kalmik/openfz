import ConfigParser
import os
import re
import json
import socket

from os import listdir
from os.path import isfile, join
from datetime import timedelta


class OpenfzAPI(object):
    def __init__(self):
        self.host = '127.0.0.1'
        self.port = 1337
        self.sock = None
        self.buffer_size = 2048

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))

    def close(self):
        self.api('shutdown')
        self.sock.close()

    def api(self, command):
        self.sock.send('%s\n' % command)
        return self.sock.recv(self.buffer_size)


def tuple_to_dict(data):
    return {key: value.replace("'", "") for key, value in data}


class FuzzyMFS(object):
    def __init__(self, mfs_str):
        parts = mfs_str.split(',')

        self.name = None
        self.type = None
        self.data = []

        if not parts:
            return
        pattern = r"(\w+):(\w+)"
        match = re.search(pattern, parts[0])
        if not match:
            return

        self.name = match.group(1)
        self.type = match.group(2)

        raw_data = re.sub(r'[\[\]]', '', parts[1])
        self.data = [float(x) for x in raw_data.split(' ')]

    def to_dict(self):
        return self.__dict__


class FuzzyVariable(object):
    def __init__(self, conf):
        self.name = conf.get('name')
        self.num_mfs = int(conf.get('nummfs'))
        self.functions = []
        raw_range = re.sub(r'[\[\]]', '', conf.get('range')).split(' ')
        self.value_range = [float(r) for r in raw_range]

        for i in range(self.num_mfs):
            mfs_str = conf.get('mf%s' % str(i + 1))
            self.functions.append(FuzzyMFS(mfs_str))

    def to_dict(self):
        return {
            'name': self.name,
            'num_mfs': self.num_mfs,
            'range': self.value_range,
            'functions': [mfs.to_dict() for mfs in self.functions]
        }


class FuzzyRule(object):
    def __init__(self, premise, result, operation):
        self.premise = premise
        self.result = result
        self.operation = operation

    def to_dict(self):
        return self.__dict__


class Fuzzy(object):
    def __init__(self):
        self.path = None
        self.conf = {}
        self.inputs = []
        self.outputs = []
        self.rules = []

    def load(self, buf):
        if isinstance(buf, (str, unicode)):
            self._load_from_file(buf)
        elif isinstance(buf, dict):
            self._load_from_dict(buf)

    def _load_from_dict(self, buf):
        pass

    def _load_from_file(self, path):
        print 'vai'
        self.path = path
        config = ConfigParser.RawConfigParser()
        config.read(self.path)

        config_system = config.items("System");
        self.conf = tuple_to_dict(config_system)

        num_inputs = int(self.conf.get('numinputs'))
        for i in range(num_inputs):
            key = "Input%s" % str(i + 1)
            input_tuple = config.items(key)
            input_dict = tuple_to_dict(input_tuple)
            self.inputs.append(FuzzyVariable(input_dict))

        num_outputs = int(self.conf.get('numoutputs'))
        for i in range(num_outputs):
            key = "Output%s" % str(i + 1)
            output_tuple = config.items(key)
            output_dict = tuple_to_dict(output_tuple)
            self.outputs.append(FuzzyVariable(output_dict))

        num_rules = int(self.conf.get('numrules'))
        pattern = r'(\d) (\d), (\d) \(\d\)'
        for data, operation in config.items('Rules'):
            data = data.split(',')
            premise = [int(x) for x in data[0].split(' ')]
            raw_result = data[1].split(' ')

            result = []
            for i in range(len(raw_result)):
                try:
                    result.append(int(raw_result[i]))
                except ValueError:
                    pass

            self.rules.append(FuzzyRule(premise, result, operation))

    def to_dict(self):
        serialized_data = {
            'path': self.path,
            'system': self.conf,
            'inputs': [i.to_dict() for i in self.inputs],
            'outputs': [o.to_dict() for o in self.outputs],
            'rules': [r.to_dict() for r in self.rules]
        }

        return serialized_data