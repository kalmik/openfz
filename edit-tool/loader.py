#! /bin/python

import ConfigParser, os
import re
import pickle

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
    

class FuzzyVariable(object):

    def __init__(self, conf):
        self.name = conf.get('name')
        self.num_mfs = int(conf.get('nummfs'))
        self.mfs = []
        for i in range(self.num_mfs):
            mfs_str = conf.get('mf%s' % str(i+1))
            self.mfs.append(FuzzyMFS(mfs_str))


class FuzzyRule(object):
    
    def __init__(self, premise, result, operation):
        self.premise = premise
        self.result = result
        self.operation = operation


class Fuzzy(object):
    
    def __init__(self):
        self.path = None
        self.conf = {}
        self.input = []
        self.output = []
        self.rules = []

    def load(self, buf):
        if isinstance(buf, str):
            self._load_from_file(buf)
        elif isinstance(buf, dict):
            self._load_from_dict(buf)
    
    def _load_from_dict(self, buf):
        pass

    def _load_from_file(self, path):
        self.path = path
        config = ConfigParser.RawConfigParser()
        config.read(self.path)
        
        config_system = config.items("System");
        self.conf = tuple_to_dict(config_system)

        num_inputs = int(self.conf.get('numinputs'))
        for i in range(num_inputs):
            key = "Input%s" % str(i+1)
            input_tuple = config.items(key)
            input_dict = tuple_to_dict(input_tuple)
            self.input.append(FuzzyVariable(input_dict))

        num_outputs = int(self.conf.get('numoutputs'))
        for i in range(num_outputs):
            key = "Output%s" % str(i+1)
            output_tuple = config.items(key)
            output_dict = tuple_to_dict(output_tuple)
            self.output.append(FuzzyVariable(output_dict))

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

    @classmethod
    def serialize(cls):
        serialized_data = {}

        return serialized_data
    

f = Fuzzy()
f.load('b7.fis')
print f.output[0].mfs[0].data
import pdb
pdb.set_trace()
