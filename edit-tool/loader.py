#! /bin/python

from flask import Flask, jsonify, request, render_template, make_response, current_app
from functools import update_wrapper
from fuzzy import *

app = Flask(__name__)

__CONF_FIS_PATH__ = '../conf/fis'

backend = OpenfzAPI()

@app.route('/')
def index():
    return app.send_static_file('index.html')


@app.route('/editor')
def editor():
    return app.send_static_file('editor.html')


@app.route('/list', methods=['GET'])
def list():
    path = __CONF_FIS_PATH__
    files = [f for f in listdir(path) if isfile(join(path, f)) and '.fis' in f]
    
    return jsonify(files)

    
@app.route('/<path>.fis', methods=['GET'])
def load(path):
    f = Fuzzy()
    f.load('%s/%s.fis' % (__CONF_FIS_PATH__, path))
    return jsonify(f.to_dict())


@app.route('/save', methods=['POST'])
def create_task():
    if not request.json:
        abort(400)
    fis_file = render_template('template.fis', fis=request.json)
    with open('%s/%s.fis' % (__CONF_FIS_PATH__, request.json['system']['name']), 'w') as f:
        f.write(fis_file)
        f.close()
    return jsonify('success', 201)


@app.route('/load/<name>', methods=['PUT', 'GET'])
def loadfis(name):
    backend.connect()
    res = backend.api('loadfis %s' % name)
    backend.close()
    return res


@app.route('/unload/<slot>', methods=['PUT'])
def unloadfis(slot):
    backend.connect()
    res = backend.api('unloadfis %s' % slot)
    backend.close()
    return res


@app.route('/summary', methods=['GET'])
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
