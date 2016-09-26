#! /bin/python

import ConfigParser
import os
import re
import json
import socket

from os import listdir
from os.path import isfile, join
from datetime import timedelta
from flask import Flask, jsonify, request, render_template, make_response, current_app
from functools import update_wrapper


def crossdomain(origin=None, methods=None, headers=None,
                max_age=21600, attach_to_all=True,
                automatic_options=True):
    if methods is not None:
        methods = ', '.join(sorted(x.upper() for x in methods))
    if headers is not None and not isinstance(headers, basestring):
        headers = ', '.join(x.upper() for x in headers)
    if not isinstance(origin, basestring):
        origin = ', '.join(origin)
    if isinstance(max_age, timedelta):
        max_age = max_age.total_seconds()

    def get_methods():
        if methods is not None:
            return methods

        options_resp = current_app.make_default_options_response()
        return options_resp.headers['allow']

    def decorator(f):
        def wrapped_function(*args, **kwargs):
            if automatic_options and request.method == 'OPTIONS':
                resp = current_app.make_default_options_response()
            else:
                resp = make_response(f(*args, **kwargs))
            if not attach_to_all and request.method != 'OPTIONS':
                return resp

            h = resp.headers

            h['Access-Control-Allow-Origin'] = origin
            h['Access-Control-Allow-Methods'] = get_methods()
            h['Access-Control-Max-Age'] = str(max_age)
            if headers is not None:
                h['Access-Control-Allow-Headers'] = headers
            return resp

        f.provide_automatic_options = False
        return update_wrapper(wrapped_function, f)
    return decorator


app = Flask(__name__)


__CONF_FIS_PATH__ = '../conf/fis'

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
            mfs_str = conf.get('mf%s' % str(i+1))
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
            key = "Input%s" % str(i+1)
            input_tuple = config.items(key)
            input_dict = tuple_to_dict(input_tuple)
            self.inputs.append(FuzzyVariable(input_dict))

        num_outputs = int(self.conf.get('numoutputs'))
        for i in range(num_outputs):
            key = "Output%s" % str(i+1)
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


backend = OpenfzAPI()


@app.route('/list', methods=['GET'])
@crossdomain(origin='*')
def index():
    path = __CONF_FIS_PATH__
    files = [f for f in listdir(path) if isfile(join(path, f)) and '.fis' in f]
    
    return jsonify(files)

    
@app.route('/<path>.fis', methods=['GET'])
@crossdomain(origin='*')
def load(path):
    f = Fuzzy()
    f.load('%s/%s.fis' % (__CONF_FIS_PATH__, path))
    return jsonify(f.to_dict())


@app.route('/save', methods=['POST'])
@crossdomain(origin='*')
def create_task():
    if not request.json:
        abort(400)
    fis_file = render_template('template.fis', fis=request.json)
    with open('%s/%s.fis' % (__CONF_FIS_PATH__, request.json['system']['name']), 'w') as f:
        f.write(fis_file)
        f.close()
    return jsonify('success', 201)
    import pdb; pdb.set_trace()


@app.route('/load/<name>', methods=['PUT'])
@crossdomain(origin='*')
def loadfis(name):
    backend.connect()
    res = backend.api('loadfis %s' % name)
    backend.close()
    return res


@app.route('/unload/<slot>', methods=['PUT'])
@crossdomain(origin='*')
def unloadfis(slot):
    backend.connect()
    res = backend.api('unloadfis %s' % slot)
    backend.close()
    return res


@app.route('/summary', methods=['GET'])
@crossdomain(origin='*')
def summary():
    backend.connect()
    res = backend.api('summary')
    backend.close()
    state = res.split('\n')
    state = state[:-1]

    serialized_data = []
    for s in state:
        details = s.split(' ')
        serialized_data.append({
            'slot': details[0],
            'port': details[1],
            'name': details[2]
        })
        
    return jsonify(serialized_data)


if __name__ == '__main__':
  app.run(debug=True)
